#include "semaphore.hpp"
#include <limits>

#define NOMINMAX
#include <Windows.h>

namespace lib {

    Semaphore::Semaphore(std::size_t init) noexcept
    : handle(
        CreateSemaphore(
            nullptr,
            static_cast<LONG>(init),
            std::numeric_limits<LONG>::max(),
            nullptr
        )
    )
    {}

    Semaphore::~Semaphore() noexcept
    {
        CloseHandle(handle);
    }

    void Semaphore::release(std::size_t count) noexcept
    {
        ReleaseSemaphore(handle, static_cast<LONG>(count), nullptr);
    }

    void Semaphore::acquire() noexcept
    {
        WaitForSingleObject(handle, INFINITE);
    }

    bool Semaphore::try_acquire() noexcept
    {
        return WaitForSingleObject(handle, 0) == WAIT_OBJECT_0;
    }

    bool Semaphore::acquire_for(std::chrono::milliseconds rel_time) noexcept
    {
        if (rel_time < std::chrono::milliseconds::zero()) {
            return try_acquire();
        }
        return WaitForSingleObject(handle, static_cast<DWORD>(rel_time.count())) == WAIT_OBJECT_0;
    }

    bool Semaphore::acquire_until(std::chrono::system_clock::time_point abs_time) noexcept
    {
        const auto now = std::chrono::system_clock::now();
        if (abs_time < now) {
            return try_acquire();
        }
        return acquire_for(std::chrono::duration_cast<std::chrono::milliseconds>(abs_time - now));
    }

}
