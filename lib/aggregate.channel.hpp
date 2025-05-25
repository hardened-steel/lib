#pragma once
#include <numeric>
#include <lib/buffered.channel.hpp>
#include <lib/concept.hpp>

namespace lib {

    template<class T, class ...Channels>
    class AggregateChannel: public IChannelBase<AggregateChannel<T, Channels...>>
    {
    public:
        using Type = T;
        using Event = EventMux<typename Channels::Event ...>;
    private:
        std::tuple<Channels&...> channels;
        mutable Event            events;
        mutable std::size_t      current = 0;
        mutable std::array<std::size_t, sizeof...(Channels)> sizes{};
    public:
        AggregateChannel(Channels& ...channels) noexcept
        : channels{channels...}
        , events(channels.event()...)
        {}
    public:
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

        Type& peek()
        {
            return peek(std::make_index_sequence<sizeof...(Channels)>{});
        }

        void next()
        {
            return next(std::make_index_sequence<sizeof...(Channels)>{});
        }
    private:
        template<std::size_t ...I>
        std::size_t poll(std::index_sequence<I...>) const noexcept
        {
            using Function = bool (*)(const AggregateChannel*);
            static const std::array<Function, sizeof...(Channels)> function {
                [] (const AggregateChannel* channel) {
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

        template<std::size_t ...I>
        Type& peek(std::index_sequence<I...>)
        {
            using Function = Type (*)(AggregateChannel*);
            static const std::array<Function, sizeof...(Channels)> function {
                [] (AggregateChannel* channel) -> Type& {
                    return std::get<I>(channel->channels).peek();
                }...
            };
            return function[current](this);
        }

        template<std::size_t ...I>
        void next(std::index_sequence<I...>)
        {
            using Function = Type (*)(AggregateChannel*);
            static const std::array<Function, sizeof...(Channels)> function {
                [] (AggregateChannel* channel) -> Type& {
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

        template<std::size_t ...I>
        bool closed(std::index_sequence<I...>) const noexcept
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
