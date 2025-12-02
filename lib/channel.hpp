#pragma once
#if false
#include <lib/event.hpp>
#include <lib/buffer.hpp>
#include <lib/overload.hpp>
#include <lib/raw.storage.hpp>

#include <variant>
#include <bitset>
#include <numeric>
#include <stdexcept>


namespace lib {

    template <class Channel>
    struct AsyncRecv
    {
        Channel& channel;
    };

    template <class Event, class TChannel>
    std::size_t wait(Subscriber<Event>& subscriber, const TChannel& channel) noexcept
    {
        subscriber.reset();
        std::size_t ready = channel.poll();

        if (!ready) {
            bool closed = channel.closed();
            while (!closed) {
                subscriber.wait();
                subscriber.reset();

                ready = channel.poll();
                if (ready) {
                    break;
                }
                closed = channel.closed();
            }
        }
        return ready;
    }

    constexpr inline struct TIChannel {} ichannel;
    constexpr inline struct TOChannel {} ochannel;

    template <class Channel>
    class IChannelBase
    {
    public:
        auto recv()
        {
            auto& self = *static_cast<Channel*>(this);
            if (self.poll(ichannel)) {
                return self.peek(ichannel);
            }

            Subscriber subscriber(self.event(ichannel));

            if (wait(subscriber, self)) {
                return self.peek(ichannel);
            }
            throw std::out_of_range("channel is closed");
        }

        auto arecv() noexcept
        {
            return AsyncRecv<Channel>(*static_cast<Channel*>(this));
        }
    };

    /*template <class T>
    class VIChannel: public IChannel<VIChannel<T>>
    {
        struct Interface
        {
            T      (*urecv)(void* channel) noexcept;
            void   (*close)(void* channel) noexcept;
            bool   (*closed)(const void* channel) noexcept;
            bool   (*rpoll) (const void* channel) noexcept;
        };
    private:
        const Interface* interface = nullptr;
        void* channel = nullptr;
        mutable IEvent event;
    public:
        using Type = T;
        using REvent = IEvent;
    public:
        template <class Channel>
        VIChannel(Channel& channel) noexcept
        : channel(&channel)
        , event(channel.revent())
        {
            static const Interface implement = {
                [](void* channel) noexcept {
                    return static_cast<Channel*>(channel)->urecv();
                },
                [](void* channel) noexcept {
                    static_cast<Channel*>(channel)->close();
                },
                [](const void* channel) noexcept {
                    return static_cast<const Channel*>(channel)->closed();
                },
                [](const void* channel) noexcept {
                    return static_cast<const Channel*>(channel)->rpoll();
                }
            };
            interface = &implement;
        }
    public:
        VIChannel(const VIChannel&) = default;
        VIChannel(VIChannel&&) = default;
        VIChannel& operator=(const VIChannel&) = default;
        VIChannel& operator=(VIChannel&&) = default;
    public:
        T urecv()
        {
            return interface->urecv(channel);
        }

        void close() noexcept
        {
            interface->close(channel);
        }
        bool closed() const noexcept
        {
            return interface->closed(channel);
        }

        IEvent& revent() const noexcept
        {
            return event;
        }
        bool rpoll() const noexcept
        {
            return interface->rpoll(channel);
        }
    };

    template <class Channel>
    VIChannel(Channel& channel) -> VIChannel<typename Channel::Type>;*/

    template <class ...Channels>
    class ChannelAny: public IChannelBase<ChannelAny<Channels...>>
    {
    public:
        using Type = std::variant<typename Channels::Type ...>;
        using Event = EventMux<typename Channels::Event ...>;

    private:
        std::tuple<Channels&...> channels;
        mutable Event            events;
        mutable std::size_t      current = 0;
        mutable std::array<std::size_t, sizeof...(Channels)> sizes{};

    public:
        constexpr ChannelAny(Channels&... channels) noexcept
        : channels{channels...}
        , events{channels.event()...}
        {}

        std::size_t poll() const noexcept
        {
            return poll(std::make_index_sequence<sizeof...(Channels)>{});
        }

        bool closed() const noexcept
        {
            return closed(std::make_index_sequence<sizeof...(Channels)>{});
        }

        void close() noexcept
        {
            close(std::make_index_sequence<sizeof...(Channels)>{});
        }

        Event& event() const noexcept
        {
            return events;
        }

        Type peek()
        {
            return peek(std::make_index_sequence<sizeof...(Channels)>{});
        }

        void next()
        {
            return next(std::make_index_sequence<sizeof...(Channels)>{});
        }

    private:
        template <std::size_t ...I>
        std::size_t poll(std::index_sequence<I...>) const noexcept
        {
            using Function = bool (*)(const ChannelAny*);
            static const std::array<Function, sizeof...(Channels)> function {
                [] (const ChannelAny* channel) {
                    return std::get<I>(channel->channels).poll();
                }...
            };

            if (sizes[current] != 0) {
                return std::accumulate(sizes.begin(), sizes.end(), 0);
            }

            std::size_t max = 0;
            for (std::size_t i = 0; i < sizeof...(Channels); ++i) {
                if (sizes[i] == 0) {
                    std::size_t size = sizes[i] = function[i](this);
                    events.set_reset_mask(i, size != 0);
                    if (size > max) {
                        current = i;
                        max = size;
                    }
                }
            }
            return std::accumulate(sizes.begin(), sizes.end(), 0);
        }

        template <std::size_t ...I>
        Type peek(std::index_sequence<I...>)
        {
            using Function = Type (*)(ChannelAny*);
            static const std::array<Function, sizeof...(Channels)> function {
                [] (ChannelAny* channel) -> Type& {
                    return Type(std::in_place_index<I>, std::get<I>(channel->channels).peek());
                }...
            };
            return function[current](this);
        }

        template <std::size_t ...I>
        void next(std::index_sequence<I...>)
        {
            using Function = Type (*)(ChannelAny*);
            static const std::array<Function, sizeof...(Channels)> function {
                [] (ChannelAny* channel) -> Type& {
                    std::get<I>(channel->channels).next();
                }...
            };
            function[current](this);
            sizes[current] -= 1;

            for (std::size_t i = 0; i < sizeof...(Channels); ++i) {
                if (sizes[current] != 0) {
                    break;
                }
                current += 1;
                if (current == sizeof...(Channels)) {
                    current = 0;
                }
            }
        }

        template <std::size_t ...I>
        bool closed(std::index_sequence<I...>) const noexcept
        {
            return (std::get<I>(channels).closed() && ...);
        }

        template <std::size_t ...I>
        void close(std::index_sequence<I...>) noexcept
        {
            (std::get<I>(channels).close(), ...);
        }
    };
    template <typename... Channels>
    ChannelAny(Channels&... channels) -> ChannelAny<Channels...>;

    template <class ...Channels>
    class ChannelAll: public IChannelBase<ChannelAll<Channels...>>
    {
    public:
        using Type = std::tuple<typename Channels::Type...>;
        using Event = EventMux<typename Channels::REvent&...>;

    private:
        std::tuple<Channels&...>                 channels;
        mutable Event                            events;
        mutable std::bitset<sizeof...(Channels)> states;
        mutable std::array<std::size_t, sizeof...(Channels)> sizes{};

    public:
        constexpr ChannelAll(Channels&... channels) noexcept
        : channels{channels...}
        , events{channels.event()...}
        {}

        std::size_t poll() const noexcept
        {
            return poll(std::make_index_sequence<sizeof...(Channels)>{});
        }

        bool closed() const noexcept
        {
            return closed(std::make_index_sequence<sizeof...(Channels)>{});
        }

        void close() noexcept
        {
            close(std::make_index_sequence<sizeof...(Channels)>{});
        }

        Event& event() const noexcept
        {
            return events;
        }

        Type peek()
        {
            return peek(std::make_index_sequence<sizeof...(Channels)>{});
        }

        void next()
        {
            return next(std::make_index_sequence<sizeof...(Channels)>{});
        }

    private:
        template <std::size_t ...I>
        std::size_t poll(std::index_sequence<I...>) const noexcept
        {
            using Function = bool (*)(const ChannelAll*);
            static const std::array<Function, sizeof...(Channels)> function {
                [] (const ChannelAll* channel) {
                    return std::get<I>(channel->channels).poll();
                }...
            };

            std::size_t min = std::numeric_limits<std::size_t>::max();
            for (std::size_t i = 0; i < sizeof...(Channels); ++i) {
                std::size_t size = sizes[i];
                if (size == 0) {
                    size = function[i](this);
                    events.set_reset_mask(i, size != 0);
                    sizes[i] = size;
                }
                if (size < min) {
                    min = size;
                }
            }
            return min;
        }

        template <std::size_t ...I>
        Type peek(std::index_sequence<I...>)
        {
            return Type(std::get<I>(channels).peek() ...);
        }

        template <std::size_t ...I>
        void next(std::index_sequence<I...>)
        {
            (std::get<I>(channels).next(), ...);
        }

        template <std::size_t ...I>
        bool closed(std::index_sequence<I...>) const noexcept
        {
            return (std::get<I>(channels).closed() && ...);
        }

        template <std::size_t ...I>
        void close(std::index_sequence<I...>) noexcept
        {
            (std::get<I>(channels).close(), ...);
        }
    };
    template <typename... Channels>
    ChannelAll(Channels&... channels) -> ChannelAll<Channels...>;

    template <class Channel>
    class IRange
    {
        friend class Iterator;
    private:
        using Value = std::remove_cv_t<std::remove_reference_t<typename Channel::Type>>;

        Channel& channel;
        Subscriber<typename Channel::REvent> subscriber;

        RawStorage<Value> storage;
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
                    range->storage.emplace(range->channel.urecv());
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
                    range->storage.destroy();
                }
            }
            auto& operator*() const noexcept
            {
                return *(range->storage.ptr());
            }
            Iterator& operator++()
            {
                range->storage.destroy();

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
        , subscriber(channel.revent())
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

    template <class Channel>
    auto irange(Channel& channel)
    {
        return IRange<Channel>(channel);
    }

    template <class Channel>
    class OChannel
    {
    public:
        template <class Event, class TChannel>
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

        template <class T>
        void send(T&& value)
        {
            auto& self = *static_cast<Channel*>(this);
            if (self.spoll()) {
                return self.usend(std::forward<T>(value));
            }
            Subscriber subscriber(self.sevent());

            if (swait(subscriber, self)) {
                self.usend(std::forward<T>(value));
            }
        }
    };

    template <class T>
    class VOChannel: public OChannel<VOChannel<T>>
    {
        struct Interface
        {
            void   (*usend)(void* channel, T value) noexcept;
            void   (*close)(void* channel) noexcept;
            bool   (*closed)(const void* channel) noexcept;
            bool   (*spoll) (const void* channel) noexcept;
        };

    private:
        const Interface* interface = nullptr;
        void* channel = nullptr;
        mutable Event event;

    public:
        using Type = T;
        using SEvent = Event;

    public:
        template <class Channel>
        VOChannel(Channel& channel) noexcept
        : channel(&channel)
        , event(channel.sevent())
        {
            static const Interface implement = {
                [](void* channel, T value) noexcept {
                    static_cast<Channel*>(channel)->usend(std::move(value));
                },
                [](void* channel) noexcept {
                    static_cast<Channel*>(channel)->close();
                },
                [](const void* channel) noexcept {
                    return static_cast<const Channel*>(channel)->closed();
                },
                [](const void* channel) noexcept {
                    return static_cast<const Channel*>(channel)->spoll();
                }
            };
            interface = &implement;
        }
    public:
        VOChannel(const VOChannel&) = default;
        VOChannel(VOChannel&&) = default;
        VOChannel& operator=(const VOChannel&) = default;
        VOChannel& operator=(VOChannel&&) = default;
    public:
        void usend(T value)
        {
            interface->usend(channel, value);
        }

        void close() noexcept
        {
            interface->close(channel);
        }
        bool closed() const noexcept
        {
            return interface->closed(channel);
        }

        SEvent& sevent() const noexcept
        {
            return event;
        }
        bool spoll() const noexcept
        {
            return interface->spoll(channel);
        }
    };

    template <class Channel>
    VOChannel(Channel& channel) -> VOChannel<typename Channel::Type>;

    template <class T>
    class IOChannel: public IChannel<T>, public OChannel<T>
    {
    };

    template <class T>
    class BlackHole: public OChannel<BlackHole<T>>
    {
        mutable NeverEvent event;
    public:
        using Type = T;
        using SEvent = const NeverEvent;
    public:
        SEvent& sevent() const noexcept
        {
            return event;
        }
        void close() const noexcept {}
        bool closed() const noexcept
        {
            return false;
        }
        void usend(T) const noexcept {}
        bool spoll() const noexcept
        {
            return true;
        }
    };

    template <class T>
    const inline BlackHole<T> black_hole {};
}
#endif
