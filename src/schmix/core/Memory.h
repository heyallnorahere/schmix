#pragma once

#include <new>

namespace schmix {
    class Memory {
    public:
        static void* Allocate(std::size_t size);
        static void Free(void* block);

        static void* Reallocate(void* block, std::size_t newSize);
        static void* AllocateZeroedArray(std::size_t nmemb, std::size_t size);

        static void Copy(const void* src, void* dst, std::size_t size);
        static void Fill(void* dst, std::uint8_t value, std::size_t size);

        Memory() = delete;
    };
} // namespace schmix

inline void* operator new(std::size_t size) {
    void* block = schmix::Memory::Allocate(size);
    if (block == nullptr) {
        throw std::bad_alloc();
    }

    return block;
}

inline void* operator new[](std::size_t size) {
    void* block = schmix::Memory::Allocate(size);
    if (block == nullptr) {
        throw std::bad_alloc();
    }

    return block;
}

inline void operator delete(void* block) noexcept { return schmix::Memory::Free(block); }
inline void operator delete[](void* block) noexcept { return schmix::Memory::Free(block); }
