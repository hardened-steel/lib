#pragma once
#include <array>
#include <cstddef>
#include <tuple>

#include <lib/tag.hpp>
#include <lib/array.hpp>

namespace lib::interpreter {

    enum OpCode: std::uint8_t
    {
        Nop,
        Lt1, Lt2, Lt4,
        Ld1, Ld2, Ld4,
        St1, St2, St4,
        Pop, Push,
        Cmp1, Cmp2, Cmp4,
        Jmp,
        Call, Ret,
        SaveSP, LoadSP,
        Alloc, Free,
        Add1, Add2, Add4
    };

    template<std::size_t HeapSize, std::size_t StackSize>
    struct Context
    {
        constexpr static inline std::uint32_t size = HeapSize + StackSize;
        constexpr static inline std::uint32_t minsize = 4;
        constexpr static inline std::uint32_t hbits = size / minsize;
        constexpr static inline std::uint32_t hmask = hbits / 64u + (hbits % 64u ? 1u : 0u);

        std::array<std::uint8_t, HeapSize + StackSize> storage = {0};
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

        constexpr std::uint8_t load(std::uint32_t ptr, lib::tag_t<std::uint8_t>) const
        {
            return storage[ptr];
        }
        constexpr void save(std::uint32_t ptr, std::uint8_t value)
        {
            storage[ptr] = value;
        }

        constexpr std::uint16_t load(std::uint32_t ptr, lib::tag_t<std::uint16_t>) const
        {
            std::uint16_t value = 0;
            value |= (static_cast<std::uint16_t>(storage[ptr + 0]) << 0u);
            value |= (static_cast<std::uint16_t>(storage[ptr + 1]) << 8u);
            return value;
        }
        constexpr void save(std::uint32_t ptr, std::uint16_t value)
        {
            storage[ptr + 0] = static_cast<std::uint8_t>(value >> 0u);
            storage[ptr + 1] = static_cast<std::uint8_t>(value >> 8u);
        }

        constexpr std::uint32_t load(std::uint32_t ptr, lib::tag_t<std::uint32_t>) const
        {
            std::uint32_t value = 0;
            value |= (static_cast<std::uint16_t>(storage[ptr + 0]) << 0u);
            value |= (static_cast<std::uint16_t>(storage[ptr + 1]) << 8u);
            value |= (static_cast<std::uint16_t>(storage[ptr + 2]) << 16u);
            value |= (static_cast<std::uint16_t>(storage[ptr + 3]) << 24u);
            return value;
        }
        constexpr void save(std::uint32_t ptr, std::uint32_t value)
        {
            storage[ptr + 0] = static_cast<std::uint8_t>(value >> 0u);
            storage[ptr + 1] = static_cast<std::uint8_t>(value >> 8u);
            storage[ptr + 2] = static_cast<std::uint8_t>(value >> 16u);
            storage[ptr + 3] = static_cast<std::uint8_t>(value >> 24u);
        }

        template<class T>
        constexpr T pop(std::uint32_t& sp, lib::tag_t<T> tag) const
        {
            auto val = load(sp, tag);
            sp += sizeof(T);
            return val;
        }
        template<class T>
        constexpr void push(std::uint32_t& sp, T value)
        {
            save(sp - sizeof(T), value);
            sp -= sizeof(T);
        }
    };

    template<std::size_t HeapSize, std::size_t StackSize>
    constexpr auto run(const std::uint8_t* program)
    {
        Context<HeapSize, StackSize> context;
        std::uint32_t sp = context.alloc(StackSize) + StackSize;
        std::uint32_t ip = 0;

        context.push(sp, std::uint32_t(0));

        while(true) {
            const auto op = static_cast<OpCode>(program[ip]);
            switch (op)
            {
            case OpCode::Call: {
                    const auto ptr = context.pop(sp, lib::tag<std::uint32_t>);
                    context.push(sp, ip + 1);
                    ip = ptr;
                }
                break;
            case OpCode::Ret: {
                    const auto ret = context.pop(sp, lib::tag<std::uint32_t>);
                    if (ret == std::uint32_t(0)) {
                        return context;
                    }
                    ip = ret;
                }
                break;
            case OpCode::Lt1: {
                    const auto arg = context.load(ip + 1, lib::tag<std::uint8_t>);
                    context.push(sp, arg);
                    ip += 2;
                }
                break;
            case OpCode::Lt2: {
                    const auto arg = context.load(ip + 1, lib::tag<std::uint16_t>);
                    context.push(sp, arg);
                    ip += 3;
                }
                break;
            case OpCode::Lt4: {
                    const auto arg = context.load(ip + 1, lib::tag<std::uint32_t>);
                    context.push(sp, arg);
                    ip += 5;
                }
                break;
            case OpCode::St1: {
                    const auto ptr = context.pop(sp, lib::tag<std::uint32_t>);
                    const auto val = context.pop(sp, lib::tag<std::uint8_t>);
                    context.save(ptr, val);
                    ip += 1;
                }
                break;
            case OpCode::St2: {
                    const auto ptr = context.pop(sp, lib::tag<std::uint32_t>);
                    const auto val = context.pop(sp, lib::tag<std::uint16_t>);
                    context.save(ptr, val);
                    ip += 1;
                }
                break;
            case OpCode::St4: {
                    const auto ptr = context.pop(sp, lib::tag<std::uint32_t>);
                    const auto val = context.pop(sp, lib::tag<std::uint32_t>);
                    context.save(ptr, val);
                    ip += 1;
                }
                break;
            case OpCode::Ld1: {
                    const auto ptr = context.pop(sp, lib::tag<std::uint32_t>);
                    const auto val = context.load(ptr, lib::tag<std::uint8_t>);
                    context.push(sp, val);
                    ip += 5;
                }
                break;
            case OpCode::Ld2: {
                    const auto ptr = context.pop(sp, lib::tag<std::uint32_t>);
                    const auto val = context.load(ptr, lib::tag<std::uint16_t>);
                    context.push(sp, val);
                    ip += 5;
                }
                break;
            case OpCode::Ld4: {
                    const auto ptr = context.pop(sp, lib::tag<std::uint32_t>);
                    const auto val = context.load(ptr, lib::tag<std::uint32_t>);
                    context.push(sp, val);
                    ip += 5;
                }
                break;
            case OpCode::Push: {
                    const auto arg = context.load(ip + 1, lib::tag<std::uint32_t>);
                    sp -= arg;
                    ip += 5;
                }
                break;
            case OpCode::Pop: {
                    const auto arg = context.load(ip + 1, lib::tag<std::uint32_t>);
                    sp += arg;
                    ip += 5;
                }
                break;
            case OpCode::Jmp: {
                    const auto ptr = context.load(ip + 1, lib::tag<std::uint32_t>);
                    const auto val = context.pop(sp, lib::tag<std::uint8_t>);
                    if (val != 0) {
                        ip += ptr;
                    } else {
                        ip += 5;
                    }
                }
                break;
            case OpCode::Cmp1: {
                    const auto val1 = context.pop(sp, lib::tag<std::uint8_t>);
                    const auto val2 = context.pop(sp, lib::tag<std::uint8_t>);
                    context.push(sp, std::uint8_t(val1 - val2));
                    ip += 1;
                }
                break;
            case OpCode::Cmp2: {
                    const auto val1 = context.pop(sp, lib::tag<std::uint16_t>);
                    const auto val2 = context.pop(sp, lib::tag<std::uint16_t>);
                    context.push(sp, std::uint8_t(val1 == val2 ? 0 : 1));
                    ip += 1;
                }
                break;
            case OpCode::Cmp4: {
                    const auto val1 = context.pop(sp, lib::tag<std::uint32_t>);
                    const auto val2 = context.pop(sp, lib::tag<std::uint32_t>);
                    context.push(sp, std::uint8_t(val1 == val2 ? 0 : 1));
                    ip += 1;
                }
                break;
            case OpCode::Alloc: {
                    const auto size = context.pop(sp, lib::tag<std::uint32_t>);
                    context.push(sp, context.alloc(size));
                    ip += 1;
                }
                break;
            case OpCode::Free: {
                    const auto ptr = context.pop(sp, lib::tag<std::uint32_t>);
                    const auto size = context.pop(sp, lib::tag<std::uint32_t>);
                    context.free(ptr, size);
                    ip += 1;
                }
                break;
            case OpCode::SaveSP: {
                    context.push(sp, sp);
                    ip += 1;
                }
                break;
            case OpCode::LoadSP: {
                    const auto val = context.pop(sp, lib::tag<std::uint32_t>);
                    sp = val;
                    ip += 1;
                }
                break;
            case OpCode::Nop:
                ip += 1;
                break;
            }
        }
        return context;
    }
}