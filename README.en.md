# Channels
Channels are a convenient abstraction for building applications that operate in a multithreaded environment. They are used for passing messages between threads and, simultaneously, as a means of synchronizing threads. I will refer to "Go-style channels" because, in my opinion, a significant feature of channels in the Go language is the ability to multiplex them.

Implementations of channels in C++ certainly exist, for example, in the Boost.Fiber library. You can find implementations of [two types](https://www.boost.org/doc/libs/1_84_0/libs/fiber/doc/html/fiber/synchronization/channels.html) of channels here. In the Boost.Fiber documentation, you can find descriptions of [ways to multiplex](https://www.boost.org/doc/libs/1_84_0/libs/fiber/doc/html/fiber/when_any.html), although not specifically for channels, but similar techniques can be applied to them here.

The implementation from Boost does not offer multiplexing channels "out of the box" and does not position itself as "Go-style channels," which is understandable since it provides a simple mechanism for message passing between fibers. The proposed multiplexing technique, which can be applied to channels, is a straightforward implementation. It involves launching additional intermediate ``fibers``, one per task (or in our case, per channel). However, using ``fibers`` for multiplexing is quite costly.

Another implementation, my first Google search result for "go-style channels C++," led me to this [result](https://github.com/Balnian/ChannelsCPP). The library uses overloaded operators ``<<`` and ``>>``, and it includes multiplexing, but it's implemented through polling channels in an infinite loop. However, the class go::internal::ChannelBuffer contains an error in the usage of ``std::conditional_variable`` and fields ``std::atomic_bool`` is_closed; (we'll discuss this below).

Both implementations use a cyclic buffer to store transmitted messages. I will demonstrate with examples that the abstraction of channels is more than just a cyclic buffer with synchronization primitives. I will formulate requirements for the ideal implementation of channels:
1) Multiplexing is implemented without using "heavy" entities. By "heavy" entities, I mean the following:
    * launching additional threads, coroutines (of any kind);
    * absence of any "background" or "service" threads;
    * absence of dynamic memory usage.
    Additional data structures that provide multiplexing should only be created when necessary.

2) Thread blocking occurs only when there is no data in the channel and uses standard OS facilities (no infinite loop "under the hood"). Thus, the channel can use lock-free containers, while synchronization objects are not engaged as long as there are messages in the channel.
3) Blocking lasts until data appears in any channel (simple ``round-robin`` is not suitable).
4) Support for iterator interface for reading from the channel:
    ```cpp
    Channel<int>& ch = /*...*/;
    for(const auto& value: ch) {
        /*...*/
    }
    ```
5) The channel interface allows any implementation of data retrieval. As a simple example, imagine a channel whose data is generated "on the fly":
    ```cpp
    template<class T, class Fn>
    class ChannelGenerator: public Channel<T>
    {
        Fn generator;
        /*...*/
    };
    /*...*/
    auto channel = create_generator<int>(
        [counter = 0] { return counter++; }
    );
    for(const auto& value: channel) { // infinity loop!!!
        std::cout << "value from channel: " << value << std::endl;
    }
    // print:
    // value from channel: 0
    // value from channel: 1
    // value from channel: 2
    // ....
    ```

I'll add one more constraint: by default, all channels are SPSC (single producer single consumer). This simplifies the basic implementation.

Point #2 requires further explanation. The idea is to combine [polling](https://en.wikipedia.org/wiki/Polling_(computer_science)) with blocking. Polling has the advantage of efficiency and minimizing delays, but its drawback is that when there is no data, processor time is wasted on constant polling. Combining approaches works like this: when there are messages in the channel(s), use the polling approach; when there are no messages, use blocking.

## Simple channel implementation
The simplest implementation of an MPMC (multiple producer multiple consumer) channel with a fixed-size buffer can be represented as follows:
```cpp
template<class T, std::size_t N>
class Channel {
    CyclicBuffer<T, N> buffer;
    bool closed = false;

    mutable std::mutex              mt;
    mutable std::condition_variable cv;
public:
    using value_type = T;
};
```
Example implementation of the ``recv`` method:
```cpp
bool recv(T& value)
{
    std::unique_lock lock{mt};
    cv.wait(lock, [this]{ return closed || !buffer.empty(); });
    if (closed) {
        return false;
    }
    value = buffer.pop();
    lock.unlock();
    cv.notify_all();
    return true;
}
```
The ``send`` method is implemented similarly. Now let's think about how to add the ability to multiplex. We'll ignore the requirement not to block if there are data in the channel for now. This implementation uses a pair of ``std::mutex`` and ``std::condition_variable`` for synchronization between threads.

We need a class that combines our channels:
```cpp
template<class ...Channels>
class ChannelSelect {
    std::tuple<Channels&...> channels;
public:
    ...
};
```

The next step is to work on the interface of the ``ChannelSelect`` class. Reminder: channels can transmit different types of data, so a simple interface for the ``recv`` function is not suitable for us. In reality, channels can be combined in different ways.

1) Calling a ``callback`` for each channel, as in the [ChannelsCPP](https://github.com/Balnian/ChannelsCPP) library.
    Example usage:
    ```cpp
    void fibonacci(Chan<int>& c, Chan<int>& quit)
    {
        int x=0, y = 1;
        for (bool go = true; go;) {
            Select {
                Case {
                    c << x,
                    [&] {
                        int t = x;
                        x = y;
                        y += t;
                    }
                },
                Case {
                    quit,
                    [&](auto v) {
                        cout << "quit" << endl;
                        go = false;
                    }
                }
            };
        }
    }
    ```
    In this example, ``Select`` and ``Case`` are classes from the library.

2) The ``recv`` method returns a combination of types. Two subvariants are possible:
     1) Wait for all combined channels to be ready and return ``std::tuple``.
        ```cpp
        bool recv(std::tuple<typename Channels::value_type...>& value);
        ```
        The combined channel is considered closed if at least one of its subchannels is closed (logical "AND" scheme).
     2) Wait for any of the combined channels to be ready and return ``std::variant``.
        ```cpp
        bool recv(std::variant<typename Channels::value_type...>& value);
        ```
        The combined channel is considered closed if all of its subchannels are closed (logical "OR" scheme).

    Subvariant 2 allows implementing "GO-style" classes ``Select`` and ``Case``.

Simple idea for implementing multiplexing:
1) We need to somehow notify the channel that there is someone "above" it. We cannot call the ``recv`` method because we will block. Let's add an additional field to the channel – a pointer (for example, to a function) so that the "writer" can wake up the "reader" after adding data.
2) The ``ChannelSelect`` class has its own pair of ``std::mutex`` + ``std::condition_variable``.
3) Add a ``poll`` method to the ``Channel`` class to make a decision about blocking the ``ChannelSelect`` class (if there is no data, then block):
    ```cpp
    bool poll() const
    {
        std::unique_lock lock{mt};
        return !buffer.empty();
    }
    ```

## Drawback of this approach
This architecture allows satisfying all requirements from the list except requirement #2.

### A small digression
Often, I see a mistake when using the combination of ``std::mutex`` + ``std::condition_variable`` – replacing some (or all!) fields of the class with atomic variables. Indeed, the notify method of the ``std::condition_variable`` class can be called without locking. But this does not mean that ``std::condition_variable`` can only be used to wake up another thread!
Let me illustrate the problem:
1) The "writer" placed data (one message) in the empty lock-free cyclic buffer without acquiring the mutex.
2) The "writer" called ``cv.notify_one()``. The "writer" will not provide more data for a long time (or never).
3) The "reader" acquires the mutex and checks for data in the cyclic buffer, finding nothing.
4) The "reader" falls asleep on ``cv.wait(...)``.

Let's assume the chronological order is: 3, 1, 2, 4. Therefore, the "reader" will fall asleep even if there is data in the buffer, and it can only be woken up by the "writer" when it adds the next batch of data. But if the "writer" finishes its work, providing the last data to the buffer, then the "reader" will sleep forever (deadlock), while there are still unprocessed data in the buffer.

An example of this error can be seen [here](https://github.com/Balnian/ChannelsCPP/blob/1ba5fd6b56d2983387356294e785b197c9b8e132/ChannelsCPP/ChannelBuffer.h#L84).

## Implementation based on semaphore
A semaphore is better suited for implementing a channel. The properties of a semaphore allow implementing a lock-free channel. If there is data in the channel, there is no need to block. However, we still need to use it every time we read from or write to the channel. The requirement "not to block" (when there is data in the channel) moves to the semaphore implementation. Most often, a semaphore implementation is a thin wrapper over the OS semaphore, which in turn is some kind of OS descriptor and corresponding system calls. Of course, one could rely on the assumption that the semaphore is implemented well enough and does not use OS system calls if blocking is not required. However, I decided not to use it in its pure form but to call semaphore methods only when necessary.

# Solution overview
## Event object
All we need is to notify another thread that new data has arrived (or space in the buffer is available for writing). For this purpose, the concept of events is best suited. An event object typically works through a mechanism of subscribing to an event.

We have the constraint SPSC (single producer single consumer), which means an event can only have one subscriber.

Let's represent the event object as a simple class:
```cpp
class Event
{
    std::atomic<std::uintptr_t> signal {0};
public:
    void emit() noexcept;
    bool poll() const noexcept;
    std::size_t subscribe(IHandler* handler) noexcept;
    std::size_t reset() noexcept;
};
```

The class has a single field – ``signal``, which stores a pointer to the subscriber object. Here's its interface:
```cpp
class IHandler
{
public:
    virtual void notify() noexcept = 0;
    virtual ~IHandler() noexcept {};
    virtual void wait(std::size_t count = 1) noexcept = 0;
};
```
Using ``std::uintptr_t`` is not arbitrary here. Let's define two states for the event: signaled and non-signaled. Storing the signaled state in a separate atomic field would be too wasteful; one bit is enough for this purpose. Since the most significant bits of pointers are not used, we can combine the state bit with the pointer.

The bit number is defined as follows:
```cpp
// the most significant bit of std::uintptr_t
constexpr std::uintptr_t bit = (std::uintptr_t(1) << (sizeof(std::uintptr_t) * CHAR_BIT - 1));
```

Here's what the methods of the ``Event`` class do:
* ``poll`` – checks the event, returning true if the event has occurred:
    ```cpp
    bool Event::poll() const noexcept
    {
        return signal.load(std::memory_order_relaxed) & bit;
    }
    ```
* ``emit`` – sets the event to the signaled state:
    ```cpp
    void Event::emit() noexcept
    {
        if (!poll()) {
            if(auto handler = signal.fetch_or(bit); handler & ~bit) {
                reinterpret_cast<IHandler*>(handler)->notify();
            }
        }
    }
    ```
  Here, we check if the event has not occurred yet. If so, we read the pointer while simultaneously setting the most significant bit. Then we call ``notify()`` on it. The additional check in the condition ``handler & ~bit`` allows calling emit from different threads.
* ``subscribe`` – subscribes to the event, passing ``nullptr`` unsubscribes the ``handler`` from this event:
    ```cpp
    std::size_t Event::subscribe(IHandler* handler) noexcept
    {
        if (signal.exchange(reinterpret_cast<std::intptr_t>(handler)) & bit) {
            return 1;
        }
        return 0;
    }
    ```
    This method is versatile; we can pass a valid pointer to subscribe or ``nullptr`` to unsubscribe. In the second case, the return value will be useful – it indicates the number of "events" that have already occurred (or will definitely occur in the future: ``emit`` managed to set the signaled state bit but has not yet called ``notify``) before we finally unsubscribe. If our handler consists of a semaphore (the default implementation), we need to wait for all events to occur before destroying it.
* ``reset`` – sets the event to the non-signaled state:
    ```cpp
    std::size_t Event::reset() noexcept
    {
        if (signal.fetch_and(~bit) & bit) {
            return 1;
        }
        return 0;
    }
    ```
    This method resets the event flag. Similar to the ``subscribe`` method, it returns the number of events that have already occurred (or will definitely occur in the future).

All methods are atomic and non-blocking. The ``emit`` method calls ``notify``, which in turn involves the semaphore.
An important property of the event object is that subsequent calls to ``emit`` only read the atomic variable and do nothing until the event is reset again.

The ``Handler`` class is a simple implementation based on a semaphore:
```cpp
class Handler: public IHandler
{
    Semaphore semaphore;
public:
    void notify() noexcept override
    {
        semaphore.release();
    }
    void wait(std::size_t count = 1) noexcept override;
    {
        semaphore.acquire(count);
    }
};
```

Let me justify the need for the counter. When events transition to the signaled state, the "subscriber" does not always call ``wait``, so the semaphore accumulates its internal counter. When the subscriber object is deleted, it unsubscribes from the event object. However, the ``emit`` method might still try to call ``notify``. Now the event counter comes in handy. Before the subscriber object is destroyed, it waits for all accumulated events in the semaphore. This ensures the complete safety of the ``emit`` method and the destruction of the ``Handler`` class instance.

### Conclusion
The event object works in tandem with any object that can change its state and needs to notify another thread about it. So far, it looks similar to working with a pair of ``std::mutex`` and ``std::condition_variable``.

The general algorithm for working with the event from the writer thread is:
1) Change the state (in our case, the channel).
2) Call ``emit``.
A significant difference from ``std::condition_variable`` is that capturing ``std::mutex`` is not required to change the state.

The general algorithm for working with the event from the reader thread looks like this:
1) Create a ``Handler`` object.
2) Subscribe to the event.
3) Check if the desired change has occurred:
   1) Poll the object (channel) for the desired changes.
   2) If there are no changes, call ``reset``.
   3) Poll the object again.
   4) If there are no changes, call ``wait``.
   5) Call ``reset``.
   6) Repeat step 3.
4) Unsubscribe from the event.
Step 3 can be repeated as many times as necessary. The ``Handler`` object can be reused, so we move to step 4 when all the work is done.

Step 4 is more complicated: simply calling ``subscribe(nullptr)`` is not enough; we need to consider the number of occurred events for which ``wait`` was not called. Let's write a class that simplifies working with event objects and subscribers. It will additionally protect us from the error of forgetting to unsubscribe:
```cpp
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
```
We wrapped the ``wait`` method of the ``Handler`` class and the ``reset`` method of the ``Event`` class. The ``Subscriber`` class implements the RAII idiom and encapsulates the work with the counter of occurred events.

## Event multiplexing
How do we now combine multiple event objects into one?
It's quite simple, but it should be noted that such an event type transitions to a signaled state when at least one sub-event transitions to a signaled state.

The event multiplexing class:
```cpp
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
```
All methods of the ``EventMux`` class are delegated to the controlled events. The ``poll`` method uses the logical "or" operator for the returned result. For ``reset`` and ``subscribe``, we sum up the returned values.
The only difference is the absence of the ``emit`` method; it is not needed for the EventMux class.

## Channels
Channels can be for reading, for writing, or both. Their interfaces are very similar, so I use the prefixes "r" (recv) and "s" (send) for similar methods.

The general interface for a read channel can be represented as follows:
```cpp
template<class T>
class IChannelInterface
{
public:
    using Type = T;
    using REvent = Event;
public:
    bool rpoll() const noexcept;
    Type urecv();

    void close() noexcept;
    bool closed() const noexcept;

    REvent& revent() const noexcept;
};
```
* ``rpoll`` – polls the channel for new messages. ``true`` if the channel is ready for reading, ``false`` if there is nothing in the channel.
* ``urecv`` – reads (or retrieves) the next message. The suffix "u" stands for "unsafe"; this method cannot be called unless ``rpoll`` has been called first and returned true.
* ``close`` – closes the channel. Messages remaining in the channel will still be available for reading. After calling this method, ``rpoll`` may still return ``true`` if there is unread content.
* ``closed`` – indicates whether the channel is closed.
* ``revent`` – returns a reference to the associated event object.

In addition to these methods, the interface should provide two types:
* ``Type`` – the message type. The ``urecv`` method should return an object of this type.
* ``REvent`` – the type of the associated event object. The ``revent`` method should return a reference to this type.

The interface does not use virtual functions because static polymorphism is primarily used in this implementation. If there is static polymorphism, implementing virtual functions is not significantly more difficult.

The interface for the write channel, it differs little from ``IChannelInterface``:
```cpp
template<class T>
class OChannelInterface
{
public:
    using Type = T;
    using SEvent = Event;
public:
    bool spoll() const noexcept;
    void usend(T value);

    void close() noexcept;
    bool closed() const noexcept;

    SEvent& sevent() const noexcept;
};
```

Both interfaces are "low-level" enough for direct use. You can't just call ``usend/urecv``; you need to make sure that ``spoll/rpoll`` returns true, and if not, you need to work with the event object returned by ``sevent/revent``. Usually, we just want to call something like ``channel.send(value)`` and block if there is not enough space in the channel.
To achieve this, let's create a helper class:
```cpp
template<class Channel>
class OChannel
{
public:
    template<class Event, class TChannel>
    static bool swait(Subscriber<Event>& subscriber, const TChannel& channel) noexcept
    {
        subscriber.reset();

        bool closed = channel.closed();
        bool ready = channel.spoll();

        while(!closed && !ready) {
            subscriber.wait();
            subscriber.reset();

            closed = channel.closed();
            ready = channel.spoll();
        }
        return ready;
    }

    template<class T>
    void send(T&& value)
    {
        auto& self = *static_cast<Channel*>(this);
        if (self.spoll()) {
            return self.usend(std::forward<T>(value));
        }
        Handler handler;
        Subscriber subscriber(self.sevent(), handler);

        if (swait(subscriber, self)) {
            self.usend(std::forward<T>(value));
        }
    }
};
```
The ``OChannel`` class uses ``auto& self = *static_cast<Channel*>(this);`` and inherits from the template parameter, which is named ``Channel``. This is the CRTP (Curiously Recurring Template Pattern).
Usage example:
```cpp
template<class T>
class OChannelInterface: public OChannel<OChannelInterface<T>>
{
public:
    ...
};
```
Done, now we have a convenient ``send`` method. For the reader channel, there is a similar class. Note that while the method is convenient, it's not efficient; it creates ``Handler`` and ``Subscriber`` objects on the stack every time and uses them only once. Working through iterators solves this problem.

### Range-based for loop for channels
To fulfill requirement #4, it's necessary to implement iterator classes. I decided to add a separate class ``IRange`` and a free function ``irange``. Both the class and the function are templated and work with any read channels. Here's the complete implementation:
```cpp
template<class Channel>
class IRange
{
    friend class Iterator;
private:
    using Value = std::remove_cv_t<std::remove_reference_t<typename Channel::Type>>;

    Channel& channel;
    Handler  handler;
    Subscriber<typename Channel::REvent> subscriber;

    // Storage for the temporary object
    std::aligned_storage_t<sizeof(Value), alignof(Value)> value;
public:
    class Iterator
    {
        IRange *range;
    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = typename Channel::Type;
        using pointer           = std::remove_reference_t<value_type>*;
        using reference         = std::remove_reference_t<value_type>&;
    private:
        void next()
        {
            if (!Channel::rwait(range->subscriber, range->channel)) {
                range = nullptr;
            } else {
                auto ptr = reinterpret_cast<value_type*>(&range->value);
                new(ptr) value_type(range->channel.urecv());
            }
        }
    public:
        Iterator(IRange *range) noexcept
        : range(range)
        {
            if (range) {
                next();
            }
        }
        ~Iterator() noexcept
        {
            if(range) {
                auto ptr = reinterpret_cast<value_type*>(&range->value);
                std::destroy_at(ptr);
            }
        }
        auto& operator*() const noexcept
        {
            auto ptr = reinterpret_cast<value_type*>(&range->value);
            return *ptr;
        }
        Iterator& operator++()
        {
            auto ptr = reinterpret_cast<value_type*>(&range->value);
            std::destroy_at(ptr);

            next();
            return *this;
        }
        friend bool operator==(const Iterator& a, const Iterator& b) noexcept
        {
            return a.range == b.range;
        }
        friend bool operator!=(const Iterator& a, const Iterator& b) noexcept
        {
            return a.range != b.range;
        }
    };
public:
    constexpr IRange(Channel& channel) noexcept
    : channel(channel)
    , subscriber(channel.revent(), handler)
    {}

    auto begin()
    {
        return Iterator(this);
    }
    auto end()
    {
        return Iterator(nullptr);
    }
public:
    IRange(const IRange& range) = delete;
    IRange& operator=(const IRange& range) = delete;
};

template<class Channel>
auto irange(Channel& channel)
{
    return IRange<Channel>(channel);
}
```
A few points to note:
1) The ``IRange`` class stores ``Handler`` and ``Subscriber`` objects. This means that subscribing to the event happens at the beginning, and unsubscribing only once at the end of working with the iterators.
2) Dereferencing the iterator should return a reference to the message, so the message needs to be temporarily stored somewhere. We use ``std::aligned_storage_t`` for this purpose.
3) The constructor and destructor of the message are called only at specific moments; there's no need for additional variables or using ``std::optional``.

### Multiplexing channels
Now we have everything we need to multiplex channels. As mentioned above, there are two ways to multiplex them.

#### Multiplexing channels with any channel ready
For multiplexing using the "OR" scheme, let's write a template class:
```cpp
    template<class ...Channels>
    class IChannelAny
        : public IChannel<IChannelAny<Channels...>> // CRTP
    {
    public:
        using Type = std::variant<typename Channels::Type...>;
        using REvent = EventMux<typename Channels::REvent&...>;
    private:
        std::tuple<Channels&...> channels;
        mutable REvent           events;
        mutable std::size_t      current = 0;
    public:
        constexpr IChannelAny(Channels&... channels) noexcept
        : channels{channels...}
        , events{channels.revent()...}
        {
        }
        bool rpoll() const noexcept;
        bool closed() const noexcept;
        void close() noexcept;
        REvent& revent() const noexcept
        {
            return events;
        }
        Type urecv();
    private:
        // Implementation goes here
    };

    template<typename... Channels>
    IChannelAny(Channels&... channels) -> IChannelAny<Channels...>;
```

The class stores references to the channels being combined in a ``std::tuple`` list. It has its own event object based on the ``EventMux`` class and a counter to implement ``round-robin`` behavior.
The return type is ``std::variant<typename Channels::Type...>``. All channels can have different message types. If types are repeated, they will still be represented in ``std::variant`` under their respective index.

Here's a simple implementation of the methods:
* ``close`` – calls ``close`` for each subchannel.
* ``closed`` – calls ``closed`` for each subchannel and returns ``true`` if all of them return ``true``.
* ``rpoll`` – calls ``rpoll`` for each subchannel but remembers where it left off. Calling ``rpoll`` in a loop uses an array of function pointers to erase the type:
    ```cpp
    template<std::size_t ...I>
    bool rpoll(std::index_sequence<I...>) const noexcept
    {
        using PollFn = bool (*)(const IChannelAny*);
        static const std::array<PollFn, sizeof...(Channels)> poll {
            [](const IChannelAny* channel) {
                return std::get<I>(channel->channels).rpoll();
            }...
        };

        for (std::size_t i = 0; i < sizeof...(Channels); ++i) {
            if (poll[current](this)) {
                return true;
            }
            current += 1;
            if (current == sizeof...(Channels)) {
                current = 0;
            }
        }
        return false;
    }
    ```
* ``urecv`` – calls ``urecv`` for the channel for which the ``rpoll`` function returned ``true``. Here also, an array of function pointers is used for type erasure:
    ```cpp
    template<std::size_t ...I>
    Type urecv(std::index_sequence<I...>)
    {
        using RecvFn = Type (*)(IChannelAny*);
        static const std::array<RecvFn, sizeof...(Channels)> recv {
            [](IChannelAny* channel) {
                return Type(std::in_place_index<I>, std::get<I>(channel->channels).urecv());
            }...
        };
        return recv[current](this);
    }
    ```
The ``IChannelAny`` class complies with the ``IChannelInterface`` interface. Therefore, it can be multiplexed with other channels.

#### Channel multiplexing with all channel ready
For multiplexing using the "AND" scheme, let's write a template class:
```cpp
template<class ...Channels>
class IChannelAll: public IChannel<IChannelAll<Channels...>>
{
public:
    using Type = std::tuple<typename Channels::Type...>;
    using REvent = Event::Mux<typename Channels::REvent&...>;
private:
    std::tuple<Channels&...>                 channels;
    mutable REvent                           events;
    mutable std::bitset<sizeof...(Channels)> states;
public:
    constexpr IChannelAll(Channels&... channels) noexcept
    : channels{channels...}
    , events{channels.revent()...}
    {
    }
    bool rpoll() const noexcept;
    bool closed() const noexcept;
    void close() noexcept;
    REvent& revent() const noexcept
    {
        return events;
    }
    Type urecv();
private:
    // Implementation details here...
};

template<typename... Channels>
IChannelAll(Channels&... channels) -> IChannelAll<Channels...>;
```

It's almost the same as the ``IChannelAny`` class. Instead of a counter, a ``std::bitset`` field is used for each channel per bit – a minor optimization to avoid repeated polling of channels. The return type is ``std::tuple<typename Channels::Type...>``.
* ``close`` – calls ``close`` for each subchannel. This is the same as before.
* ``closed`` – calls ``closed`` for each subchannel and returns ``true`` if at least one of them returns ``true``.
* ``rpoll`` – calls ``rpoll`` for each subchannel, and the result is stored in the bitset field:
    ```cpp
    template<std::size_t ...I>
    bool rpoll(std::index_sequence<I...>) const noexcept
    {
        if(states.all()) {
            return true;
        }
        ((states[I] = states.test(I) || std::get<I>(channels).rpoll()), ...);
        return states.all();
    }
    ```
* ``urecv`` – calls ``urecv`` for all channels and combines the results into a tuple:
    ```cpp
    template<std::size_t ...I>
    Type urecv(std::index_sequence<I...>)
    {
        states.reset();
        return std::make_tuple(std::get<I>(channels).urecv()...);
    }
    ```
The ``IChannelAll`` class complies with the ``IChannelInterface`` interface, so it can also be multiplexed with other channels.

Using ``structured binding``, reading from ``IChannelAll`` can be organized as follows:
```cpp
lib::IChannelAll channels_ab(channel_a, channel_b);
for(const auto& [a, b]: lib::irange(channels_ab)) {
    // ...
}
```

# Conclusions
Channels (and events) implement the composite pattern: multiplexed channels, in turn, can also be multiplexed and so on, everything will work with any "level of nesting". All requirements for implementation are satisfied: no dynamic memory, no additional, service (background), or intermediate threads, coroutines, etc. We can wrap a lock-free queue in a channel or create a thin wrapper over a generator function.

The architectural capabilities of the library allow implementing channels and their multiplexing in any way you like. Let me provide two examples.

## AggregateChannel
Let's assume we need to combine multiple channels using the "OR" scheme, but all channels transmit the same type of message or different types of messages, but all of them can be converted into one common type. At the same time, the "reader" doesn't care which channel the message came from. We could use a regular ``IChannelAny``, but then the resulting type would be ``std::variant``. For such combination, we can write a separate class:
```cpp
template<class T, class ...Channels>
class AggregateChannel: public IChannel<AggregateChannel<T, Channels...>>
{
public:
    using Type = T;
    using REvent = Event::Mux<typename Channels::REvent& ...>;
private:
    std::tuple<Channels&...> channels;
    mutable REvent           events;
    mutable std::size_t      current = 0;
public:
    AggregateChannel(Channels& ...channels) noexcept
    : channels{channels...}
    , events(channels.revent()...)
    , current(0)
    {}
public:
    bool rpoll() const noexcept;
    bool closed() const noexcept;
    void close() noexcept;
    REvent& revent() const noexcept;
    Type urecv();
private:
    ...
};

template<class ...Channels>
AggregateChannel(Channels& ...channels) -> AggregateChannel<std::common_type_t<typename Channels::Type ...>, Channels...>;
```
The logic of operation of this class is almost identical to the ``IChannelAny`` class, the difference lies only in the type of message created.

## BroadCastChannel
A write channel that broadcasts copies of messages to other channels.
The implementation is straightforward:
```cpp
template<class T, class ...Channels>
class BroadCastChannel: public OChannel<BroadCastChannel<T, Channels...>>
{
public:
    using Type = T;
    using SEvent = Event::Mux<typename Channels::SEvent& ...>;
private:
    std::tuple<Channels&...> channels;
    mutable SEvent events;
public:
    BroadCastChannel(Channels& ...channels) noexcept
    : channels{channels...}
    , events(channels.sevent()...)
    {}
public:
    SEvent& sevent() const noexcept;
    void usend(T value);
    bool spoll() const noexcept;
    void close() noexcept;
    bool closed() const noexcept;
private:
    ...
    template<std::size_t ...I>
    void usend(T value, std::index_sequence<I...>)
    {
        (std::get<I>(channels).usend(value), ...);
    }
    ...
};

template<class ...Channels>
BroadCastChannel(Channels& ...channels) -> BroadCastChannel<std::common_type_t<typename Channels::Type ...>, Channels...>;
```
By default, the input type is computed as ``std::common_type_t<typename Channels::Type ...>``, but you can specify any other type, as long as it can be converted to the types ``typename Channels::Type``.

# Possible Future Developments of the Library
## OS Events
The foundation of the implementation is the event class and subscriber. An event can only transition to a signaled state through the ``emit`` method. This limitation prevents us from generalizing entities such as file descriptors or sockets. Suppose we want to wrap network communication in a channel abstraction. Creating a service or intermediate thread is unacceptable. What can be done in the future:

* Add a new message type - a thin wrapper over an OS object. Such a class will not have an ``emit`` method;
* Implement a partial specialization of the event multiplexing template class if the list of event types includes "OS event";
* The subscriber changes its type or adaptively adjusts to the common event type;
* Combine multiple OS events to use the OS event multiplexing mechanism (for example, using the poll system call in Linux);
* "Regular" events are also combined, for example, in [eventfd](https://man7.org/linux/man-pages/man2/eventfd.2.html) with its descriptor. The emit method of such events will wake up a thread through ``eventfd``.

At the moment, it is difficult to outline the overall picture, but intuition suggests that this is more than possible. Additional complexities arise with channels with dynamic polymorphism; it is necessary to decide what type of event the ``revent/sevent`` methods will return.

## C++20 Coroutines
We can implement channel classes that convert one message type to another through a user-defined function. The implementation is trivial, but it works if the conversion is one-to-one. However, sometimes there is a need for many-to-one (or one-to-many) conversion. For example, from some channel come byte buffers (``std::span<std::byte>`` or ``std::string_view``), and we can write a function that parses such data and returns another object, for example, a JSON structure. It's good if we have a parser that supports a streaming interface (e.g., [boost::json::stream_parser](https://www.boost.org/doc/libs/1_84_0/libs/json/doc/html/json/ref/boost__json__stream_parser.html)).
Otherwise, we need an intermediate thread (or ``stackfull`` coroutine) whose task is to parse multiple objects into one and pass them on.

C++20 has basic support for ``stackless`` coroutines. Integrating them into the library will allow implementing complex converters with minimal overhead. Generators (requirement #5) will become more organic; one C++ coroutine can describe both infinite and finite generators, and all this can be wrapped in a channel interface. Moreover, a thread pool is not needed to execute coroutines; they can all run within a single thread, which reads from or writes to the channel.

## Output iterators and multiplexing channels of different directions
At the moment, write channels do not support iterators. We can multiplex them, but we cannot mix them with read channels. At least, this functionality has not been developed yet. The idea is to enable writing like this:
```cpp
    for (auto& [input, output]: lib::IChannelAll{channel_in, channel_out}) {
        output = process(input);
    }
```
Thus, the loop body will be executed only when both channels are ready: there is a new message in the input channel, and there is space for sending in the output channel.

## Dynamic multiplexing
We can multiplex as many channels as we want into one, but we cannot dynamically change the number of channels. This is remedied by adding a separate event and channel multiplexer class.
Such a channel takes a container (for example, ``std::vector`` or ``std::span``) with the channels to be multiplexed.
This class will only work with channels of the same message type, and the channels to be multiplexed must have dynamic polymorphism.

## Optimizing rpoll/spoll calls
Before each ``usend/urecv`` method call, we must ensure that the channel is available by calling ``rpoll/spoll``.
We can reduce the number of ``rpoll/spoll`` calls by having them return the number of available messages to read/write instead of a boolean value. This count needs to be stored somewhere, thus improving performance slightly at the cost of increased memory consumption.
