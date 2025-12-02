#pragma once
#include <lib/test.hpp>
#include <lib/semaphore.hpp>
#include <lib/buffer.hpp>
#include <lib/mutex.hpp>

#include <atomic>
#include <bitset>
#include <tuple>


namespace lib {

    enum class EventType: std::uint8_t
    {
        Never  = 0b000,
        Simple = 0b001,
        Time   = 0b010,
        System = 0b100,
    };

    template <EventType E1, EventType E2, EventType E3>
    struct Events {};

    template <EventType E>
    using MakeEvents = Events<
        // EventType::Simple, EventType::Time, EventType::System
        static_cast<EventType>(static_cast<std::uint8_t>(E) & static_cast<std::uint8_t>(EventType::Simple)),
        static_cast<EventType>(static_cast<std::uint8_t>(E) & static_cast<std::uint8_t>(EventType::Time)),
        static_cast<EventType>(static_cast<std::uint8_t>(E) & static_cast<std::uint8_t>(EventType::System))
    >;

    constexpr auto operator | (EventType lhs, EventType rhs) noexcept
    {
        using Type = std::underlying_type_t<EventType>;
        return static_cast<EventType>(static_cast<Type>(lhs) | static_cast<Type>(rhs));
    }

    template <class EType>
    class Handler;

    template <>
    class Handler<Events<EventType::Never, EventType::Never, EventType::Never>>
    {};

    template <class Event>
    class Subscriber
    {
        Event& event;
        Handler<MakeEvents<Event::type>> handler;
        std::size_t count = 0;

    public:
        template <class ...TArgs>
        explicit Subscriber(Event& event, TArgs&& ...args) noexcept
        : event(event)
        , handler(std::forward<TArgs>(args)...)
        {
            event.subscribe(&handler);
        }

        void reset() noexcept
        {
            count += event.reset();
        }

        void wait() noexcept
        {
            count -= 1;
            return handler.wait();
        }

        ~Subscriber() noexcept
        {
            handler.wait(event.subscribe(nullptr) + count);
        }
    };

    template <class Event>
    Subscriber(Event& event) -> Subscriber<Event>;


    class IEvent
    {
    public:
        constexpr static inline auto type = EventType::Simple;
        class IHandler
        {
        public:
            virtual void notify() noexcept = 0;
        };

    public:
        virtual void emit() noexcept {};
        virtual std::size_t subscribe(IHandler* handler) noexcept = 0;
        virtual std::size_t reset() noexcept = 0;
    };

    class Event: public IEvent
    {
    public:
        void emit() noexcept final;
        std::size_t subscribe(IHandler* handler) noexcept final;
        std::size_t reset() noexcept final;

    public:
        bool poll() const noexcept;
        IHandler* handler() const noexcept;

    protected:
        void* set() noexcept;

    private:
        std::atomic<std::uintptr_t> signal {0};
    };

    unittest {
        Event event;

        check(!event.poll());
        event.emit();
        check(event.poll());
        event.reset();
        check(!event.poll());
    }

    class AlwaysEvent: public IEvent
    {
    public:
        bool poll() const noexcept // NOLINT
        {
            return true;
        }

        std::size_t subscribe(IHandler* ihandler) noexcept final // NOLINT
        {
            handler = ihandler;
            if (handler != nullptr) {
                handler->notify();
            }
            return 1;
        }

        std::size_t reset() noexcept final // NOLINT
        {
            if (handler != nullptr) {
                handler->notify();
            }
            return 1;
        }

    private:
        IHandler* handler = nullptr;
    };

    class NeverEvent: public IEvent
    {
    public:
        bool poll() const noexcept { return false; }                              // NOLINT
        std::size_t subscribe(IHandler* /*handler*/) noexcept final { return 0; } // NOLINT
        std::size_t reset() noexcept final { return 0; }                          // NOLINT
    };


    class ITimeEvent
    {
    public:
        using Chrono = std::chrono::system_clock;
        using TimePoint = Chrono::time_point;

    public:
        constexpr static inline auto type = EventType::Time;
        class IHandler: public IEvent::IHandler
        {
        public:
            virtual void update_timeout(TimePoint time, ITimeEvent* event) = 0;
        };

        class IClock
        {
        public:
            virtual TimePoint now() noexcept = 0;
        };

        class Clock: public IClock
        {
            TimePoint now() noexcept final
            {
                return std::chrono::system_clock::now();
            }
        };

        virtual void timeout(TimePoint time) noexcept = 0;
        virtual std::size_t subscribe(IHandler* handler) noexcept = 0;
        virtual std::size_t reset() noexcept = 0;
    };

    class TimeEvent: public ITimeEvent
    {
        bool signaled = false;
        TimePoint time_point = TimePoint::max();
        IHandler* handler = nullptr;
        mutable Mutex mutex;

    private:
        void timeout(TimePoint time) noexcept final
        {
            if (!signaled && handler != nullptr && time >= time_point) {
                signaled = true;
                handler->notify();
            }
        }

    public:
        void emit_on(TimePoint time)
        {
            std::lock_guard lock(mutex);

            time_point = time;
            if (handler != nullptr) {
                handler->update_timeout(time_point, this);
            }
        }

        bool poll() const noexcept
        {
            std::lock_guard lock(mutex);
            return signaled;
        }

        std::size_t subscribe(IHandler* handler) noexcept final
        {
            std::lock_guard lock(mutex);

            this->handler = handler;
            if (handler != nullptr) {
                handler->update_timeout(time_point, this);
            }
            return signaled ? 1 : 0;
        }

        std::size_t reset() noexcept final
        {
            std::lock_guard lock(mutex);
            if (signaled) {
                signaled = false;
                return 1;
            }
            return 0;
        }
    };


    template <>
    class Handler<Events<EventType::Simple, EventType::Never, EventType::Never>>
        : public Event::IHandler
    {
        Semaphore semaphore;

    private:
        void notify() noexcept final
        {
            semaphore.release();
        }

    public:
        void wait() noexcept
        {
            wait(1);
        }

        void wait(std::size_t count) noexcept
        {
            while (count-- > 0) {
                semaphore.acquire();
            }
        }
    };

    template <EventType Simple>
    class Handler<Events<Simple, EventType::Time, EventType::Never>>
        : public TimeEvent::IHandler
    {
    private:
        mutable Semaphore semaphore;
        mutable Mutex mutex;
    
        TimeEvent::IClock& clock;
        ITimeEvent* event = nullptr;
        TimeEvent::TimePoint timeout = TimeEvent::TimePoint::max();

    private:
        void notify() noexcept override
        {
            semaphore.release();
        }

        void update_timeout(TimeEvent::TimePoint time, ITimeEvent* wakeup_event) final
        {
            std::lock_guard lock(mutex);
            if (time < timeout) {
                event = wakeup_event;
                timeout = time;
            }
        }

        void check_timeout() noexcept
        {
            std::lock_guard lock(mutex);
            const auto now = clock.now();
            if ((event != nullptr) && (now > timeout)) {
                event->timeout(now);
                timeout = TimeEvent::TimePoint::max();
            }
        }

    public:
        explicit Handler(TimeEvent::IClock& clock) noexcept
        : clock(clock)
        {}

        void wait() noexcept
        {
            do {
                check_timeout();
            } while (!semaphore.acquire_until(timeout));
        }

        void wait(std::size_t count) noexcept
        {
            while ((count--) != 0U) {
                semaphore.acquire();
            }
        }
    };

    unittest {
        struct TestClock: public TimeEvent::IClock
        {
            using TimePoint = TimeEvent::TimePoint;

            TimePoint time = TimePoint::max();

            TimePoint now() noexcept final
            {
                return time;
            }
        };

        using namespace std::chrono_literals;

        TestClock clock;
        TimeEvent event;
        Subscriber subscriber(event, clock);

        const auto now = TimeEvent::Chrono::now();
        event.emit_on(now + 1s);
        clock.time = now + 11s;
        subscriber.wait();
    }


    template <class ...Events>
    class EventMux
    {
    private:
        std::tuple<Events&...> events;
        std::bitset<sizeof...(Events)> reset_mask;

    public:
        constexpr static inline auto type = (Events::type | ...);
        using IHandler = Handler<MakeEvents<type>>;

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

        void set_reset_mask(std::size_t index, bool value) noexcept
        {
            reset_mask[index] = value;
        }

    private:
        template <std::size_t ...I>
        std::size_t subscribe(IHandler* handler, std::index_sequence<I...>) noexcept
        {
            return (std::get<I>(events).subscribe(handler) + ...);
        }

        template <std::size_t ...I>
        std::size_t reset(std::index_sequence<I...>) noexcept
        {
            return ((reset_mask[I] ? 0 : std::get<I>(events).reset()) + ...);
        }
    };

    template <typename... Events>
    EventMux(Events& ...events) -> EventMux<Events...>;

    unittest {
        Event event_a;
        Event event_b;
        Event event_c;
        Event event_d;

        EventMux mux {event_a, event_b, event_c, event_d};
        Subscriber subscriber(mux);

        event_d.emit();
        subscriber.wait();
        subscriber.reset();

        event_b.emit();
        subscriber.wait();
        subscriber.reset();

        event_c.emit();
        subscriber.wait();
        subscriber.reset();

        event_d.emit();
        subscriber.wait();
        subscriber.reset();
    }
}
