#pragma once
#include <chrono>

namespace lib {

    class Semaphore
    {
        void* handle;
    public:
        explicit Semaphore(std::size_t init = 0) noexcept;
        ~Semaphore() noexcept;

    public:
        void release(std::size_t count = 1) noexcept;
        void acquire() noexcept;
        bool try_acquire() noexcept;
        bool acquire_for(std::chrono::milliseconds rel_time) noexcept;
        bool acquire_until(std::chrono::system_clock::time_point abs_time) noexcept;
    };
}
