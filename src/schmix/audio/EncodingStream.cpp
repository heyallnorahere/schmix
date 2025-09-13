#include "schmixpch.h"
#include "schmix/audio/EncodingStream.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

namespace schmix {
    static void avLogCallback(void* avcl, int level, const char* fmt, va_list vl) {
        static constexpr std::size_t maxMessageLength = 256;
        char message[maxMessageLength];

        std::vsnprintf(message, maxMessageLength, fmt, vl);

        if (level <= AV_LOG_FATAL) {
            SCHMIX_CRITICAL("av_log: {}", message);
            return;
        }

        if (level <= AV_LOG_ERROR) {
            SCHMIX_ERROR("av_log: {}", message);
            return;
        }

        if (level <= AV_LOG_WARNING) {
            SCHMIX_WARN("av_log: {}", message);
            return;
        }

        if (level <= AV_LOG_INFO) {
            SCHMIX_INFO("av_log: {}", message);
            return;
        }

        if (level <= AV_LOG_VERBOSE) {
            SCHMIX_DEBUG("av_log: {}", message);
            return;
        }

        SCHMIX_TRACE("av_log: {}", message);
    }

    void EncodingStream::Init() {
        SCHMIX_DEBUG("Setting FFmpeg log settings...");

        av_log_set_level(AV_LOG_TRACE);
        av_log_set_callback(avLogCallback);
    }

    static AVCodecID ConvertCodecID(EncodingStream::Codec codec) {
        switch (codec) {
        case EncodingStream::Codec::MP3:
            return AV_CODEC_ID_MP3;
        case EncodingStream::Codec::OGG:
            return AV_CODEC_ID_VORBIS;
        default:
            return AV_CODEC_ID_NONE;
        }
    }

    EncodingStream::EncodingStream(Codec codec, Action action, std::size_t channels,
                                   std::size_t sampleRate, SampleFormat sampleFormat) {
        m_Initialized = false;

        m_CodecID = codec;
        m_Action = action;

        m_Channels = channels;
        m_SampleRate = sampleRate;
        m_SampleFormat = sampleFormat;

        m_Codec = nullptr;
        m_Context = nullptr;

        auto id = ConvertCodecID(codec);
        if (id == AV_CODEC_ID_NONE) {
            SCHMIX_ERROR("Unsupported codec!");
            return;
        }

        switch (action) {
        case Action::Encoding:
            m_Codec = avcodec_find_encoder(id);
            break;
        case Action::Decoding:
            m_Codec = avcodec_find_decoder(id);
            break;
        default:
            SCHMIX_ERROR("Invalid action!");
            return;
        }

        if (m_Codec == nullptr) {
            SCHMIX_ERROR("Failed to find codec!");
            return;
        }

        m_Context = avcodec_alloc_context3(m_Codec);
        if (m_Context == nullptr) {
            SCHMIX_ERROR("Failed to allocate context!");
            return;
        }

        SelectChannels();
        SelectSampleRate();
        SelectSampleFormat();

        if (avcodec_open2(m_Context, m_Codec, nullptr) < 0) {
            SCHMIX_ERROR("Failed to open codec!");
            return;
        }

        m_Initialized = true;
    }

    EncodingStream::~EncodingStream() {
        if (m_Context != nullptr) {
            avcodec_free_context(&m_Context);
        }
    }

    bool EncodingStream::EncodeFrame(const void* pcm, std::size_t length) {
        if (!m_Initialized) {
            SCHMIX_ERROR("Encoding stream not fully initialized!");
            return false;
        }

        if (m_Action != Action::Encoding) {
            SCHMIX_ERROR("Attempted to encode on a non-encoding stream!");
            return false;
        }

        auto avFrame = av_frame_alloc();
        if (avFrame == nullptr) {
            SCHMIX_ERROR("Failed to allocate AV frame!");
            return false;
        }

        avFrame->nb_samples = length;
        avFrame->sample_rate = m_Context->sample_rate;
        avFrame->ch_layout = m_Context->ch_layout;

        std::size_t bufferSize =
            av_samples_get_buffer_size(nullptr, m_Channels, length, m_Context->sample_fmt, 0);

        void* frameData = Memory::Allocate(bufferSize);
        Memory::Copy(pcm, frameData, bufferSize);

        int ret = avcodec_fill_audio_frame(avFrame, m_Context->ch_layout.nb_channels,
                                           m_Context->sample_fmt, (const uint8_t*)frameData,
                                           (int)bufferSize, 0);

        if (ret < 0) {
            SCHMIX_ERROR("Failed to set up AV frame pointers!");

            Memory::Free(frameData);
            av_frame_free(&avFrame);

            return false;
        }

        ret = avcodec_send_frame(m_Context, avFrame);

        Memory::Free(frameData);
        av_frame_free(&avFrame);

        if (ret < 0) {
            SCHMIX_ERROR("Failed to send AV frame!");
            return false;
        }

        return true;
    }

    bool EncodingStream::DecodePacket(const void* data, std::size_t dataSize) {
        if (!m_Initialized) {
            SCHMIX_ERROR("Encoding stream not fully initialized!");
            return false;
        }

        if (m_Action != Action::Decoding) {
            SCHMIX_ERROR("Attempted to decode on a non-decoding stream!");
            return false;
        }

        void* packetData = av_malloc(dataSize);
        Memory::Copy(data, packetData, dataSize);

        auto avPacket = av_packet_alloc();
        int ret = av_packet_from_data(avPacket, (uint8_t*)packetData, dataSize);

        if (ret < 0) {
            SCHMIX_ERROR("Failed to attach memory chunk to packet!");

            av_freep(&packetData);
            av_packet_free(&avPacket);

            return false;
        }

        ret = avcodec_send_packet(m_Context, avPacket);
        av_packet_unref(avPacket);

        if (ret < 0) {
            SCHMIX_ERROR("Failed to send packet to decoding context!");
            return false;
        }

        return true;
    }

    std::optional<void*> EncodingStream::GetEncodedPacket(std::size_t* dataSize) {
        if (!m_Initialized) {
            SCHMIX_ERROR("Encoding stream not fully initialized!");
            return {};
        }

        if (m_Action != Action::Encoding) {
            SCHMIX_ERROR("Attempted to encode on a non-encoding stream!");
            return {};
        }

        auto outputPacket = av_packet_alloc();
        if (outputPacket == nullptr) {
            SCHMIX_ERROR("Failed to allocate output packet!");
            return {};
        }

        int ret = avcodec_receive_packet(m_Context, outputPacket);

        std::optional<void*> allocatedPacket;
        if (ret >= 0) {
            std::size_t packetSize = outputPacket->size;

            allocatedPacket = Memory::Allocate(packetSize);
            Memory::Copy(outputPacket->data, allocatedPacket.value(), packetSize);

            if (dataSize != nullptr) {
                *dataSize = packetSize;
            }
        } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            allocatedPacket = nullptr;
        } else {
            SCHMIX_ERROR("Error retrieving encoded packet!");
        }

        av_packet_free(&outputPacket);
        return allocatedPacket;
    }

    std::optional<void*> EncodingStream::GetDecodedFrame(std::size_t* length,
                                                         std::size_t* dataSize) {
        if (!m_Initialized) {
            SCHMIX_ERROR("Encoding stream not fully initialized!");
            return {};
        }

        if (m_Action != Action::Decoding) {
            SCHMIX_ERROR("Attempted to decode on a non-decoding stream!");
            return {};
        }

        auto outputFrame = av_frame_alloc();
        if (outputFrame == nullptr) {
            SCHMIX_ERROR("Failed to allocate output frame!");
            return {};
        }

        int ret = avcodec_receive_frame(m_Context, outputFrame);

        std::optional<void*> allocatedFrame;
        if (ret >= 0) {
            std::size_t sampleCount = outputFrame->nb_samples;
            std::size_t bufferSize = av_samples_get_buffer_size(nullptr, m_Channels, sampleCount,
                                                                m_Context->sample_fmt, 0);

            allocatedFrame = Memory::Allocate(bufferSize);
            Memory::Copy(outputFrame->data, allocatedFrame.value(), bufferSize);

            if (length != nullptr) {
                *length = sampleCount;
            }

            if (dataSize != nullptr) {
                *dataSize = bufferSize;
            }
        } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            allocatedFrame = nullptr;
        } else {
            SCHMIX_ERROR("Error retrieving decoded frame!");
        }

        av_frame_free(&outputFrame);
        return allocatedFrame;
    }

    template <typename _Ty>
    static const _Ty* GetCodecSupport(AVCodecContext* context, const AVCodec* codec,
                                      AVCodecConfig config, int* items) {
        const void* data;
        avcodec_get_supported_config(context, codec, config, 0, &data, items);

        return (const _Ty*)data;
    }

    void EncodingStream::SelectChannels() {
        int layoutCount;
        auto layouts = GetCodecSupport<AVChannelLayout>(
            m_Context, m_Codec, AV_CODEC_CONFIG_CHANNEL_LAYOUT, &layoutCount);

        if (layouts == nullptr) {
            m_Context->ch_layout.order = AV_CHANNEL_ORDER_UNSPEC;
            m_Context->ch_layout.nb_channels = (int)m_Channels;

            return;
        }

        for (int i = 0; i < layoutCount; i++) {
            const auto& layout = layouts[i];

            if ((std::size_t)layout.nb_channels == m_Channels) {
                m_Context->ch_layout = layout;
                return;
            }
        }

        m_Context->ch_layout = layouts[0];

        std::size_t requested = m_Channels;
        m_Channels = (std::size_t)m_Context->ch_layout.nb_channels;

        SCHMIX_WARN("No support found for {} channels - using first supported ({} channels)",
                    requested, m_Channels);
    }

    void EncodingStream::SelectSampleRate() {
        int rateCount;
        auto rates =
            GetCodecSupport<int>(m_Context, m_Codec, AV_CODEC_CONFIG_SAMPLE_RATE, &rateCount);

        bool found;
        if (rates == nullptr) {
            found = true;
        } else {
            found = false;

            for (int i = 0; i < rateCount; i++) {
                int supportedRate = rates[i];
                if ((std::size_t)supportedRate == m_SampleRate) {
                    found = true;
                    break;
                }
            }
        }

        if (found) {
            m_Context->sample_rate = (int)m_SampleRate;
        } else {
            m_Context->sample_rate = rates[0];

            std::size_t requested = m_SampleRate;
            m_SampleRate = (std::size_t)m_Context->sample_rate;

            SCHMIX_WARN("No support found for sample rate of {} Hz - using first supported ({} Hz)",
                        requested, m_SampleRate);
        }
    }

    static AVSampleFormat ConvertSampleFormat(EncodingStream::SampleFormat format) {
        switch (format) {
        case EncodingStream::SampleFormat::U8:
            return AV_SAMPLE_FMT_U8;
        case EncodingStream::SampleFormat::S16:
            return AV_SAMPLE_FMT_S16;
        case EncodingStream::SampleFormat::S32:
            return AV_SAMPLE_FMT_S32;
        case EncodingStream::SampleFormat::Float:
            return AV_SAMPLE_FMT_FLT;
        case EncodingStream::SampleFormat::Double:
            return AV_SAMPLE_FMT_DBL;
        default:
            throw std::runtime_error("Invalid sample format!");
        }
    }

    void EncodingStream::SelectSampleFormat() {
        auto requestedFormat = ConvertSampleFormat(m_SampleFormat);

        int formatCount;
        auto formats = GetCodecSupport<AVSampleFormat>(m_Context, m_Codec,
                                                       AV_CODEC_CONFIG_SAMPLE_FORMAT, &formatCount);

        bool found;
        if (formats == nullptr) {
            found = true;
        } else {
            found = false;

            for (int i = 0; i < formatCount; i++) {
                auto supportedFormat = formats[i];
                if (supportedFormat == requestedFormat) {
                    found = true;
                    break;
                }
            }
        }

        AVSampleFormat selectedFormat;
        if (found) {
            selectedFormat = requestedFormat;
        } else {
            selectedFormat = formats[0];
            SCHMIX_WARN("No support found for requested sample format - using default");
        }

        switch (m_Action) {
        case Action::Encoding:
            m_Context->sample_fmt = selectedFormat;
            break;
        case Action::Decoding:
            m_Context->request_sample_fmt = selectedFormat;
            break;
        default:
            throw std::runtime_error("????");
        }
    }
} // namespace schmix
