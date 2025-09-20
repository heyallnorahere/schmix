#include "schmixpch.h"
#include "schmix/encoding/FormatStream.h"

#include "schmix/encoding/FFmpeg.h"

namespace schmix {
    const AVOutputFormat* FormatStream::GuessOutputFormat(const std::filesystem::path& path) {
        auto pathString = path.string();
        return av_guess_format(nullptr, pathString.c_str(), nullptr);
    }

    static int AVIOReadCallback(void* opaque, uint8_t* buf, int buf_size) {
        auto callbacks = (const IO::Callbacks*)opaque;

        std::int32_t bytesRead = callbacks->ReadPacket(buf, (std::size_t)buf_size);

        // assume eof
        return bytesRead < 0 ? (int)bytesRead : AVERROR_EOF;
    }

    static int AVIOWriteCallback(void* opaque, const uint8_t* buf, int buf_size) {
        auto callbacks = (const IO::Callbacks*)opaque;
        return (int)callbacks->WritePacket(buf, (std::size_t)buf_size);
    }

    static int64_t AVIOSeekCallback(void* opaque, int64_t offset, int whence) {
        auto callbacks = (const IO::Callbacks*)opaque;
        return callbacks->Seek(offset, (std::int32_t)whence);
    }

    FormatStream::FormatStream(const IO::Callbacks& callbacks, IO::Mode mode,
                               const AVOutputFormat* format) {
        m_IOContext = nullptr;
        m_IOBuffer = nullptr;
        m_BufferSize = 0;

        m_FormatContext = nullptr;

        m_Callbacks = callbacks;
        m_Mode = mode;

        m_IsOpen = false;

        // todo: base this off of something? lol
        // 1 kb
        m_BufferSize = 1024;
        m_IOBuffer = Memory::Allocate(m_BufferSize);

        if (m_IOBuffer == nullptr) {
            SCHMIX_ERROR("Failed to allocate IO buffer!");
            return;
        }

        int64_t (*seekCallback)(void*, int64_t, int) = callbacks.Seek ? AVIOSeekCallback : nullptr;
        int (*readPacket)(void*, uint8_t*, int) = callbacks.ReadPacket ? AVIOReadCallback : nullptr;
        int (*writePacket)(void*, const uint8_t*, int) =
            callbacks.WritePacket ? AVIOWriteCallback : nullptr;

        m_IOContext =
            avio_alloc_context((unsigned char*)m_IOBuffer, m_BufferSize, mode == IO::Mode::Output,
                               &m_Callbacks, readPacket, writePacket, seekCallback);

        if (m_IOContext == nullptr) {
            SCHMIX_ERROR("Failed to allocate AVIO context!");
            return;
        }

        m_FormatContext = avformat_alloc_context();
        if (m_FormatContext == nullptr) {
            SCHMIX_ERROR("Failed to allocate libavformat context!");
            return;
        }

        m_FormatContext->oformat = nullptr;
        m_FormatContext->pb = m_IOContext;

        bool success = false;
        switch (mode) {
        case IO::Mode::Input:
            success = OpenInput();
            break;
        case IO::Mode::Output:
            m_FormatContext->oformat = format;
            success = OpenOutput();
            break;
        }

        if (!success) {
            return;
        }

        bool streamFound = false;
        for (unsigned int i = 0; i < m_FormatContext->nb_streams; i++) {
            auto stream = m_FormatContext->streams[i];
            auto params = stream->codecpar;

            if (params->codec_type != AVMEDIA_TYPE_AUDIO) {
                continue;
            }

            m_AudioIndex = i;
            m_Parameters = params;
        }

        if (!streamFound) {
            SCHMIX_ERROR("Failed to find audio stream!");
            return;
        }

        m_IsOpen = true;
    }

    FormatStream::~FormatStream() {
        if (m_IsOpen) {
            switch (m_Mode) {
            case IO::Mode::Input:
                CloseInput();
                break;
            case IO::Mode::Output:
                CloseOutput();
                break;
            }
        }

        if (m_FormatContext != nullptr) {
            avformat_free_context(m_FormatContext);
        }

        if (m_IOContext != nullptr) {
            avio_context_free(&m_IOContext);
        }

        Memory::Free(m_IOBuffer);
    }

    std::optional<std::size_t> FormatStream::ReadPacket(void** data) {
        if (m_Mode != IO::Mode::Input) {
            SCHMIX_ERROR("This stream is not an input stream!");
            return {};
        }

        if (!m_IsOpen) {
            SCHMIX_ERROR("Stream is not open!");
            return {};
        }

        auto packet = av_packet_alloc();
        int result = av_read_frame(m_FormatContext, packet);

        if (result < 0) {
            if (result != AVERROR_EOF) {
                SCHMIX_ERROR("Failed to read frame packet from stream!");
            }

            return {};
        }

        std::size_t dataSize = (std::size_t)packet->size;
        void* buffer = Memory::Allocate(dataSize);

        Memory::Copy(packet->data, buffer, dataSize);
        av_packet_free(&packet);

        *data = buffer;
        return dataSize;
    }

    bool FormatStream::WritePacket(const void* data, std::size_t dataSize) {
        if (m_Mode != IO::Mode::Output) {
            SCHMIX_ERROR("This stream is not an output stream!");
            return false;
        }

        if (!m_IsOpen) {
            SCHMIX_ERROR("Stream is not open!");
            return false;
        }

        auto packetBuffer = Memory::Allocate(dataSize);
        Memory::Copy(data, packetBuffer, dataSize);

        auto packet = av_packet_alloc();
        av_packet_from_data(packet, (uint8_t*)packetBuffer, dataSize);

        int result = av_write_frame(m_FormatContext, packet);

        av_packet_free(&packet);
        Memory::Free(packetBuffer);

        if (result < 0) {
            SCHMIX_ERROR("Failed to write packet to stream!");
            return false;
        }

        return true;
    }

    bool FormatStream::Flush() {
        if (!m_IsOpen) {
            SCHMIX_ERROR("Attempted to flush stream that is not open!");
            return false;
        }

        if (avformat_flush(m_FormatContext) < 0) {
            SCHMIX_ERROR("Failed to flush libavformat context!");
            return false;
        }

        avio_flush(m_IOContext);

        return true;
    }

    bool FormatStream::OpenInput() {
        if (avformat_open_input(&m_FormatContext, nullptr, nullptr, nullptr) < 0) {
            SCHMIX_ERROR("Failed to open libavformat input!");
            return false;
        }

        if (avformat_find_stream_info(m_FormatContext, nullptr) < 0) {
            SCHMIX_ERROR("Failed to find metadata for this stream!");
            return false;
        }

        return true;
    }

    void FormatStream::CloseInput() { avformat_close_input(&m_FormatContext); }

    bool FormatStream::OpenOutput() {
        if (avformat_write_header(m_FormatContext, nullptr) < 0) {
            SCHMIX_ERROR("Failed to write header to stream!");
            return false;
        }

        return true;
    }

    void FormatStream::CloseOutput() { /* nothing? */ }
} // namespace schmix
