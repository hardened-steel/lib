#pragma once
#include <cstddef>
#include <limits>
#include <mutex>
#include <condition_variable>

namespace lib {

    class Semaphore
    {
        mutable std::condition_variable cv;
        mutable std::mutex mutex;

        std::size_t sem;
        std::size_t max;

        constexpr static inline auto maxsem = std::numeric_limits<std::size_t>::max();
    public:
        explicit Semaphore(std::size_t init = 0, std::size_t max = maxsem) noexcept
        : sem(init), max(max)
        {}
    public:
        void release() noexcept
        {
            std::unique_lock locker(mutex);
            cv.wait(locker, [this] { return sem < max; });
            sem += 1;
            locker.unlock();
            cv.notify_one();
        }
        void acquire() noexcept
        {
            std::unique_lock locker(mutex);
            cv.wait(locker, [this] { return sem > 0; });
            sem -= 1;
            locker.unlock();
            cv.notify_one();
        }
        bool try_acquire() noexcept
        {
            std::unique_lock locker(mutex);
            if(sem > 0) {
                sem -= 1;
                locker.unlock();
                cv.notify_one();
                return true;
            }
            return false;
        }
    };
}
