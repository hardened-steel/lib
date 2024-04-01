#pragma once
#include <lib/mutex.hpp>

namespace lib {
    template<class Mutex>
    class LockGuard
    {
        Mutex& mutex;
        bool locked = false;
    public:
        LockGuard(Mutex& mutex) noexcept
        : mutex(mutex)
        , locked(true)
        {
            mutex.lock();
        }
        void lock() noexcept
        {
            if(!locked) {
                mutex.lock();
                locked = true;
            }
        }
        void unlock() noexcept
        {
            if(locked) {
                mutex.unlock();
                locked = false;
            }
        }
        ~LockGuard() noexcept
        {
            if(locked) {
                mutex.unlock();
            }
        }
    };
}
