#include <gtest/gtest.h>
#include <lib/interpreter/constexpr.hpp>

TEST(grammar, test)
{
    using namespace lib::interpreter;

    constexpr auto m = module(
        fn("u32", "foo")(param("u32", "lhs"), param("u32", "rhs"))(
            [](auto& context, const_expr::Pointer sp) noexcept {
                auto& memory = context.memory;
                const auto lhs = memory.load(context["lhs"], lib::tag<std::uint32_t>);
                const auto rhs = memory.load(context["rhs"], lib::tag<std::uint32_t>);
                memory.save(sp, lhs + rhs);
            }
        ),
        fn("u32", "bar")(param("u32", "lhs"), param("u32", "rhs"))(
            //var("a") = fn("foo")(var("lhs"), 5_l),
            //var("b") = fn("foo")(var("lhs"), 5_l),
            //ret(var("a") + var("b") + 10_l)
            ret(var("lhs") + var("rhs"))
        )
    );
    const auto result = const_expr::call(m, "bar", std::uint32_t(1), std::uint32_t(2));
}
