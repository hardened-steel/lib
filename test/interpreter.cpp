#include <gtest/gtest.h>
#include <lib/typename.hpp>
#include <lib/literal.hpp>
#include <lib/concept.hpp>
#include <lib/static.string.hpp>
#include <lib/interpreter/core.hpp>
#include <iostream>

namespace lib::interpreter {

    template<class Constructors, class Destructor, class Functions>
    struct TypeInfo
    {
        std::uint32_t    size;
        //std::string_view name;

        Constructors constructors;
        Destructor   destructor;
        Functions    functions;
    };

    struct Expression {};

    template<class T>
    struct Literal: Expression
    {
        T value;

        template<class Context>
        constexpr void calc(Context& ctx) const noexcept
        {
            ctx.push(value);
        }

        friend constexpr static bool operator==(const Literal& lhs, const Literal& rhs) noexcept
        {
            return lhs.value == rhs.value;
        }

        constexpr explicit Literal(T value) noexcept
        : value(value)
        {}
        constexpr Literal(const Literal& other) = default;
        constexpr Literal& operator=(const Literal& other) = default;
    };

    template<char ...Chars>
    constexpr auto operator ""_l() noexcept
    {
        using Parser = lib::literal::Parser<Chars...>;
        return Literal<typename Parser::Type>(Parser::value);
    }

    template<class Lhs, class Rhs>
    struct Sum: Expression
    {
        Lhs lhs;
        Rhs rhs;

        template<class Context>
        constexpr void calc(Context& ctx) const
        {
            lhs.calc(ctx);
            rhs.calc(ctx);
            ctx.add();
        }

        constexpr explicit Sum(const Lhs& lhs, const Rhs& rhs) noexcept
        : lhs(lhs), rhs(rhs)
        {}
        constexpr Sum(const Sum& other) = default;
        constexpr Sum& operator=(const Sum& other) = default;
    };

    template<class Lhs, class Rhs, typename = lib::Require<std::is_base_of_v<Expression, Lhs>, std::is_base_of_v<Expression, Rhs>>>
    constexpr auto operator+(const Lhs& lhs, const Rhs& rhs) noexcept
    {
        return Sum<Lhs, Rhs>(lhs, rhs);
    }

    template<std::size_t N>
    struct Variable: Expression
    {
        lib::StaticString<N> name;

        template<class Context>
        constexpr void calc(Context& ctx) const noexcept
        {
            ctx.get(name);
        }

        constexpr explicit Variable(lib::StaticString<N> name) noexcept
        : name(name)
        {}
        constexpr Variable(const Variable& other) = default;
        constexpr Variable& operator=(const Variable& other) = default;
    };

    struct Var
    {
        template<std::size_t N>
        constexpr auto operator()(const char (&string)[N]) const noexcept
        {
            return Variable(lib::StaticString(string));
        }
    };
    constexpr inline Var var;

    struct Statement {};

    template<class ...Statements>
    struct Scope: Statement
    {
        std::tuple<Statements...> statemets;

        constexpr explicit Scope(const Statements& ...statemets) noexcept
        : statemets(statemets...)
        {}
        constexpr explicit Scope(const std::tuple<Statements...>& statemets) noexcept
        : statemets(statemets)
        {}
        constexpr Scope(const Scope& other) = default;
        constexpr Scope& operator=(const Scope& other) = default;
    };

    class CalcContext: Context<256, 128>
    {
        using Base = Context<256, 128>;
        std::uint32_t sp = 128;
    public:
        template<class T>
        constexpr auto push(T value) noexcept
        {
            Base::push(sp, value);
            return sp &= 0xFFFFFFC;
        }

        constexpr auto add(std::uint32_t lhs, std::uint32_t rhs) noexcept
        {

        }
    };
}

TEST(grammar, test)
{
    using namespace lib::interpreter;
    lib::interpreter::CalcContext context;
    EXPECT_EQ(10_l, Literal<std::uint8_t>{10});
    constexpr auto expr = 10_l + 10_l;
    expr.calc(context);
    constexpr auto a = var("a");
}
