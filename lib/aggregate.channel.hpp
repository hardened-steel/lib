#pragma once
#include <lib/buffered.channel.hpp>
#include <lib/concept.hpp>

namespace lib {

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
        bool rpoll(std::index_sequence<I...>) const noexcept
        {
            using PollFn = bool (*)(const AggregateChannel*);
            static const std::array<PollFn, sizeof...(Channels)> poll {
                [](const AggregateChannel* channel) {
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
            using RecvFn = Type (*)(AggregateChannel*);
            static const std::array<RecvFn, sizeof...(Channels)> recv {
                [](AggregateChannel* channel) {
                    return std::get<I>(channel->channels).urecv();
                }...
            };
            return recv[current](this);
        }

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
    };

    template<class ...Channels>
    AggregateChannel(Channels& ...channels) -> AggregateChannel<std::common_type_t<typename Channels::Type ...>, Channels...>;
}