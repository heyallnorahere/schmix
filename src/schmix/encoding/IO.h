#pragma once

namespace schmix {
    class IO {
    public:
        struct Callbacks {
            std::function<std::int32_t(void* buffer, std::size_t bufferSize)> ReadPacket;
            std::function<std::int32_t(const void* buffer, std::size_t bufferSize)> WritePacket;
            std::function<std::int64_t(std::int64_t offset, std::int32_t origin)> Seek;
        };

        enum class Mode : std::int32_t { Input = 0, Output };

        IO() = delete;

        static void Init();
    };
} // namespace schmix
