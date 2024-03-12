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

    class Channel
    {
    public:
        virtual void close() noexcept = 0;
        virtual bool closed() const noexcept = 0;
        virtual ~Channel() noexcept {}
    };

    class IChannelBase: public Channel
    {
    public:
        virtual IEvent revent() const noexcept = 0;
        virtual bool rpoll() const noexcept = 0;
        static bool rwait(IHandler& handler, const IChannelBase& channel) noexcept;
    };

    template<class T>
    class IChannel: public IChannelBase
    {
        template<class ...Types>
        friend class IChannelAny;
    public:
        using Type = T;
    public:
        virtual T urecv() = 0;
        void recv(T& value)
        {
            if (rpoll()) {
                value = urecv();
                return;
            }

            Handler handler;
            revent().subscribe(&handler);

            if (IChannelBase::rwait(handler, *this)) {
                value = urecv();
            }
            revent().subscribe(nullptr);
        }
        T recv()
        {
            if (rpoll()) {
                return urecv();
            }

            Handler handler;
            revent().subscribe(&handler);

            if (IChannelBase::rwait(handler, *this)) {
                revent().subscribe(nullptr);
                return urecv();
            }
            throw std::out_of_range("channel is closed");
        }
    };

    template<class ...Types>
    class IChannelAny: public IChannel<std::variant<Types...>>
    {
        std::tuple<IChannel<Types>&...>       channels;
        mutable Event::Mux<sizeof...(Types)>  events;
        mutable std::bitset<sizeof...(Types)> states;
    public:
        constexpr IChannelAny(IChannel<Types>&... channels) noexcept
        : channels{channels...}
        , events{channels.revent()...}
        {
        }
        bool rpoll() const noexcept override
        {
            return rpoll(std::make_index_sequence<sizeof...(Types)>{});
        }
        bool closed() const noexcept override
        {
            return closed(std::make_index_sequence<sizeof...(Types)>{});
        }
        void close() noexcept override
        {
            close(std::make_index_sequence<sizeof...(Types)>{});
        }
        IEvent revent() const noexcept override
        {
            return events;
        }
        std::variant<Types...> urecv() override
        {
            return urecv(std::make_index_sequence<sizeof...(Types)>{});
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
            if(states.any()) {
                return true;
            }
            ((states[I] = std::get<I>(channels).rpoll()), ...);
            return states.any();
        }
        
        template<std::size_t ...I>
        std::variant<Types...> urecv(std::index_sequence<I...>)
        {
            for(std::size_t i = 0; i < sizeof...(Types); ++i) {
                if(states[i]) {
                    return lib::rswitch(
                        i,
                        [this]() noexcept(std::is_nothrow_constructible_v<std::tuple_element_t<I, std::tuple<Types...>>, decltype(std::get<I>(channels).urecv())>)
                        {
                            states[I] = false;
                            return std::variant<Types...>(std::in_place_index<I>, std::get<I>(channels).urecv());
                        }...
                    );
                }
            }
            LIB_UNREACHABLE;
        }
    };
    template<typename... Types>
    IChannelAny(IChannel<Types>&... channels) -> IChannelAny<Types...>;

    template<class ...Types>
    class IChannelAll: public IChannel<std::tuple<Types...>>
    {
        std::tuple<IChannel<Types>&...>       channels;
        mutable Event::Mux<sizeof...(Types)>  events;
        mutable std::bitset<sizeof...(Types)> states;
    public:
        constexpr IChannelAll(IChannel<Types>&... channels) noexcept
        : channels{channels...}
        , events{channels.revent()...}
        {
        }
        bool rpoll() const noexcept override
        {
            return rpoll(std::make_index_sequence<sizeof...(Types)>{});
        }
        bool closed() const noexcept override
        {
            return closed(std::make_index_sequence<sizeof...(Types)>{});
        }
        void close() noexcept override
        {
            close(std::make_index_sequence<sizeof...(Types)>{});
        }
        IEvent revent() const noexcept override
        {
            return events;
        }
        std::tuple<Types...> urecv() override
        {
            return urecv(std::make_index_sequence<sizeof...(Types)>{});
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
        std::tuple<Types...> urecv(std::index_sequence<I...>)
        {
            states.reset();
            return std::make_tuple(std::get<I>(channels).urecv()...);
        }
    };
    template<typename... Types>
    IChannelAll(IChannel<Types>&... channels) -> IChannelAll<Types...>;

    template<class Channel>
    class IRange
    {
        friend class Iterator;
    private:
        using Value = std::remove_cv_t<std::remove_reference_t<typename Channel::Type>>;
        Handler  handler;
        Channel& channel;
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
                if (!range->channel.rpoll() && !IChannelBase::rwait(range->handler, range->channel)) {
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
        {
            channel.revent().subscribe(&handler);
        }
        ~IRange() noexcept
        {
            channel.revent().subscribe(nullptr);
        }
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

    template<class T>
    auto irange(IChannel<T>& channel)
    {
        return IRange<IChannel<T>>(channel);
    }

    class OChannelBase: public Channel
    {
    public:
        virtual IEvent sevent() const noexcept = 0;
        virtual bool spoll() const noexcept = 0;
        static bool swait(IHandler& handler, const OChannelBase& channel) noexcept;
    };

    template<class T>
    class OChannel: public OChannelBase
    {
    public:
        using Type = T;
    public:
        virtual void usend(T value) = 0;
        void send(T value)
        {
            if (spoll()) {
                return usend(std::move(value));
            }

            Handler handler;
            sevent().subscribe(&handler);

            if (OChannelBase::swait(handler, *this)) {
                usend(std::move(value));
            }
            sevent().subscribe(nullptr);
        }
    };

    template<class T>
    class IOChannel: public IChannel<T>, public OChannel<T>
    {
    public:
        using Type = T;
    };

    template<class T>
    class BlackHole: public OChannel<T>
    {
        mutable NeverEvent event;
    public:
        IEvent sevent() const noexcept override
        {
            return event;
        }
        void close() noexcept override {}
        bool closed() const noexcept override
        {
            return false;
        }
        void usend(T) noexcept override {}
        bool spoll() const noexcept override
        {
            return true;
        }
    };

    template<class T>
    inline BlackHole<T> blackHole {};
}
