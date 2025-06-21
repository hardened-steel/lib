#pragma once
#include <lib/lock.guard.hpp>
#include <lib/condition.variable.hpp>
#include <cstddef>
#include <limits>


namespace lib {

    class Semaphore
    {
        ConditionVariable gcv;
        ConditionVariable tcv;
        std::mutex mutex;

        std::size_t sem;
        std::size_t max;

    public:
        Semaphore(std::size_t init = 0, std::size_t max = std::numeric_limits<std::size_t>::max()) noexcept
        : sem(init), max(max)
        {}

    public:
        void release() noexcept
        {
            lib::LockGuard locker(mutex);
            tcv.wait(locker, [this] { return sem < max; });
            sem += 1;
            locker.unlock();
            gcv.notify_one();
        }

        void acquire() noexcept
        {
            lib::LockGuard locker(mutex);
            gcv.wait(locker, [this] { return sem > 0; });
            sem -= 1;
            locker.unlock();
            tcv.notify_one();
        }

        bool try_acquire() noexcept
        {
            lib::LockGuard locker(mutex);
            if(sem > 0) {
                sem -= 1;
                locker.unlock();
                tcv.notify_one();
                return true;
            }
            return false;
        }
    };

}
