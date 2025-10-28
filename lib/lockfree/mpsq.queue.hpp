#pragma once
#include <lib/data-structures/flist.hpp>
#include <lib/typename.hpp>
#include <atomic>
#include <new>


namespace lib::lockfree {
    class MPSCQueue
    {
    public:
        using Element = data_structures::FLListElement<>;

    private:
        Element stub;
        alignas(std::hardware_destructive_interference_size) std::atomic<Element*> head {&stub};
        alignas(std::hardware_destructive_interference_size) std::atomic<Element*> tail {&stub};

    public:
        MPSCQueue() noexcept = default;
        MPSCQueue(MPSCQueue&& other) = delete;
        MPSCQueue& operator=(MPSCQueue&& other) = delete;

        void Enqueue(Element* item) noexcept;
        Element* Dequeue() noexcept;
    };
}

namespace lib {
    template <>
    struct TypeName<lockfree::MPSCQueue>
    {
        constexpr static inline StaticString name = "lib::lockfree::MPSCQueue";
    };
}
