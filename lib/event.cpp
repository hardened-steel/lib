#include <lib/event.hpp>
#include <climits>

namespace lib {
    void Handler::notify() noexcept
    {
        semaphore.release();
    }
    void Handler::wait() noexcept
    {
        semaphore.acquire();
    }

    namespace {
        constexpr std::uintptr_t bit = (std::uintptr_t(1) << (sizeof(std::uintptr_t) * CHAR_BIT - 1));
    }
    void Event::emit() noexcept
    {
        if (!poll()) {
            if(auto handler = signal.fetch_or(bit)) {
                reinterpret_cast<IHandler*>(handler)->notify();
            }
        }
    }
    bool Event::poll() const noexcept
    {
        return signal.load(std::memory_order_relaxed) & bit;
    }
    std::size_t Event::subscribe(IHandler* handler) noexcept
    {
        if (signal.exchange(reinterpret_cast<std::intptr_t>(handler)) & bit) {
            return 1;
        }
        return 0;
    }
    void Event::reset() noexcept
    {
        signal.fetch_and(~bit);
    }
}
