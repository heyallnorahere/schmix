#pragma once

#include "schmix/encoding/CodecParameters.h"
#include "schmix/encoding/IO.h"

typedef struct AVCodec AVCodec;
typedef struct AVCodecContext AVCodecContext;

namespace schmix {
    class CodecStream {
    public:
        CodecStream(const IO::Callbacks& callbacks, IO::Mode mode,
                    const CodecParameters& parameters, std::size_t streamIndex);

        ~CodecStream();

        CodecStream(const CodecStream&) = delete;
        CodecStream& operator=(const CodecStream&) = delete;

        IO::Mode GetMode() const { return m_Mode; }

        const CodecParameters& GetParameters() const { return m_Parameters; }
        std::size_t GetStreamIndex() const { return m_StreamIndex; }

        bool IsOpen() const { return m_IsOpen; }

        std::size_t GetFrameSize() const;

        std::optional<std::size_t> ReadFrame(void** data);
        bool WriteFrame(const void* data, std::size_t samples);

        bool Flush();

    private:
        bool FlushPackets();

        const AVCodec* m_Codec;
        AVCodecContext* m_Context;

        IO::Callbacks m_Callbacks;
        IO::Mode m_Mode;

        CodecParameters m_Parameters;
        std::size_t m_StreamIndex;

        bool m_IsOpen;
    };
} // namespace schmix
