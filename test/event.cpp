#include <gtest/gtest.h>
#include <lib/event.hpp>
#include <future>

TEST(lib, event)
{
    lib::Event a, b, c, d;
    {
        ASSERT_FALSE(a.poll());
        lib::EventMux mux {a, b, c, d};
        lib::Handler handler;
        mux.subscribe(&handler);

        auto result = std::async([&] { a.emit(); });
        handler.wait();
        result.get();
        mux.subscribe(nullptr);
    }
    {
        ASSERT_FALSE(a.poll());
        lib::EventMux mux {a, b, c, d};
        lib::Handler handler;
        mux.subscribe(&handler);

        auto result = std::async([&] { a.emit(); });
        handler.wait();
        result.get();
    }
    {
        lib::EventMux mux {a, b, c, d};
        lib::Handler handler;
        mux.subscribe(&handler);

        auto result = std::async([&] { c.emit(); b.emit(); });
        handler.wait();
        ASSERT_TRUE(c.poll() || b.poll());
        handler.wait();
        ASSERT_TRUE(c.poll() && b.poll());
        result.get();
    }
}
