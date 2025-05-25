#include <lib/event.hpp>

namespace lib {
    namespace {
        constexpr std::uintptr_t bit = 1;
    }

    void* Event::set() noexcept
    {
        if (!poll()) {
            if (auto handler = signal.fetch_or(bit); handler & ~bit) {
                return reinterpret_cast<void*>(handler); // NOLINT
            }
        }
        return nullptr;
    }

    void Event::emit() noexcept
    {
        if (auto* handler = set()) {
            std::launder(static_cast<IHandler*>(handler))->notify();
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

    Event::IHandler* Event::handler() const noexcept
    {
        if (auto handler = signal.load(std::memory_order_relaxed); handler & ~bit) {
            return std::launder(reinterpret_cast<IHandler*>(handler)); // NOLINT
        }
        return nullptr;
    }
}
