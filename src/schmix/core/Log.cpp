#include "schmixpch.h"
#include "schmix/core/Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace schmix {
    spdlog::logger* g_Logger = nullptr;

    bool CreateLogger(const std::optional<std::filesystem::path>& logDirectory) {
        if (g_Logger != nullptr) {
            return false;
        }

        g_Logger = new spdlog::logger("schmix");
        g_Logger->set_level(spdlog::level::trace);

        spdlog::level::level_enum level;
#ifdef SCHMIX_IS_DEBUG
        level = spdlog::level::debug;
#else
        level = spdlog::level::info;
#endif

        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(level);
        consoleSink->set_pattern("[%s:%# %!] [%^%l%$] %v");
        g_Logger->sinks().push_back(consoleSink);

        std::optional<std::string> logPathStr;
        if (logDirectory.has_value()) {
            auto logPath = logDirectory.value() / "schmix.log";
            logPathStr = logPath.lexically_normal().string();

            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logPathStr.value(), 10 * 1024 * 1024, 3);

            fileSink->set_level(spdlog::level::trace);
            fileSink->set_pattern("[%x %X] [%g:%# %!] [%l] %v");

            g_Logger->sinks().push_back(fileSink);
        }

        if (logPathStr.has_value()) {
            SCHMIX_INFO("Logging to file: {}", logPathStr->c_str());
        }

        return true;
    }

    bool ResetLogger() {
        if (g_Logger == nullptr) {
            return false;
        }

        SCHMIX_INFO("Closing log...");

        delete g_Logger;
        g_Logger = nullptr;

        return true;
    }
} // namespace schmix
