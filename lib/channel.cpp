#include <lib/channel.hpp>

namespace lib {
    /*bool IChannelBase::rwait(IHandler& handler, const IChannelBase& channel) noexcept
    {
        IEvent event = channel.revent();
        event.reset();
        auto poll = channel.rpoll();
        while(!poll && (!channel.closed() || event.poll())) {
            handler.wait();
            event.reset();
            poll = channel.rpoll();
        }
        return poll;
    }

    bool OChannelBase::swait(IHandler& handler, const OChannelBase& channel) noexcept
    {
        IEvent event = channel.sevent();
        event.reset();
        auto poll = channel.spoll();
        while(!poll && !channel.closed()) {
            handler.wait();
            event.reset();
            poll = channel.spoll();
        }
        return poll;
    }*/
}
