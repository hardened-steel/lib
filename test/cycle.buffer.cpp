#include <gtest/gtest.h>
#include <lib/cycle.buffer.hpp>

TEST(cyclebuffer, base)
{
    using namespace lib;

    CycleBuffer<int, 2> buffer;
    EXPECT_EQ(buffer.rsize(), 0);
    EXPECT_EQ(buffer.wsize(), 2);
    for(auto value: buffer) {
        FAIL() << "buffer must be a empty";
    }

    buffer.send(1);
    EXPECT_EQ(buffer.rsize(), 1);
    EXPECT_EQ(buffer.wsize(), 1);
    for(auto value: buffer) {
        EXPECT_EQ(value, 1);
    }
    for(auto value: buffer) {
        EXPECT_EQ(value, 1);
    }
    buffer.send(2);
    EXPECT_EQ(buffer.rsize(), 2);
    EXPECT_EQ(buffer.wsize(), 0);

    {
        std::vector<int> evalues = {1, 2};
        std::vector<int> ivalues;
        for(auto value: buffer) {
            ivalues.push_back(value);
        }
        EXPECT_EQ(ivalues, evalues);
    }
    {
        std::vector<int> evalues = {2, 1};
        std::vector<int> ivalues;
        for(auto value: buffer.reverse()) {
            ivalues.push_back(value);
        }
        EXPECT_EQ(ivalues, evalues);
    }
}
