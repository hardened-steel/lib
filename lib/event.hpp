#pragma once
#include <atomic>
#include <bitset>
#include <tuple>

#include <lib/data-structures/dl-list.hpp>
#include <lib/semaphore.hpp>
#include <lib/buffer.hpp>
#include <lib/mutex.hpp>

namespace lib {

    template<class Event>
    class Subscriber
    {
        using Handler = typename Event::Handler;
        using Message = typename Event::Message;
    private:
        Event& event;
        Handler handler;
        std::size_t count = 0;
    public:
        explicit Subscriber(Event& event) noexcept
        : event(event)
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

    template<class Event>
    Subscriber(Event& event) -> Subscriber<Event>;

    enum class EventType: std::uint8_t
    {
        Empty  = 0,
        Simple = 0b001,
        Time   = 0b010,
        System = 0b100,
    };

    template<EventType ...ETypes>
    struct Events {};

    template<EventType E>
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

    template<class EType>
    class Handler;

    template<>
    class Handler<Events<EventType::Empty, EventType::Empty, EventType::Empty>>
    {};

    class Event
    {
    public:
        constexpr static inline auto type = EventType::Simple;
        class IHandler
        {
        public:
            virtual void notify() noexcept = 0;
        };

    public:
        void emit() noexcept;
        bool poll() const noexcept;
        std::size_t subscribe(IHandler* handler) noexcept;
        std::size_t reset() noexcept;
        IHandler* handler() const noexcept;

    protected:
        void* set() noexcept;

    private:
        std::atomic<std::uintptr_t> signal {0};
    };

    class AlwaysEvent
    {
    public:
        constexpr static inline auto type = EventType::Simple;
        using IHandler = Event::IHandler;

    public:
        bool poll() const noexcept // NOLINT
        {
            return true;
        }

        std::size_t subscribe(IHandler* ihandler) noexcept // NOLINT
        {
            handler = ihandler;
            if (handler != nullptr) {
                handler->notify();
            }
            return 1;
        }

        std::size_t reset() const noexcept // NOLINT
        {
            if (handler != nullptr) {
                handler->notify();
            }
            return 1;
        }
    
    private:
        IHandler* handler = nullptr;
    };

    class NeverEvent
    {
    public:
        constexpr static inline auto type = EventType::Simple;
        using IHandler = Event::IHandler;

    public:
        bool poll() const noexcept { return false; }                              // NOLINT
        std::size_t subscribe(IHandler* /*handler*/) const noexcept { return 0; } // NOLINT
        std::size_t reset() const noexcept { return 0; }                          // NOLINT
    };

    class TimeEvent: public data_structures::DLListElement<>
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

    public:
        constexpr static inline auto type = EventType::Time;
        class IHandler: public Event::IHandler
        {
        public:
            virtual bool update_timeout(TimePoint time, TimeEvent* event) = 0;
        };

    private:
        TimePoint time_point;
        IHandler* handler = nullptr;
        bool signaled = false;
        mutable Mutex mutex;

    public:
        void emit_on(TimePoint time)
        {
            std::lock_guard lock(mutex);

            time_point = time;
            if (handler != nullptr) {
                if (handler->update_timeout(time_point, this)) {
                    if (!signaled) {
                        signaled = true;
                        handler->notify();
                    }
                }
            }
        }

        void emit()
        {
            std::lock_guard lock(mutex);

            if (!signaled && handler != nullptr) {
                signaled = true;
                handler->notify();
            }
        }

        std::size_t subscribe(IHandler* handler) noexcept
        {
            std::lock_guard lock(mutex);

            if (handler != nullptr) {
                handler->update_timeout(time_point, this);
                signaled = false;
            }
            this->handler = handler;
            return signaled ? 1 : 0;
        }

        std::size_t reset() noexcept
        {
            std::lock_guard lock(mutex);
            if (signaled) {
                signaled = false;
                return 1;
            }
            return 0;
        }

    private:
    };

    // EventType::Simple, EventType::Time, EventType::System
    template<>
    class Handler<Events<EventType::Simple, EventType::Empty, EventType::Empty>>
        : public Event::IHandler
    {
        Semaphore semaphore;

    private:
        void notify() noexcept override
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

    // EventType::Simple, EventType::Time, EventType::System
    template<EventType Simple>
    class Handler<Events<Simple, EventType::Time, EventType::Empty>>
        : public TimeEvent::IHandler
    {
        Semaphore semaphore;

        mutable std::mutex mutex;
        TimeEvent* event = nullptr;
        TimeEvent::TimePoint timeout = TimeEvent::TimePoint::max();

    private:
        void notify() noexcept override
        {
            semaphore.release();
        }

        bool update_timeout(TimeEvent::TimePoint time, TimeEvent* wakeup_event) override
        {
            std::lock_guard lock(mutex);
            if (time < timeout) {
                event = wakeup_event;
                timeout = time;
                return true;
            }
            return false;
        }

        void check_timeout() noexcept
        {
            std::lock_guard lock(mutex);
            if ((event != nullptr) && (TimeEvent::Clock::now() > timeout)) {
                event->emit();
                timeout = TimeEvent::TimePoint::max();
            }
        }

    public:
        void wait() noexcept
        {
            check_timeout();
            if (!semaphore.acquire_until(timeout)) {
                check_timeout();
            }
        }

        void wait(std::size_t count) noexcept
        {
            while ((count--) != 0U) {
                semaphore.acquire();
            }
        }
    };

    template<class ...Events>
    class EventMux
    {
    private:
        std::tuple<Events&...> events;
        std::bitset<sizeof...(Events)> reset_mask;
    public:
        constexpr static inline auto type = (Events::type | ...);
    public:
        constexpr EventMux(Events& ...events) noexcept
        : events{events...}
        {}

        std::size_t subscribe(Event::IHandler* handler) noexcept
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
        template<std::size_t ...I>
        std::size_t subscribe(Event::IHandler* handler, std::index_sequence<I...>) noexcept
        {
            return (std::get<I>(events).subscribe(handler) + ...);
        }

        template<std::size_t ...I>
        std::size_t reset(std::index_sequence<I...>) noexcept
        {
            return ((reset_mask[I] ? 0 : std::get<I>(events).reset()) + ...);
        }
    };

    template<typename... Events>
    EventMux(Events& ...events) -> EventMux<Events...>;

    /*class IEvent
    {
    public:
        using Message = std::any;

        struct BaseHandler
        {
            virtual ~BaseHandler() noexcept = default;
        };

        class IHandler
        {
            friend IEvent;
            std::unique_ptr<BaseHandler> base;
        public:
            virtual void notify(Message) noexcept = 0;
        };
        class Handler; // unbounded MPSC queue

    private:
        struct EventInterface
        {
            std::size_t (*subscribe)(void* event, IHandler* handler);
            std::size_t (*reset)(void* event) noexcept;
        };

        const EventInterface* interface = nullptr;
        void* event = nullptr;

    public:
        template<class Event>
        IEvent(Event& event) noexcept
        : event(&event)
        {
            static const EventInterface implement = {
                [](void* event, IHandler* handler) {
                    if (handler) {
                        class Handler: public BaseHandler, public Event::IHandler
                        {
                            IHandler* handler;
                        public:
                            explicit Handler(IHandler* handler) noexcept
                            : handler(handler)
                            {}

                            void notify(typename Event::Message message) noexcept
                            {
                                handler->notify(std::move(message));
                            }
                        };
                        auto base = std::make_unique<Handler>(handler);
                        auto* next = base.get();
                        handler->base = std::move(base);
                        return static_cast<Event*>(event)->subscribe(next);
                    } else {
                        return static_cast<Event*>(event)->subscribe(nullptr);
                    }
                },
                [](void* event) noexcept {
                    return static_cast<Event*>(event)->reset();
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
        std::size_t subscribe(IHandler* handler) noexcept
        {
            return interface->subscribe(event, handler);
        }
        std::size_t reset() noexcept
        {
            return interface->reset(event);
        }
    };*/
}
