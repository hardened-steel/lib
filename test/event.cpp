#include <gtest/gtest.h>
#include <lib/event.hpp>
#include <future>

TEST(lib, event)
{
    lib::Event event_a;
    lib::Event event_b;
    lib::Event event_c;
    lib::Event event_d;
    {
        ASSERT_FALSE(event_a.poll());
        lib::EventMux mux {event_a, event_b, event_c, event_d};
        lib::Subscriber subscriber(mux);

        auto result = std::async([&] { event_a.emit(); });
        subscriber.wait();
        result.get();
    }
    {
        ASSERT_FALSE(event_a.poll());
        lib::EventMux mux {event_a, event_b, event_c, event_d};
        lib::Subscriber subscriber(mux);

        auto result = std::async([&] { event_a.emit(); });
        subscriber.wait();
        result.get();
    }
    {
        lib::EventMux mux {event_a, event_b, event_c, event_d};
        lib::Subscriber subscriber(mux);

        auto result = std::async([&] { event_c.emit(); event_b.emit(); });
        subscriber.wait();
        ASSERT_TRUE(event_c.poll() || event_b.poll());
        subscriber.wait();
        ASSERT_TRUE(event_c.poll() && event_b.poll());
        result.get();
    }
}
