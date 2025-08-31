#include "schmixpch.h"
#include "schmix/core/Memory.h"

#include <cstring>

namespace schmix {
    void* Memory::Allocate(std::size_t size) {
        void* block = std::malloc(size);

        // todo: record

        return block;
    }

    void Memory::Free(void* block) {
        if (block == nullptr) {
            return;
        }

        // todo: record

        std::free(block);
    }

    void* Memory::Reallocate(void* block, std::size_t newSize) {
        // todo: record free

        void* newBlock = std::realloc(block, newSize);

        // todo: record alloc

        return newBlock;
    }

    void Memory::Copy(const void* src, void* dst, std::size_t size) { std::memcpy(dst, src, size); }

    void Memory::Fill(void* dst, std::uint8_t value, std::size_t size) {
        std::memset(dst, (int)value, size);
    }
} // namespace schmix
