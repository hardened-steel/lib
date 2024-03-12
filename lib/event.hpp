#pragma once
#include <atomic>
#include <array>
#include <lib/semaphore.hpp>
#include <lib/buffer.hpp>

namespace lib {

    template<std::size_t N>
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

    class Event
    {
        std::atomic<std::uintptr_t> signal {0};
    public:
        void emit() noexcept;
        bool poll() const noexcept;
        void subscribe(IHandler* handler) noexcept;
        void reset() noexcept;
    public:
        template<std::size_t N>
        using Mux = EventMux<N>;
    };

    struct EventInterface
    {
        void (*subscribe)(      void* event, IHandler* handler) noexcept;
        void (*reset)    (      void* event)                    noexcept;
        bool (*poll)     (const void* event)                    noexcept;
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
                    static_cast<Event*>(event)->subscribe(handler);
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
        void subscribe(IHandler* handler) noexcept
        {
            interface->subscribe(event, handler);
        }
        void reset() noexcept
        {
            interface->reset(event);
        }
    };

    class NeverEvent
    {
    public:
        void emit() noexcept {}
        bool poll() const noexcept { return false; }
        void subscribe(IHandler* handler) noexcept { }
        void reset() noexcept { }
    };

    class BEventMux
    {
        std::size_t               counter;
        lib::buffer::View<IEvent> events;
    private:
        template<std::size_t ...I>
        constexpr BEventMux(IEvent* events, std::index_sequence<I...>) noexcept
        : counter(0), events(events, sizeof...(I))
        {}
    public:
        template<std::size_t N>
        constexpr BEventMux(IEvent (&events)[N]) noexcept
        : BEventMux(events, std::make_index_sequence<N>{})
        {}
        template<std::size_t N>
        constexpr BEventMux(std::array<IEvent, N>& events) noexcept
        : BEventMux(events.data(), std::make_index_sequence<N>{})
        {}
        ~BEventMux() noexcept;

        void subscribe(IHandler* handler) noexcept;
        void reset() noexcept;
        bool poll() const noexcept;
    };

    template<std::size_t N>
    class EventMux: std::array<IEvent, N>, public BEventMux
    {
    public:
        template<class ...Events>
        constexpr EventMux(Events&& ...events) noexcept
        : std::array<IEvent, N>{std::forward<Events>(events)...}, BEventMux(static_cast<std::array<IEvent, N>&>(*this))
        {}
    };
    template<typename... Events>
    EventMux(Events&& ...events) -> EventMux<sizeof...(Events)>;
}
