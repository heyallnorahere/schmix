#pragma once

typedef struct AVCodecParameters AVCodecParameters;

namespace schmix {
    class CodecParameters {
    public:
        CodecParameters();
        ~CodecParameters();

        CodecParameters(const AVCodecParameters* source);

        CodecParameters(const CodecParameters& other);
        CodecParameters& operator=(const CodecParameters& other);

        CodecParameters(CodecParameters&& other);
        CodecParameters& operator=(CodecParameters&& other);

        AVCodecParameters* Get() { return m_Parameters; }
        const AVCodecParameters* Get() const { return m_Parameters; }

        std::size_t GetChannels() const;

        // int32 so we dont have to include the header
        std::int32_t GetAVSampleFormat() const;

        // todo: more accessors for bindings

        std::size_t CalculateFrameBufferSize(std::size_t samples) const;

    private:
        void Reset();

        AVCodecParameters* m_Parameters;
    };
} // namespace schmix
