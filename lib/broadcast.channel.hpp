#pragma once
#include <array>
#include <lib/buffered.channel.hpp>
#include <lib/mutex.hpp>

namespace lib {
    template<class T, std::size_t Count>
    class BroadCastChannel: public OChannel<T>
    {
    public:
        const std::array<OChannel<T>*, Count> channels;
    private:
        mutable EventMux<Count> events;
        bool closed_;
    private:
        template<class OChannel, std::size_t ...I>
        BroadCastChannel(OChannel (&channels)[Count], std::index_sequence<I...>) noexcept
        : channels{&channels[I]...}
        , events(channels[I].sevent()...)
        , closed_(false)
        {
        }
        template<std::size_t ...I>
        BroadCastChannel(std::array<OChannel<T>*, Count> channels, std::index_sequence<I...>) noexcept
        : channels{channels[I]...}
        , events(channels[I]->sevent()...)
        , closed_(false)
        {
        }
    public:
        template<class ...Types>
        BroadCastChannel(OChannel<Types>& ...channels) noexcept
        : channels{&channels...}
        , events(channels.sevent()...)
        , closed_(false)
        {
            static_assert(sizeof...(Types) == Count);
        }
        template<class OChannel>
        BroadCastChannel(OChannel (&channels)[Count]) noexcept
        : BroadCastChannel(channels, std::make_index_sequence<Count>{})
        {
        }
        BroadCastChannel(std::array<OChannel<T>*, Count> channels) noexcept
        : BroadCastChannel(channels, std::make_index_sequence<Count>{})
        {
        }
    public:
        IEvent sevent() const noexcept override
        {
            return events;
        }
        void usend(T value) override
        {
            for(auto& channel: channels) {
                channel->usend(value);
            }
        }
        bool spoll() const noexcept override
        {
            for(auto& channel: channels) {
                if(!channel->spoll()) {
                    return false;
                }
            }
            return true;
        }
        void close() noexcept override
        {
            for(auto& channel: channels) {
                channel->close();
            }
            closed_ = true;
        }
        bool closed() const noexcept override
        {
            return closed_;
        }
    };

    template<typename T, typename... Types>
    BroadCastChannel(OChannel<T>& channel, OChannel<Types>&... channels) -> BroadCastChannel<std::enable_if_t<(std::is_same_v<T, Types> && ...), T>, sizeof...(Types) + 1>;
}