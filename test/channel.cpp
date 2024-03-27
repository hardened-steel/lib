#include <gtest/gtest.h>
#include <lib/buffered.channel.hpp>
#include <lib/aggregate.channel.hpp>
#include <lib/broadcast.channel.hpp>
#include <thread>
#include <future>

TEST(lib, buffered_channel_base)
{
    lib::BufferedChannel<int, 2> iochannel;

    ASSERT_EQ(iochannel.rsize(), 0);
    ASSERT_EQ(iochannel.wsize(), 2);

    iochannel.send(1);

    ASSERT_EQ(iochannel.rsize(), 1);
    ASSERT_EQ(iochannel.wsize(), 1);

    iochannel.send(2);

    ASSERT_EQ(iochannel.rsize(), 2);
    ASSERT_EQ(iochannel.wsize(), 0);

    ASSERT_EQ(iochannel.urecv(), 1);
    ASSERT_EQ(iochannel.urecv(), 2);

    ASSERT_EQ(iochannel.rsize(), 0);
    ASSERT_EQ(iochannel.wsize(), 2);

    iochannel.send(3);

    ASSERT_EQ(iochannel.rsize(), 1);
    ASSERT_EQ(iochannel.wsize(), 1);

    iochannel.send(4);

    ASSERT_EQ(iochannel.rsize(), 2);
    ASSERT_EQ(iochannel.wsize(), 0);

    ASSERT_EQ(iochannel.urecv(), 3);
    ASSERT_EQ(iochannel.urecv(), 4);

    ASSERT_EQ(iochannel.rsize(), 0);
    ASSERT_EQ(iochannel.wsize(), 2);
}

TEST(lib, buffered_channel_async)
{
    lib::BufferedChannel<int, 2> iochannel;

    ASSERT_TRUE(iochannel.spoll());
    ASSERT_FALSE(iochannel.rpoll());

    iochannel.send(3);
    iochannel.send(4);

    ASSERT_FALSE(iochannel.spoll());
    ASSERT_TRUE(iochannel.rpoll());

    {
        auto result = std::async(
            [](lib::VOChannel<int> ochannel) {
                ochannel.send(5);
                ochannel.send(6);
                ochannel.close();
            },
            lib::VOChannel<int>(iochannel)
        );
        lib::VIChannel ichannel = iochannel;
        ASSERT_EQ(ichannel.recv(), 3);
        ASSERT_EQ(ichannel.recv(), 4);
        ASSERT_EQ(ichannel.recv(), 5);
        ASSERT_EQ(ichannel.recv(), 6);
        ASSERT_FALSE(iochannel.rpoll());
        result.get();
    }
}

TEST(lib, buffered_channel_mux_any)
{
    lib::BufferedChannel<int, 1> chA;
    lib::BufferedChannel<int, 2> chB;
    lib::BufferedChannel<int, 3> chC;
    lib::BufferedChannel<int, 4> chD;
    lib::BufferedChannel<int, 5> chE;

    auto worker = [](lib::VOChannel<int> ochannel, int base)
    {
        for(std::size_t i = 0; i < 4; ++i) {
            ochannel.send(base + i * base);
        }
        ochannel.close();
    };
    std::array threads = {
        std::async(worker, lib::VOChannel<int>(chA), 2),
        std::async(worker, lib::VOChannel<int>(chB), 3),
        std::async(worker, lib::VOChannel<int>(chC), 5),
        std::async(worker, lib::VOChannel<int>(chD), 6),
        std::async(worker, lib::VOChannel<int>(chE), 7)
    };
    std::vector<int> values[5];
    lib::IChannelAny ichannels(chA, chB, chC, chD, chE);

    for(auto result: lib::irange(ichannels)) {
        switch(result.index()) {
            case 0: values[0].push_back(std::get<0>(result)); break;
            case 1: values[1].push_back(std::get<1>(result)); break;
            case 2: values[2].push_back(std::get<2>(result)); break;
            case 3: values[3].push_back(std::get<3>(result)); break;
            case 4: values[4].push_back(std::get<4>(result)); break;
        }
    }
    EXPECT_EQ(values[0], (std::vector<int>{2, 4, 6, 8}));
    EXPECT_EQ(values[1], (std::vector<int>{3, 6, 9, 12}));
    EXPECT_EQ(values[2], (std::vector<int>{5, 10, 15, 20}));
    EXPECT_EQ(values[3], (std::vector<int>{6, 12, 18, 24}));
    EXPECT_EQ(values[4], (std::vector<int>{7, 14, 21, 28}));

    for(auto& thread: threads) {
        thread.get();
    }
}

TEST(lib, buffered_channel_mux_mux_any)
{
    lib::BufferedChannel<int, 1> chA;
    lib::BufferedChannel<int, 2> chB;
    lib::BufferedChannel<int, 3> chC;
    lib::BufferedChannel<int, 4> chD;
    lib::BufferedChannel<int, 5> chE;

    auto worker = [](lib::VOChannel<int> ochannel, int base)
    {
        for(std::size_t i = 0; i < 4; ++i) {
            ochannel.send(base + i * base);
        }
        ochannel.close();
    };
    std::array threads = {
        std::async(worker, lib::VOChannel(chA), 2),
        std::async(worker, lib::VOChannel(chB), 3),
        std::async(worker, lib::VOChannel(chC), 5),
        std::async(worker, lib::VOChannel(chD), 6),
        std::async(worker, lib::VOChannel(chE), 7)
    };
    std::vector<int> values[5];
    auto& outputA = values[0];
    auto& outputB = values[1];
    auto& outputC = values[2];
    auto& outputD = values[3];
    auto& outputE = values[4];
    lib::IChannelAny channelsAB(chA, chB);
    lib::IChannelAny channelsCD(chC, chD);
    lib::IChannelAny ichannels(channelsAB, channelsCD, chE);

    for(const auto result: lib::irange(ichannels)) {
        lib::rswitch(result,
            [&outputA, &outputB](const auto& AB) {
                lib::rswitch(AB,
                    [&outputA](int A) {
                        outputA.push_back(A);
                    },
                    [&outputB](int B) {
                        outputB.push_back(B);
                    }
                );
            },
            [&outputC, &outputD](const auto& CD) {
                lib::rswitch(CD,
                    [&outputC](int C) {
                        outputC.push_back(C);
                    },
                    [&outputD](int D) {
                        outputD.push_back(D);
                    }
                );
            },
            [&outputE](int E) {
                outputE.push_back(E);
            }
        );
    }
    EXPECT_EQ(values[0], (std::vector<int>{2, 4, 6, 8}));
    EXPECT_EQ(values[1], (std::vector<int>{3, 6, 9, 12}));
    EXPECT_EQ(values[2], (std::vector<int>{5, 10, 15, 20}));
    EXPECT_EQ(values[3], (std::vector<int>{6, 12, 18, 24}));
    EXPECT_EQ(values[4], (std::vector<int>{7, 14, 21, 28}));

    for(auto& thread: threads) {
        thread.get();
    }
}

TEST(lib, buffered_channel_mux_mux_all)
{
    lib::BufferedChannel<int, 1> chA;
    lib::BufferedChannel<int, 2> chB;
    lib::BufferedChannel<int, 3> chC;
    lib::BufferedChannel<int, 4> chD;
    lib::BufferedChannel<int, 5> chE;

    auto worker = [](lib::VOChannel<int> ochannel, int base)
    {
        for(std::size_t i = 0; i < 4; ++i) {
            ochannel.send(base + i * base);
        }
        ochannel.close();
    };
    std::array threads = {
        std::async(worker, lib::VOChannel(chA), 2),
        std::async(worker, lib::VOChannel(chB), 3),
        std::async(worker, lib::VOChannel(chC), 5),
        std::async(worker, lib::VOChannel(chD), 6),
        std::async(worker, lib::VOChannel(chE), 7)
    };
    std::vector<int> values[5];
    auto& outputA = values[0];
    auto& outputB = values[1];
    auto& outputC = values[2];
    auto& outputD = values[3];
    auto& outputE = values[4];
    lib::IChannelAll channelsAB(chA, chB);
    lib::IChannelAll channelsCD(chC, chD);
    lib::IChannelAny ichannels(channelsAB, channelsCD, chE);

    for(const auto& [a, b]: lib::irange(channelsAB)) {

    }

    for(const auto result: lib::irange(ichannels)) {
        lib::rswitch(result,
            [&outputA, &outputB](const auto& AB) {
                const auto& [A, B] = AB;
                outputA.push_back(A);
                outputB.push_back(B);
            },
            [&outputC, &outputD](const auto& CD) {
                const auto& [C, D] = CD;
                outputC.push_back(C);
                outputD.push_back(D);
            },
            [&outputE](int E) {
                outputE.push_back(E);
            }
        );
    }
    ASSERT_TRUE(ichannels.closed());
    EXPECT_EQ(values[0], (std::vector<int>{2, 4, 6, 8}));
    EXPECT_EQ(values[1], (std::vector<int>{3, 6, 9, 12}));
    EXPECT_EQ(values[2], (std::vector<int>{5, 10, 15, 20}));
    EXPECT_EQ(values[3], (std::vector<int>{6, 12, 18, 24}));
    EXPECT_EQ(values[4], (std::vector<int>{7, 14, 21, 28}));
    if (values[4].size() < 4) {
        std::cout << "ahtung" << std::endl;
    }

    for(auto& thread: threads) {
        thread.get();
    }
}

TEST(lib, broadcast_channel)
{
    lib::BufferedChannel<int, 2> channels[3];
    lib::BroadCastChannel channel(channels[0], channels[1], channels[2]);
    std::vector<int> values[3];
    auto worker = [](lib::VIChannel<int> ichannel, std::vector<int>& output)
    {
        for(auto value: lib::irange(ichannel)) {
            output.push_back(value);
        }
    };
    std::array threads = {
        std::async(worker, lib::VIChannel(channels[0]), std::ref(values[0])),
        std::async(worker, lib::VIChannel(channels[1]), std::ref(values[1])),
        std::async(worker, lib::VIChannel(channels[2]), std::ref(values[2]))
    };
    channel.send(1);
    channel.send(2);
    channel.send(3);
    channel.send(4);
    channel.close();
    for(auto& thread: threads) {
        thread.get();
    }
    EXPECT_EQ(values[0], (std::vector<int>{1, 2, 3, 4}));
    EXPECT_EQ(values[1], (std::vector<int>{1, 2, 3, 4}));
    EXPECT_EQ(values[2], (std::vector<int>{1, 2, 3, 4}));
}

TEST(lib, aggregate_channel)
{
    lib::BufferedChannel<int, 2> channels[3];
    lib::AggregateChannel channel(channels[0], channels[1], channels[2]);
    std::vector<int> values;
    auto worker = [](lib::VOChannel<int> ochannel, std::vector<int> input)
    {
        for(auto value: input) {
            ochannel.send(value);
        }
        ochannel.close();
    };
    std::array threads = {
        std::async(worker, lib::VOChannel(channels[0]), std::vector<int>{1, 4, 7}),
        std::async(worker, lib::VOChannel(channels[1]), std::vector<int>{2, 5, 8}),
        std::async(worker, lib::VOChannel(channels[2]), std::vector<int>{3, 6, 9})
    };
    for(auto value: lib::irange(channel)) {
        values.push_back(value);
    }
    std::sort(values.begin(), values.end());
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9}));
    for(auto& thread: threads) {
        thread.get();
    }
}
