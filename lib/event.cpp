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
    void Event::subscribe(IHandler* handler) noexcept
    {
        signal.exchange(reinterpret_cast<std::intptr_t>(handler));
    }
    void Event::reset() noexcept
    {
        signal.fetch_and(~bit);
    }

    BEventMux::~BEventMux() noexcept
    {
        for(auto& event: events) {
            event.subscribe(nullptr);
        }
    }
    void BEventMux::subscribe(IHandler* handler) noexcept
    {
        for(auto& event: events) {
            event.subscribe(handler);
        }
    }
    void BEventMux::reset() noexcept
    {
        for(auto& event: events) {
            event.reset();
        }
    }
    bool BEventMux::poll() const noexcept
    {
        for(auto& event: events) {
            if(event.poll()) {
                return true;
            }
        }
        return false;
    }
}
