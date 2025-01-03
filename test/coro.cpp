#include <gtest/gtest.h>
#include <future>
#include <lib/buffered.channel.hpp>
#include <lib/aggregate.channel.hpp>
#include <lib/broadcast.channel.hpp>
#include <lib/typename.hpp>
#include <lib/coro.hpp>
#include <lib/data-structures/dl-list.hpp>

template<class Channel>
struct IChannelAwaiter
{
    using Type = typename Channel::Type;
    Channel* channel;

    [[nodiscard]] bool await_ready() const noexcept
    {
        return channel->rpoll();
    }
    Type await_resume()
    {
        return channel->urecv();
    }
    void await_suspend(std::coroutine_handle<> handle)
    {

    }
};

template<class Channel>
auto operator co_await (Channel& channel) noexcept
{
    return IChannelAwaiter{&channel};
}

template<class T>
class ChannelGenerator: public lib::IChannel<ChannelGenerator<T>>
{
public:
    class promise_type; // NOLINT
    using Handle = std::coroutine_handle<promise_type>;

    class promise_type
    {
        friend ChannelGenerator<T>;
        std::optional<T> value;
    public:
        ChannelGenerator get_return_object() 
        {
            return ChannelGenerator(Handle::from_promise(*this));
        }
        auto initial_suspend() const noexcept
        {
            return std::suspend_always{};
        }

        auto final_suspend() noexcept
        {
            return std::suspend_always{};
        }

        template<class TArg>
        auto yield_value(TArg&& arg)
        {
            value.emplace(std::forward<TArg>(arg));
            return std::suspend_always{};
        }
        template<class TArg>
        void return_value(TArg&& arg)
        {
            value.emplace(std::forward<TArg>(arg));
        }
        void unhandled_exception()
        {
        }
    };
public:
    using Type = T;
    using REvent = lib::NeverEvent;

    bool rpoll() const noexcept
    {
        return !handle.done();
    }
    bool closed() const noexcept
    {
        return handle.done();
    }
    void close() noexcept
    {
        if (handle) {
            handle.destroy();
            handle = nullptr;
        }
    }
    REvent& revent() const noexcept
    {
        return event;
    }
    Type urecv()
    {
        handle.resume();
        return std::move(*handle.promise().value);
    }
private:
    Handle handle;
    mutable REvent event;
public:
    explicit ChannelGenerator(Handle handle) noexcept
    : handle(handle)
    {}
    ~ChannelGenerator() noexcept
    {
        close();
    }
};

ChannelGenerator<int> generate(int value)
{
    co_yield 1 + value;
    co_yield 2 + value;
    co_return value + 3;
}

ChannelGenerator<int> generate2(int value)
{
    co_yield 1 + value;
    co_yield 2 + value;
    co_yield 3 + value;
    co_yield 4 + value;
    co_return value + 5;
}

TEST(lib, coro)
{
    auto channel1 = generate(10);
    auto channel2 = generate2(20);
    lib::AggregateChannel channel (channel1, channel2);
    for (const auto& value: lib::irange(channel)) {
        std::cout << value << std::endl;
    }
    std::cout << lib::type_name<decltype(&generate2)> << std::endl;
}

lib::Task<int> go()
{
    co_return 1;
}

lib::Task<void> foo()
{
    int value = co_await go();
    std::cout << "value: " << value << std::endl;
}

TEST(lib, coro_task)
{
    lib::Scheduler scheduler;
    scheduler += foo();
    scheduler.run();
}

template<class Channel>
lib::Task<void> read_all(Channel& channel)
{
    for (const auto& value: lib::irange(channel)) {
        std::cout << value << std::endl;
    }
    co_return;
}

TEST(lib, coro_channel)
{
    lib::Scheduler scheduler;
    lib::BufferedChannel<int, 1> channel;

    scheduler += read_all(channel);

    auto worker = std::async(
        [](lib::VOChannel<int> out_channel) {
            out_channel.send(4);
            out_channel.send(5);
            out_channel.send(6);
            out_channel.close();
        },
        lib::VOChannel<int>(channel)
    );

    scheduler.run();
    worker.get();
}
