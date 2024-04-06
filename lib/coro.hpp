#pragma once
#include <lib/units.hpp>
#include <lib/units/si/time.hpp>
#include <coroutine>
#include <list>

namespace lib {
    template<class T>
    class Task;

    class Scheduler;

    class EventAwaiter
    {
    public:
        virtual bool ready() const noexcept = 0;
        virtual std::coroutine_handle<> coro() const noexcept = 0;
        virtual lib::IEvent& event() const noexcept = 0;
    };

    template<>
    class EventMux<std::list<EventAwaiter*>>
    {
        const std::list<EventAwaiter*>& awaiters;
    public:
        constexpr explicit EventMux(const std::list<EventAwaiter*>& awaiters) noexcept
        : awaiters(awaiters)
        {}

        std::size_t subscribe(IHandler* handler) noexcept
        {
            std::size_t count = 0;
            for (auto& awaiter: awaiters) {
                count += awaiter->event().subscribe(handler);
            }
            return count;
        }
        std::size_t reset() noexcept
        {
            std::size_t count = 0;
            for (auto& awaiter: awaiters) {
                count += awaiter->event().reset();
            }
            return count;
        }
        const EventAwaiter* poll() const noexcept
        {
            for (auto& awaiter: awaiters) {
                if (awaiter->event().poll()) {
                    return awaiter;
                }
            }
            return nullptr;
        }
    };

    class BasePromise
    {
        friend Task;
        friend Scheduler;

        Scheduler* scheduler = nullptr;
        std::coroutine_handle<> prev = nullptr;
    public:
        auto initial_suspend() const noexcept
        {
            return std::suspend_always{};
        }

        class Awaiter: public std::suspend_always
        {
        public:
            std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept;
        public:
            Awaiter(Scheduler* scheduler, std::coroutine_handle<> prev) noexcept
            : scheduler(scheduler), prev(prev)
            {}
        private:
            Scheduler* scheduler;
            std::coroutine_handle<> prev;
        };

        auto final_suspend() const noexcept
        {
            return Awaiter(scheduler, prev);
        }

        template<class Channel>
        class ChannelAwaiter: public EventAwaiter
        {
        public:
            ChannelAwaiter(Scheduler* scheduler, Channel& channel) noexcept
            : scheduler(scheduler), channel(channel), cevent(channel.revent())
            {}
            bool ready() const noexcept override
            {
                return channel.rpoll() || channel.closed();
            }
            std::coroutine_handle<> coro() const noexcept override
            {
                return suspended_coro;
            }
            lib::IEvent& event() const noexcept override
            {
                return cevent;
            }
        public:
            bool await_ready() const noexcept
            {
                return ready();
            }
            typename Channel::Type await_resume() const
            {
                if (!channel.rpoll()) {
                    throw std::out_of_range("channel is closed");
                }
                return channel.urecv();
            }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<> coro) noexcept;
        private:
            Scheduler*  scheduler;
            Channel&    channel;
            mutable lib::IEvent cevent;
            std::coroutine_handle<> suspended_coro = nullptr;
        };

        template<class Channel>
        auto await_transform(AsyncRecv<Channel> recv) noexcept
        {
            return ChannelAwaiter<Channel>(scheduler, recv.channel);
        }

        template<typename Awaiter>
        auto&& await_transform(Awaiter&& awaitable) noexcept
        {
            return std::forward<Awaiter>(awaitable);
        }
    };

    template<class T>
    class Promise: public BasePromise
    {
        friend Scheduler;

        std::optional<T> value;
    public:
        auto get_return_object()
        {
            return Task<T>(std::coroutine_handle<Promise>::from_promise(*this));
        }

        template<class TArg>
        void return_value(TArg&& arg)
        {
            value.emplace(std::forward<TArg>(arg));
        }
        void unhandled_exception()
        {
        }

        T get_value()
        {
            return std::move(*value);
        }
    };

    template<class T>
    class Task
    {
        friend Scheduler;
    public:
        using promise_type = Promise<T>;
        using CoroHandle = std::coroutine_handle<Promise<T>>;
        using Type = T;
    private:
        CoroHandle coro;
    public:
        constexpr explicit Task(CoroHandle coro) noexcept
        : coro(coro)
        {}

        Task(Task&& other) noexcept
        : coro(other.coro)
        {
            other.coro = nullptr;
        }

        ~Task() noexcept
        {
            if (coro) {
                coro.destroy();
            }
        }
    public:
        CoroHandle release() noexcept
        {
            CoroHandle coro = this->coro;
            this->coro = nullptr;
            return coro;
        }

        class Awaiter: public std::suspend_always
        {
        public:
            T await_resume() const noexcept
            {
                return next.promise().get_value();
            }
            template<class Promise>
            auto await_suspend(std::coroutine_handle<Promise> prev) const noexcept
            {
                auto& next_promise = next.promise();
                auto& prev_promise = prev.promise();
                auto* scheduler = prev_promise.scheduler;

                next_promise.prev = prev;
                next_promise.scheduler = scheduler;

                scheduler->ready(next);
                return scheduler->next();
            }
        public:
            explicit Awaiter(std::coroutine_handle<Promise<T>> next) noexcept
            : next(next)
            {}
        private:
            std::coroutine_handle<promise_type> next;
        };

        auto operator co_await()
        {
            return Awaiter(coro);
        }
    };

    template<>
    class Promise<void>: public BasePromise
    {
        friend Scheduler;
    public:
        auto get_return_object()
        {
            return Task<void>(std::coroutine_handle<Promise>::from_promise(*this));
        }

        constexpr void return_void() const noexcept
        {}

        void unhandled_exception()
        {
        }

        constexpr void get_value() const noexcept
        {}
    };

    class Scheduler
    {
        std::list<std::coroutine_handle<>> ready_tasks;
        std::list<EventAwaiter*> block_list;
        std::list<std::coroutine_handle<>> gc_tasks;

        static Task<void> garbage_collector(Scheduler& scheduler)
        {
            auto& tasks = scheduler.gc_tasks;
            while (co_await scheduler) {
                for (auto it = tasks.begin(); it != tasks.end();) {
                    if (it->done()) {
                        it->destroy();
                        it = tasks.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }

        const Task<void> gc = garbage_collector(*this);

        auto operator co_await()
        {
            class Awaiter: public std::suspend_always
            {
            public:
                bool await_resume() const noexcept
                {
                    return true;
                }
            };
            return Awaiter{};
        }
    public:
        template<class T>
        void bind(Task<T> task)
        {
            auto coro = task.release();
            coro.promise().scheduler = this;
            coro.promise().prev = gc.coro;
            ready_tasks.push_back(coro);
            gc_tasks.push_back(coro);
        }

        void ready(std::coroutine_handle<> coro)
        {
            ready_tasks.push_back(coro);
        }

        void wait(EventAwaiter& awaiter)
        {
            block_list.push_back(&awaiter);
        }

        std::coroutine_handle<> next()
        {
            if (!ready_tasks.empty()) {
                auto coro = ready_tasks.front();
                ready_tasks.pop_front();
                return coro;
            }
            return std::noop_coroutine();
        }

        template<class T>
        void operator += (Task<T> task)
        {
            bind(std::move(task));
        }

        bool check_blocks()
        {
            bool result = false;
            for (auto it = block_list.begin(); it != block_list.end();) {
                const auto* awaiter = *it;
                if (awaiter->ready()) {
                    ready(awaiter->coro());
                    result = true;
                    it = block_list.erase(it);
                } else {
                    ++it;
                }
            }
            return result;
        }

        void run()
        {
            Handler handler;
            EventMux<std::list<EventAwaiter*>> blocks(block_list);
            
            while (!ready_tasks.empty() || !block_list.empty()) {
                if (!ready_tasks.empty()) {
                    auto coro = ready_tasks.front();
                    ready_tasks.pop_front();
                    coro.resume();
                }
                if (!block_list.empty()) {
                    Subscriber subscriber(blocks, handler);
                    subscriber.reset();
                    while (!check_blocks()) {
                        subscriber.wait();
                        subscriber.reset();
                    }
                }
            }
        }
    };

    std::coroutine_handle<> BasePromise::Awaiter::await_suspend(std::coroutine_handle<>) const noexcept
    {
        if (prev) {
            scheduler->ready(prev);
        }
        return scheduler->next();
    }

    template<class Channel>
    std::coroutine_handle<> BasePromise::ChannelAwaiter<Channel>::await_suspend(std::coroutine_handle<> coro) noexcept
    {
        this->suspended_coro = coro;
        scheduler->wait(*this);
        return scheduler->next();
    }

    template<class T, class Ratio>
    auto operator co_await(const Quantity<units::Time, T, Ratio>& duration) noexcept
    {
        return std::suspend_never{};
    }
}
