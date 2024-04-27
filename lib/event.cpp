#include <lib/event.hpp>
#include <climits>

namespace lib {
    void Handler::notify() noexcept
    {
        semaphore.release();
    }
    void Handler::wait(std::size_t count) noexcept
    {
        semaphore.acquire(count);
    }

    namespace {
        constexpr std::uintptr_t bit = (std::uintptr_t(1) << (sizeof(std::uintptr_t) * CHAR_BIT - 1));
    }
    void Event::emit() noexcept
    {
        if (!poll()) {
            if (auto handler = signal.fetch_or(bit); handler & ~bit) {
                reinterpret_cast<IHandler*>(handler)->notify(); // NOLINT
            }
        }
    }
    bool Event::poll() const noexcept
    {
        return (signal.load(std::memory_order_relaxed) & bit) != 0U;
    }
    std::size_t Event::subscribe(IHandler* handler) noexcept
    {
        if ((signal.exchange(reinterpret_cast<std::intptr_t>(handler)) & bit) != 0U) {
            return 1;
        }
        return 0;
    }
    std::size_t Event::reset() noexcept
    {
        if ((signal.fetch_and(~bit) & bit) != 0U) {
            return 1;
        }
        return 0;
    }
}
