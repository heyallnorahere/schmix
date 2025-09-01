#pragma once

#include <spdlog/spdlog.h>

namespace schmix {
    extern spdlog::logger* g_Logger;

    bool CreateLogger(const std::optional<std::filesystem::path>& logDirectory);
    bool ResetLogger();
} // namespace schmix

#define SCHMIX_TRACE(...) SPDLOG_LOGGER_TRACE(::schmix::g_Logger, __VA_ARGS__)
#define SCHMIX_DEBUG(...) SPDLOG_LOGGER_DEBUG(::schmix::g_Logger, __VA_ARGS__)
#define SCHMIX_INFO(...) SPDLOG_LOGGER_INFO(::schmix::g_Logger, __VA_ARGS__)
#define SCHMIX_WARN(...) SPDLOG_LOGGER_WARN(::schmix::g_Logger, __VA_ARGS__)
#define SCHMIX_ERROR(...) SPDLOG_LOGGER_ERROR(::schmix::g_Logger, __VA_ARGS__)
#define SCHMIX_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::schmix::g_Logger, __VA_ARGS__)
