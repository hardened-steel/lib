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
                reinterpret_cast<IHandler*>(handler)->notify();
            }
        }
    }
    bool Event::poll() const noexcept
    {
        return signal.load() & bit;
    }
    std::size_t Event::subscribe(IHandler* handler) noexcept
    {
        if (signal.exchange(reinterpret_cast<std::intptr_t>(handler)) & bit) {
            return 1;
        }
        return 0;
    }
    std::size_t Event::reset() noexcept
    {
        if (signal.fetch_and(~bit) & bit) {
            return 1;
        }
        return 0;
    }
}
