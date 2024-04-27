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
        lib::Handler handler;
        mux.subscribe(&handler);

        auto result = std::async([&] { event_a.emit(); });
        handler.wait();
        result.get();
        mux.subscribe(nullptr);
    }
    {
        ASSERT_FALSE(event_a.poll());
        lib::EventMux mux {event_a, event_b, event_c, event_d};
        lib::Handler handler;
        mux.subscribe(&handler);

        auto result = std::async([&] { event_a.emit(); });
        handler.wait();
        result.get();
    }
    {
        lib::EventMux mux {event_a, event_b, event_c, event_d};
        lib::Handler handler;
        mux.subscribe(&handler);

        auto result = std::async([&] { event_c.emit(); event_b.emit(); });
        handler.wait();
        ASSERT_TRUE(event_c.poll() || event_b.poll());
        handler.wait();
        ASSERT_TRUE(event_c.poll() && event_b.poll());
        result.get();
    }
}
