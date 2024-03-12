#pragma once
#include <array>
#include <lib/buffered.channel.hpp>
#include <lib/mutex.hpp>

namespace lib {

    template<class T, std::size_t Count>
    class AggregateChannel: public IChannel<T>
    {
        std::array<IChannel<T>*, Count> channels;
        mutable EventMux<Count> events;
        mutable std::size_t current;
    private:
        template<class IChannel, std::size_t ...I>
        AggregateChannel(IChannel (&channels)[Count], std::index_sequence<I...>) noexcept
        : channels{&channels[I]...}
        , events(channels[I].revent()...)
        , current(0)
        {
        }
    public:
        template<class ...Types>
        AggregateChannel(IChannel<Types>& ...channels) noexcept
        : channels{&channels...}
        , events(channels.revent()...)
        , current(0)
        {
            static_assert(sizeof...(Types) == Count);
        }
        template<class IChannel>
        AggregateChannel(IChannel (&channels)[Count]) noexcept
        : AggregateChannel(channels, std::make_index_sequence<Count>{})
        {
        }
    public:
        IEvent revent() const noexcept override
        {
            return events;
        }
        T urecv() override
        {
            return channels[current]->urecv();
        }
        bool rpoll() const noexcept override
        {
            for(std::size_t i = 0; i < Count; ++i) {
                current += 1;
                if(current == Count) {
                    current = 0;
                }
                if(channels[current]->rpoll()) {
                    return true;
                }
            }
            return false;
        }
        void close() noexcept override
        {
            for(auto& channel: channels) {
                channel->close();
            }
        }
        bool closed() const noexcept override
        {
            for(auto& channel: channels) {
                if(!channel->closed()) {
                    return false;
                }
            }
            return true;
        }
    };

    template<typename T, typename... Types>
    AggregateChannel(IChannel<T>& channel, IChannel<Types>&... channels) -> AggregateChannel<std::enable_if_t<(std::is_same_v<T, Types> && ...), T>, sizeof...(Types) + 1>;
}