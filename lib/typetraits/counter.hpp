#pragma once
#include <lib/typetraits/var.hpp>


namespace lib::typetraits {
    namespace details {
        template <StaticString Name, class Tag>
        constexpr std::size_t counter() noexcept
        {
            const auto init = set<Name, std::size_t(0), void>;
            return set<Name, init + get<Name, Tag> + 1, Tag> - 1;
        }
    }

    template <StaticString Name, class Tag>
    constexpr auto counter = details::counter<Name, Tag>();

    static_assert(counter<"lib-test-counter", VTag<next()>> == 0);
    static_assert(counter<"lib-test-counter", VTag<next()>> == 1);
    static_assert(counter<"lib-test-counter", VTag<next()>> == 2);
    static_assert(counter<"lib-test-counter", VTag<next()>> == 3);
    static_assert(counter<"lib-test-counter", VTag<next()>> == 4);
}
