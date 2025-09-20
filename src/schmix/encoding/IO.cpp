#include "schmixpch.h"
#include "schmix/encoding/IO.h"

#include "schmix/encoding/FFmpeg.h"

namespace schmix {
    static void FFmpegLogCallback(void* avcl, int level, const char* fmt, va_list vl) {
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

    void IO::Init() {
        SCHMIX_DEBUG("Setting FFmpeg log settings...");

        av_log_set_level(AV_LOG_TRACE);
        av_log_set_callback(FFmpegLogCallback);
    }
} // namespace schmix
