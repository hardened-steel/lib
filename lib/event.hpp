#pragma once
#include <atomic>
#include <tuple>
#include <lib/semaphore.hpp>
#include <lib/buffer.hpp>

namespace lib {

    template<class ...Events>
    class EventMux;

    class IHandler
    {
    public:
        virtual void notify() noexcept = 0;
        virtual ~IHandler() noexcept {};
        virtual void wait() noexcept = 0;
    };

    class Handler: public IHandler
    {
        Semaphore semaphore;
    public:
        void notify() noexcept override;
        void wait() noexcept override;
    };

    template<class Event>
    class SubscribeGuard
    {
        Event*      event;
        IHandler* handler;
    public:
        template<class Handler>
        SubscribeGuard(Event& event, Handler& handler) noexcept
        : event(&event)
        , handler(&handler)
        {
            event.subscribe(&handler);
        }

        SubscribeGuard(SubscribeGuard&& other) noexcept
        : event(other.event)
        , handler(other.handler)
        {
            other.event = nullptr;
        }

        SubscribeGuard& operator=(SubscribeGuard&& other) noexcept
        {
            if (this != &other) {
                if (event) {
                    auto count = event->subscribe(nullptr);
                    while(count--) {
                        handler->wait();
                    }
                }
                event = other.event;
                handler = other.handler;
                other.event = nullptr;
            }
            return *this;
        }

        ~SubscribeGuard() noexcept
        {
            if (event) {
                auto count = event->subscribe(nullptr);
                while(count--) {
                    handler->wait();
                }
            }
        }
    };

    template<class Event, class Handler>
    SubscribeGuard(Event& event, Handler& handler) -> SubscribeGuard<Event>;

    class Event
    {
        std::atomic<std::uintptr_t> signal {0};
    public:
        void emit() noexcept;
        bool poll() const noexcept;
        std::size_t subscribe(IHandler* handler) noexcept;
        void reset() noexcept;
    public:
        template<class ...Events>
        using Mux = EventMux<Events...>;
    };

    struct EventInterface
    {
        std::size_t (*subscribe)(      void* event, IHandler* handler) noexcept;
        void        (*reset)    (      void* event)                    noexcept;
        bool        (*poll)     (const void* event)                    noexcept;
    };

    class IEvent
    {
        const EventInterface* interface = nullptr;
        void* event = nullptr;
    public:
        template<class Event>
        IEvent(Event& event) noexcept
        : event(&event)
        {
            static const EventInterface implement = {
                [](void* event, IHandler* handler) noexcept {
                    return static_cast<Event*>(event)->subscribe(handler);
                },
                [](void* event) noexcept {
                    static_cast<Event*>(event)->reset();
                },
                [](const void* event) noexcept {
                    return static_cast<const Event*>(event)->poll();
                }
            };
            interface = &implement;
        }
    public:
        IEvent(const IEvent&) = default;
        IEvent(IEvent&&) = default;
        IEvent& operator=(const IEvent&) = default;
        IEvent& operator=(IEvent&&) = default;
    public:
        bool poll() const noexcept
        {
            return interface->poll(event);
        }
        std::size_t subscribe(IHandler* handler) noexcept
        {
            return interface->subscribe(event, handler);
        }
        void reset() noexcept
        {
            interface->reset(event);
        }
    };

    class NeverEvent
    {
    public:
        void emit() const noexcept {}
        bool poll() const noexcept { return false; }
        std::size_t subscribe(IHandler* handler) const noexcept { return 0; }
        void reset() const noexcept { }
    };

    template<class ...Events>
    class EventMux
    {
        std::tuple<Events&...> events;
    public:
        constexpr EventMux(Events& ...events) noexcept
        : events{events...}
        {}
        ~EventMux() noexcept
        {
            subscribe(nullptr);
        }

        std::size_t subscribe(IHandler* handler) noexcept
        {
            return subscribe(handler, std::make_index_sequence<sizeof...(Events)>{});
        }
        void reset() noexcept
        {
            reset(std::make_index_sequence<sizeof...(Events)>{});
        }
        bool poll() const noexcept
        {
            return poll(std::make_index_sequence<sizeof...(Events)>{});
        }
    private:
        template<std::size_t ...I>
        std::size_t subscribe(IHandler* handler, std::index_sequence<I...>) noexcept
        {
            return (std::get<I>(events).subscribe(handler) + ...);
        }
        template<std::size_t ...I>
        void reset(std::index_sequence<I...>) noexcept
        {
            (std::get<I>(events).reset(), ...);
        }
        template<std::size_t ...I>
        bool poll(std::index_sequence<I...>) const noexcept
        {
            return (std::get<I>(events).poll() || ...);
        }
    };
    template<typename... Events>
    EventMux(Events& ...events) -> EventMux<Events...>;
}
