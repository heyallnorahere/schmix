#include "schmixpch.h"
#include "schmix/encoding/CodecStream.h"

#include "schmix/encoding/FFmpeg.h"

namespace schmix {
    CodecStream::CodecStream(const IO::Callbacks& callbacks, IO::Mode mode,
                             const CodecParameters& parameters, std::size_t streamIndex) {
        m_IsOpen = false;

        m_Callbacks = callbacks;
        m_Mode = mode;

        m_Parameters = parameters;
        m_StreamIndex = streamIndex;

        m_Codec = nullptr;
        m_Context = nullptr;

        auto codecParams = m_Parameters.Get();
        auto avCodecID = codecParams->codec_id;

        switch (mode) {
        case IO::Mode::Input:
            m_Codec = avcodec_find_decoder(avCodecID);
            break;
        case IO::Mode::Output:
            m_Codec = avcodec_find_encoder(avCodecID);
            break;
        }

        if (m_Codec == nullptr) {
            SCHMIX_ERROR("Failed to find codec!");
            return;
        }

        m_Context = avcodec_alloc_context3(m_Codec);
        if (m_Context == nullptr) {
            SCHMIX_ERROR("Failed to allocate libavcodec context!");
            return;
        }

        avcodec_parameters_to_context(m_Context, codecParams);

        if (avcodec_open2(m_Context, m_Codec, nullptr) < 0) {
            SCHMIX_ERROR("Failed to open codec in context!");
            return;
        }

        m_IsOpen = true;
    }

    CodecStream::~CodecStream() { avcodec_free_context(&m_Context); }

    std::size_t CodecStream::GetFrameSize() const { return m_Context->frame_size; }

    static std::optional<std::size_t> CopyFrameToNewBuffer(const AVFrame* frame,
                                                           std::size_t streamIndex, void** data) {
        std::size_t samples = frame->nb_samples;
        std::size_t channels = frame->ch_layout.nb_channels;
        auto sampleFormat = (AVSampleFormat)frame->format;

        std::size_t bufferSize =
            av_samples_get_buffer_size(nullptr, channels, samples, sampleFormat, 0);

        void* buffer = Memory::Allocate(bufferSize);
        if (buffer == nullptr) {
            SCHMIX_ERROR("Failed to allocate memory for frame output!");
            return {};
        }

        Memory::Copy(frame->data[streamIndex], buffer, bufferSize);

        *data = buffer;
        return samples;
    }

    std::optional<std::size_t> CodecStream::ReadFrame(void** data) {
        *data = nullptr;

        if (m_Mode != IO::Mode::Input) {
            SCHMIX_ERROR("Not an input stream!");
            return {};
        }

        if (!m_IsOpen) {
            SCHMIX_ERROR("Stream is not open!");
            return {};
        }

        // todo: pull this number from somewhere other than my ass
        std::size_t packetSize = 2048;

        std::size_t bufferSize = packetSize + AV_INPUT_BUFFER_PADDING_SIZE;
        void* packetBuffer = Memory::Allocate(bufferSize);

        bool success = true;
        while (true) {
            std::int32_t bytesRead = m_Callbacks.ReadPacket(packetBuffer, packetSize);
            if (bytesRead < 0) {
                SCHMIX_WARN("Failed to read packet from stream - assuming EOF");
                break;
            }

            if (bytesRead == 0) {
                SCHMIX_DEBUG("Packet stream EOF");
                break;
            }

            auto packet = av_packet_alloc();
            av_packet_from_data(packet, (uint8_t*)packetBuffer, bytesRead);

            int result = avcodec_send_packet(m_Context, packet);
            av_packet_free(&packet);

            if (result < 0) {
                SCHMIX_ERROR("Failed to send packet to stream!");

                success = true;
                break;
            }
        }

        Memory::Free(packetBuffer);
        if (!success) {
            SCHMIX_WARN("Error piping in data to stream - may be incomplete");

            // do we want to fail here?
        }

        auto frame = av_frame_alloc();
        if (frame == nullptr) {
            SCHMIX_ERROR("Failed to allocate frame!");
            return {};
        }

        int ret = avcodec_receive_frame(m_Context, frame);

        std::optional<std::size_t> result;
        if (ret >= 0) {
            result = CopyFrameToNewBuffer(frame, m_StreamIndex, data);
        } else if (ret != AVERROR_EOF) {
            SCHMIX_ERROR("Failed to receive frame from stream!");
        }

        av_frame_unref(frame);
        av_frame_free(&frame);

        return result;
    }

    bool CodecStream::WriteFrame(const void* data, std::size_t samples) {
        if (m_Mode != IO::Mode::Output) {
            SCHMIX_ERROR("Not an output stream!");
            return false;
        }

        if (!m_IsOpen) {
            SCHMIX_ERROR("Stream is not open!");
            return false;
        }

        std::size_t channels = m_Parameters.GetChannels();
        auto sampleFormat = (AVSampleFormat)m_Parameters.GetAVSampleFormat();

        std::size_t sampleBufferSize =
            av_samples_get_buffer_size(nullptr, channels, samples, sampleFormat, 0);

        std::size_t bufferSize = sampleBufferSize + AV_INPUT_BUFFER_PADDING_SIZE;
        auto frameBuffer = Memory::Allocate(bufferSize);

        Memory::Copy(data, frameBuffer, sampleBufferSize);

        auto frame = av_frame_alloc();
        frame->ch_layout = m_Parameters.Get()->ch_layout;
        frame->nb_samples = samples;
        frame->format = (int)sampleFormat;

        avcodec_fill_audio_frame(frame, channels, sampleFormat, (uint8_t*)frameBuffer, bufferSize,
                                 0);

        int ret = avcodec_send_frame(m_Context, frame);

        av_frame_free(&frame);
        Memory::Free(frameBuffer);

        if (ret < 0) {
            SCHMIX_ERROR("Failed to send frame to stream!");
            return false;
        }

        return FlushPackets();
    }

    bool CodecStream::Flush() {
        if (!m_IsOpen) {
            SCHMIX_ERROR("Stream is not open!");
        }

        if (m_Mode == IO::Mode::Output || !FlushPackets()) {
            SCHMIX_ERROR("Failed to flush packets to underlying stream!");
            return false;
        }

        avcodec_flush_buffers(m_Context);

        return true;
    }

    bool CodecStream::FlushPackets() {
        auto packet = av_packet_alloc();
        if (packet == nullptr) {
            SCHMIX_ERROR("Failed to allocate packet!");
            return false;
        }

        bool success = true;
        while (true) {
            int ret = avcodec_receive_packet(m_Context, packet);
            if (ret < 0) {
                if (ret != AVERROR_EOF) {
                    SCHMIX_ERROR("Failed to retrieve encoded packet from stream!");
                    success = false;
                }

                break;
            }

            std::int32_t bytesWritten = m_Callbacks.WritePacket(packet->data, packet->size);
            if (bytesWritten < 0) {
                SCHMIX_ERROR("Failed to write packet to stream!");
                success = false;
            }
        }

        av_packet_unref(packet);
        av_packet_free(&packet);

        return success;
    }
} // namespace schmix
