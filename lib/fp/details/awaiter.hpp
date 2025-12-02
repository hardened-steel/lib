#pragma once
#include <lib/fp/executor.hpp>
#include <lib/semaphore.hpp>


namespace lib::fp::details {
    class Awaiter: public IAwaiter
    {
        mutable Semaphore semaphore;

    public:
        void wakeup(IExecutor*) noexcept final
        {
            semaphore.release();
        }

        void wait() noexcept
        {
            semaphore.acquire();
        }

        void wait(std::size_t count) noexcept
        {
            while (count-- > 0) {
                semaphore.acquire();
            }
        }
    };
}
