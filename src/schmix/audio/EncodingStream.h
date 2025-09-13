#pragma once

typedef struct AVCodec AVCodec;
typedef struct AVCodecContext AVCodecContext;

namespace schmix {
    class EncodingStream {
    public:
        enum class Codec : std::int32_t { MP3 = 0, OGG };
        enum class Action : std::int32_t { Encoding = 0, Decoding };
        enum class SampleFormat : std::int32_t { U8 = 0, S16, S32, Float, Double };

        static void Init();

        EncodingStream(Codec codec, Action action, std::size_t channels, std::size_t sampleRate,
                       SampleFormat sampleFormat);

        ~EncodingStream();

        EncodingStream(const EncodingStream&) = delete;
        EncodingStream& operator=(const EncodingStream&) = delete;

        bool EncodeFrame(const void* pcm, std::size_t length);
        bool DecodePacket(const void* data, std::size_t dataSize);

        // free return chunk with Memory::Free
        // empty optional means error
        // null chunk means no chunks left
        std::optional<void*> GetEncodedPacket(std::size_t* dataSize);
        std::optional<void*> GetDecodedFrame(std::size_t* length, std::size_t* dataSize);

        Codec GetCodecID() const { return m_CodecID; }
        Action GetAction() const { return m_Action; }

        std::size_t GetChannels() const { return m_Channels; }
        std::size_t GetSampleRate() const { return m_SampleRate; }
        SampleFormat GetSampleFormat() const { return m_SampleFormat; }

        bool IsInitialized() const { return m_Initialized; }

    private:
        void SelectChannels();
        void SelectSampleRate();
        void SelectSampleFormat();

        Codec m_CodecID;
        Action m_Action;

        std::size_t m_Channels, m_SampleRate;
        SampleFormat m_SampleFormat;

        const AVCodec* m_Codec;
        AVCodecContext* m_Context;

        bool m_Initialized;
    };
} // namespace schmix
