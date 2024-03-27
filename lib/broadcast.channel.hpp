#pragma once
#include <array>
#include <lib/buffered.channel.hpp>
#include <lib/mutex.hpp>

namespace lib {
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
        SEvent& sevent() const noexcept
        {
            return events;
        }
        void usend(T value)
        {
            usend(std::move(value), std::make_index_sequence<sizeof...(Channels)>{});
        }
        bool spoll() const noexcept
        {
            return spoll(std::make_index_sequence<sizeof...(Channels)>{});
        }
        void close() noexcept
        {
            close(std::make_index_sequence<sizeof...(Channels)>{});
        }
        bool closed() const noexcept
        {
            return closed(std::make_index_sequence<sizeof...(Channels)>{});
        }
    private:
        template<std::size_t ...I>
        bool spoll(std::index_sequence<I...>) const noexcept
        {
            return (std::get<I>(channels).spoll() && ...);
        }

        template<std::size_t ...I>
        void usend(T value, std::index_sequence<I...>)
        {
            (std::get<I>(channels).usend(value), ...);
        }

        template<std::size_t ...I>
        unsigned closed(std::index_sequence<I...>) const noexcept
        {
            return (std::get<I>(channels).closed() || ...);
        }

        template<std::size_t ...I>
        void close(std::index_sequence<I...>) noexcept
        {
            (std::get<I>(channels).close(), ...);
        }
    };

    template<class ...Channels>
    BroadCastChannel(Channels& ...channels) -> BroadCastChannel<std::common_type_t<typename Channels::Type ...>, Channels...>;
}
