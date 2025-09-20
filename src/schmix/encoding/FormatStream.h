#pragma once

#include "schmix/encoding/CodecParameters.h"
#include "schmix/encoding/IO.h"

typedef struct AVIOContext AVIOContext;
typedef struct AVFormatContext AVFormatContext;
typedef struct AVOutputFormat AVOutputFormat;

namespace schmix {
    class FormatStream {
    public:
        static const AVOutputFormat* GuessOutputFormat(const std::filesystem::path& path);

        FormatStream(const IO::Callbacks& callbacks, IO::Mode mode,
                     const AVOutputFormat* outputFormat);

        ~FormatStream();

        FormatStream(const FormatStream&) = delete;
        FormatStream& operator=(const FormatStream&) = delete;

        IO::Mode GetMode() const { return m_Mode; }
        bool IsOpen() const { return m_IsOpen; }

        std::size_t GetAudioStreamIndex() const { return m_AudioIndex; }
        const CodecParameters& GetCodecParameters() const { return m_Parameters; }

        std::optional<std::size_t> ReadPacket(void** data);
        bool WritePacket(const void* data, std::size_t dataSize);

        bool Flush();

    private:
        bool OpenInput();
        void CloseInput();

        bool OpenOutput();
        void CloseOutput();

        AVIOContext* m_IOContext;
        void* m_IOBuffer;
        std::size_t m_BufferSize;

        AVFormatContext* m_FormatContext;

        std::size_t m_AudioIndex;
        CodecParameters m_Parameters;

        IO::Callbacks m_Callbacks;
        IO::Mode m_Mode;

        bool m_IsOpen;
    };
} // namespace schmix
