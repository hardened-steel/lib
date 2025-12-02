#pragma once
#include <lib/array.hpp>

namespace lib::interpreter::const_expr {

    template <std::size_t Size>
    struct Heap
    {
        constexpr static inline std::uint32_t size = Size;
        constexpr static inline std::uint32_t minsize = 4;
        constexpr static inline std::uint32_t hbits = size / minsize;
        constexpr static inline std::uint32_t hmask = hbits / 64u + (hbits % 64u ? 1u : 0u);

        std::array<std::uint64_t, hmask> heap = {0};

        constexpr std::uint32_t alloc(std::uint32_t size)
        {
            const std::uint32_t allocated_bits = size / minsize + (size % minsize ? 1u : 0u);
            for (std::uint32_t i = 0, j = 0; i < hbits; ++i) {
                if (heap[i / 64] & (std::uint64_t(1) << (i % 64))) {
                    j = 0;
                } else {
                    j += 1;
                }
                if (j == allocated_bits) {
                    const std::uint32_t start = i - allocated_bits + 1;
                    const std::uint32_t end = i + 1;
                    for (j = start; j < end; j++) {
                        heap[j / 64] |= (std::uint64_t(1) << (j % 64));
                    }
                    return start * minsize;
                }
            }
            throw std::bad_alloc();
        }

        constexpr void free(std::uint32_t ptr, std::uint32_t size)
        {
            const std::uint32_t allocated_bits = size / minsize + (size % minsize ? 1u : 0u);
            const std::uint32_t start = ptr / minsize;
            const std::uint32_t end = start + allocated_bits;
            for (std::uint32_t i = start; i < end; i++) {
                heap[i / 64] &= ~(std::uint64_t(1) << (i % 64));
            }
        }
    };
}
