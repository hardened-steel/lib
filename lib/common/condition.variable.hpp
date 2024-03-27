#pragma once
#include <condition_variable>
#include <utility>
#include "mutex.hpp"

namespace lib {
    
    class ConditionVariable
    {
        std::condition_variable_any cv;
    public:
        void notify_one() noexcept
        {
            cv.notify_one();
        }
        void notify_all() noexcept
        {
            cv.notify_all();
        }
        void wait(lib::LockGuard<Mutex>& lock) noexcept
        {
            cv.wait(lock);
        }

        template<class Predicate>
        void wait(lib::LockGuard<Mutex>& lock, Predicate&& stop_waiting) noexcept
        {
            cv.wait(lock, std::forward<Predicate>(stop_waiting));
        }
    };

}
