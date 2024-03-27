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
        virtual void wait(std::size_t count = 1) noexcept = 0;
    };

    class Handler: public IHandler
    {
        Semaphore semaphore;
    public:
        Handler() = default;
        Handler(const Handler&) = delete;
        void notify() noexcept override;
        void wait(std::size_t count = 1) noexcept override;
    };

    template<class Event>
    class Subscriber
    {
        Event*      event;
        IHandler* handler;
        std::size_t count;
    public:
        template<class Handler>
        Subscriber(Event& event, Handler& handler) noexcept
        : event(&event)
        , handler(&handler)
        , count(0)
        {
            event.subscribe(&handler);
        }

        Subscriber(Subscriber&& other) noexcept
        : event(other.event)
        , handler(other.handler)
        {
            other.event = nullptr;
        }

        void reset() noexcept
        {
            count += event->reset();
        }

        void wait() noexcept
        {
            handler->wait();
            count -= 1;
        }

        ~Subscriber() noexcept
        {
            if (event) {
                handler->wait(event->subscribe(nullptr) + count);
            }
        }
    };

    template<class Event, class Handler>
    Subscriber(Event& event, Handler& handler) -> Subscriber<Event>;

    class Event
    {
        std::atomic<std::uintptr_t> signal {0};
    public:
        void emit() noexcept;
        bool poll() const noexcept;
        std::size_t subscribe(IHandler* handler) noexcept;
        std::size_t reset() noexcept;
    public:
        template<class ...Events>
        using Mux = EventMux<Events...>;
    };

    struct EventInterface
    {
        std::size_t (*subscribe)(      void* event, IHandler* handler) noexcept;
        std::size_t (*reset)    (      void* event)                    noexcept;
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
                    return static_cast<Event*>(event)->reset();
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
        std::size_t reset() noexcept
        {
            return interface->reset(event);
        }
    };

    class NeverEvent
    {
    public:
        void emit() const noexcept {}
        bool poll() const noexcept { return false; }
        std::size_t subscribe(IHandler* handler) const noexcept { return 0; }
        std::size_t reset() const noexcept { return 0; }
    };

    template<class ...Events>
    class EventMux
    {
        std::tuple<Events&...> events;
    public:
        constexpr EventMux(Events& ...events) noexcept
        : events{events...}
        {}

        std::size_t subscribe(IHandler* handler) noexcept
        {
            return subscribe(handler, std::make_index_sequence<sizeof...(Events)>{});
        }
        std::size_t reset() noexcept
        {
            return reset(std::make_index_sequence<sizeof...(Events)>{});
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
        std::size_t reset(std::index_sequence<I...>) noexcept
        {
            return (std::get<I>(events).reset() + ...);
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
