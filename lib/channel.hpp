#pragma once
#include <variant>
#include <optional>
#include <bitset>
#include <stdexcept>
#include <lib/event.hpp>
#include <lib/buffer.hpp>
#include <lib/overload.hpp>
#include <lib/unreachable.hpp>

namespace lib {

    template<class Channel>
    class IChannel
    {
    public:
        template<class TChannel>
        static bool rwait(IHandler& handler, const TChannel& channel) noexcept
        {
            auto& event = channel.revent();
            event.reset();
            bool poll = channel.rpoll();
            while(!poll && (!channel.closed() || event.poll())) {
                handler.wait();
                event.reset();
                poll = channel.rpoll();
            }
            return poll;
        }

        auto recv()
        {
            auto& self = *static_cast<Channel*>(this);
            if (self.rpoll()) {
                return self.urecv();
            }

            Handler handler;
            SubscribeGuard subscriber(self.revent(), handler);

            if (rwait(handler, self)) {
                return self.urecv();
            }
            throw std::out_of_range("channel is closed");
        }
    };

    template<class T>
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
        template<class Channel>
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

    template<class Channel>
    VIChannel(Channel& channel) -> VIChannel<typename Channel::Type>;

    template<class ...Channels>
    class IChannelAny: public IChannel<IChannelAny<Channels...>>
    {
    public:
        using Type = std::variant<typename Channels::Type...>;
        using REvent = Event::Mux<typename Channels::REvent&...>;
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
        bool rpoll() const noexcept
        {
            return rpoll(std::make_index_sequence<sizeof...(Channels)>{});
        }
        bool closed() const noexcept
        {
            return closed(std::make_index_sequence<sizeof...(Channels)>{});
        }
        void close() noexcept
        {
            close(std::make_index_sequence<sizeof...(Channels)>{});
        }
        REvent& revent() const noexcept
        {
            return events;
        }
        Type urecv()
        {
            return urecv(std::make_index_sequence<sizeof...(Channels)>{});
        }
    private:
        template<std::size_t ...I>
        unsigned closed(std::index_sequence<I...>) const noexcept
        {
            return (std::get<I>(channels).closed() && ...);
        }

        template<std::size_t ...I>
        void close(std::index_sequence<I...>) noexcept
        {
            (std::get<I>(channels).close(), ...);
        }

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
                current += 1;
                if (current == sizeof...(Channels)) {
                    current = 0;
                }
                if (poll[current](this)) {
                    return true;
                }
            }
            return false;
        }
        
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
    };
    template<typename... Channels>
    IChannelAny(Channels&... channels) -> IChannelAny<Channels...>;

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
        bool rpoll() const noexcept
        {
            return rpoll(std::make_index_sequence<sizeof...(Channels)>{});
        }
        bool closed() const noexcept
        {
            return closed(std::make_index_sequence<sizeof...(Channels)>{});
        }
        void close() noexcept
        {
            close(std::make_index_sequence<sizeof...(Channels)>{});
        }
        REvent& revent() const noexcept
        {
            return events;
        }
        Type urecv()
        {
            return urecv(std::make_index_sequence<sizeof...(Channels)>{});
        }
    private:
        template<std::size_t ...I>
        unsigned closed(std::index_sequence<I...>) const noexcept
        {
            return ((std::get<I>(channels).closed() && !std::get<I>(channels).rpoll()) || ...);
        }

        template<std::size_t ...I>
        void close(std::index_sequence<I...>) noexcept
        {
            (std::get<I>(channels).close(), ...);
        }

        template<std::size_t ...I>
        bool rpoll(std::index_sequence<I...>) const noexcept
        {
            if(states.all()) {
                return true;
            }
            ((states[I] = states.test(I) || std::get<I>(channels).rpoll()), ...);
            return states.all();
        }

        template<std::size_t ...I>
        Type urecv(std::index_sequence<I...>)
        {
            states.reset();
            return std::make_tuple(std::get<I>(channels).urecv()...);
        }
    };
    template<typename... Channels>
    IChannelAll(Channels&... channels) -> IChannelAll<Channels...>;

    template<class Channel>
    class IRange
    {
        friend class Iterator;
    private:
        using Value = std::remove_cv_t<std::remove_reference_t<typename Channel::Type>>;

        Channel& channel;
        Handler  handler;
        SubscribeGuard<typename Channel::REvent> subscriber;
        
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
                if (!range->channel.rpoll() && !Channel::rwait(range->handler, range->channel)) {
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

    template<class Channel>
    class OChannel
    {
    public:
        template<class TChannel>
        static bool swait(IHandler& handler, const TChannel& channel) noexcept
        {
            auto& event = channel.sevent();
            event.reset();
            bool poll = channel.spoll();
            while(!poll && !channel.closed()) {
                handler.wait();
                event.reset();
                poll = channel.spoll();
            }
            return poll;
        }

        template<class T>
        void send(T&& value)
        {
            auto& self = *static_cast<Channel*>(this);
            if (self.spoll()) {
                return self.usend(std::forward<T>(value));
            }
            Handler handler;
            SubscribeGuard subscriber(self.sevent(), handler);

            if (swait(handler, self)) {
                self.usend(std::forward<T>(value));
            }
        }
    };

    template<class T>
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
        mutable IEvent event;
    public:
        using Type = T;
        using SEvent = IEvent;
    public:
        template<class Channel>
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

    template<class Channel>
    VOChannel(Channel& channel) -> VOChannel<typename Channel::Type>;

    template<class T>
    class IOChannel: public IChannel<T>, public OChannel<T>
    {
    };

    template<class T>
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

    template<class T>
    const inline BlackHole<T> blackHole {};
}
