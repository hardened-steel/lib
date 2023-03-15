#pragma once
#include "lib/typetraits/if.hpp"
#include <lib/typetraits/set.hpp>
#include <lib/typetraits/map.hpp>
#include <type_traits>

namespace lib::typetraits {

    template<class ...Actions>
    using Scope = List<Actions...>;

    template<auto IValue>
    struct Value
    {
        constexpr static inline auto value = IValue;
    };

    namespace impl {

        template<class Variables = Map<>, class Parent = void>
        struct Context {};

        template<class Variables>
        struct Context<Variables, void> {};

        template<class Context, class VarName, class Value>
        struct CreateVariableF;

        template<class Context, class VarName, class Value>
        using CreateVariable = typename CreateVariableF<Context, VarName, Value>::Result;

        template<class Variables, class Parent, class VarName, class Value>
        struct CreateVariableF<Context<Variables, Parent>, VarName, Value>
        {
            static_assert(!exists<Variables, VarName>);
            using Result = Context<Insert<Variables, Map<VarName, Value>>, Parent>;
        };

        template<class Context, class VarName, class Value>
        struct WriteVariableF;

        template<class Context, class VarName, class Value>
        using WriteVariable = typename WriteVariableF<Context, VarName, Value>::Result;

        template<class Variables, class VarName, class Value>
        struct WriteVariableF<Context<Variables, void>, VarName, Value>
        {
            static_assert(exists<Variables, VarName>);
            using NewVariables = Insert<Variables, Map<VarName, Value>>;
            using Result = Context<NewVariables, void>;
        };

        template<class Variables, class Parent, class VarName, class Value>
        struct WriteVariableF<Context<Variables, Parent>, VarName, Value>
        {
            using NewParent = If<
                exists<Variables, VarName>,
                Constant, List<Parent>,
                WriteVariable, List<Parent, VarName, Value>
            >;
            using NewVariables = If<
                exists<Variables, VarName>,
                Insert, List<Variables, Map<VarName, Value>>,
                Constant, List<Variables>
            >;
            using Result = Context<NewVariables, NewParent>;
        };

        template<class Context, class VarName>
        struct ReadVariableF;

        template<class Context, class VarName>
        using ReadVariable = typename ReadVariableF<Context, VarName>::Result;

        template<class Variables, class VarName>
        struct ReadVariableF<Context<Variables, void>, VarName>
        {
            static_assert(exists<Variables, VarName>);
            using Result = Get<Variables, index<Variables, VarName>>;
        };

        template<class Variables, class Parent, class VarName>
        struct ReadVariableF<Context<Variables, Parent>, VarName>
        {
            using Result = If<
                exists<Variables, VarName>,
                Find, List<Variables, VarName>,
                ReadVariable, List<Parent, VarName>
            >;
        };

    }

    template<class VarName, class Value>
    struct CreateVariable {};

    template<class VarName, class Value>
    struct Write {};

    namespace impl {
        template<class Lhs, class Rhs>
        struct AddF
        {
            using Result = AddF;
        };

        template<class Lhs, class Rhs>
        using Add = typename AddF<Lhs, Rhs>::Result;

        template<auto Lhs, auto Rhs>
        struct AddF<Value<Lhs>, Value<Rhs>>
        {
            using Result = Value<Lhs + Rhs>;
        };
    }

    template<class Lhs, class Rhs>
    using Add = impl::Add<Lhs, Rhs>;

    template<class Expr>
    struct Return {};

    namespace impl {
        template<class Lhs, class Rhs>
        struct EqF
        {
            using Result = EqF;
        };

        template<class Lhs, class Rhs>
        using Eq = typename EqF<Lhs, Rhs>::Result;

        template<auto Lhs, auto Rhs>
        struct EqF<Value<Lhs>, Value<Rhs>>
        {
            using Result = Value<Lhs == Rhs>;
        };
    }

    template<class Lhs, class Rhs>
    using Eq = impl::Eq<Lhs, Rhs>;

    namespace impl {
        template<class Lhs, class Rhs>
        struct MulF
        {
            using Result = MulF;
        };

        template<class Lhs, class Rhs>
        using Mul = typename MulF<Lhs, Rhs>::Result;

        template<auto Lhs, auto Rhs>
        struct MulF<Value<Lhs>, Value<Rhs>>
        {
            using Result = Value<Lhs * Rhs>;
        };
    }

    template<class Lhs, class Rhs>
    using Mul = impl::Mul<Lhs, Rhs>;

    namespace impl {
        template<class Value>
        struct NotF
        {
            using Result = NotF;
        };

        template<class Value>
        using Not = typename NotF<Value>::Result;

        template<auto value>
        struct NotF<Value<value>>
        {
            using Result = Value<!value>;
        };
    }

    template<class Value>
    using Not = impl::Not<Value>;

    namespace impl {
        template<class Value>
        struct DecF
        {
            using Result = DecF;
        };

        template<class Value>
        using Dec = typename DecF<Value>::Result;

        template<auto value>
        struct DecF<Value<value>>
        {
            using Result = Value<value - 1>;
        };
    }

    template<class Value>
    using Dec = impl::Dec<Value>;

    namespace impl {
        template<class Value>
        struct IncF
        {
            using Result = IncF;
        };

        template<class Value>
        using Inc = typename IncF<Value>::Result;

        template<auto value>
        struct IncF<Value<value>>
        {
            using Result = Value<value + 1>;
        };
    }

    template<class Value>
    using Inc = impl::Inc<Value>;

    template<class Condition, class ...Actions>
    struct If {};

    template<class Condition, class ...Actions>
    struct While {};

    namespace impl {

        template<class Context, class Variable>
        struct CalcF
        {
            using Result = ReadVariable<Context, Variable>;
        };

        template<class Context, class Expr>
        using Calc = typename CalcF<Context, Expr>::Result;

        template<class Context, class Lhs, class Rhs>
        struct CalcF<Context, AddF<Lhs, Rhs>>
        {
            using Result = Calc<Context, Add<Calc<Context, Lhs>, Calc<Context, Rhs>>>;
        };

        template<class Context, class Lhs, class Rhs>
        struct CalcF<Context, MulF<Lhs, Rhs>>
        {
            using Result = Calc<Context, Mul<Calc<Context, Lhs>, Calc<Context, Rhs>>>;
        };

        template<class Context, class Lhs, class Rhs>
        struct CalcF<Context, EqF<Lhs, Rhs>>
        {
            using Result = Calc<Context, Eq<Calc<Context, Lhs>, Calc<Context, Rhs>>>;
        };

        template<class Context, class Expr>
        struct CalcF<Context, NotF<Expr>>
        {
            using Result = Calc<Context, Not<Calc<Context, Expr>>>;
        };

        template<class Context, class Expr>
        struct CalcF<Context, IncF<Expr>>
        {
            using Result = Calc<Context, Inc<Calc<Context, Expr>>>;
        };

        template<class Context, auto value>
        struct CalcF<Context, Value<value>>
        {
            using Result = Value<value>;
        };

        template<class Context, class Statement>
        struct Action;

        template<class Context, class Statement>
        using ActionResult = typename Action<Context, Statement>::Result;

        template<class Context, class VarName, class Value>
        struct Action<Context, lib::typetraits::CreateVariable<VarName, Value>>
        {
            using Result = CreateVariable<Context, VarName, Value>;
            static inline constexpr bool Break = false;
        };

        template<class Context, class VarName, class Expr>
        struct Action<Context, lib::typetraits::Write<VarName, Expr>>
        {
            using Value = Calc<Context, Expr>;
            using Result = WriteVariable<Context, VarName, Value>;
            static inline constexpr bool Break = false;
        };

        template<class Variables, class Parent, class Expr>
        struct Action<Context<Variables, Parent>, lib::typetraits::Return<Expr>>
        {
            using Result = Calc<Context<Variables, Parent>, Expr>;
            static inline constexpr bool Break = true;
        };

        template<class Update>
        struct ActionNext;

        template<class Variables, class Parent>
        struct ActionNext<Context<Variables, Parent>>
        {
            using Result = Parent;
            static inline constexpr bool Break = false;
        };

        template<auto value>
        struct ActionNext<Value<value>>
        {
            using Result = Value<value>;
            static inline constexpr bool Break = true;
        };

        template<class Parent, class Body>
        struct ActionScopeF
        {
            using Result = For<Body, Context<Map<>, Parent>, Action>;
        };

        template<class Parent, class Body>
        using ActionScope = typename ActionScopeF<Parent, Body>::Result;

        template<class Context, class Condition, class ...Actions>
        struct Action<Context, lib::typetraits::If<Condition, Actions...>>
        {
            using Value = Calc<Context, Condition>;
            using IfResult = If<
                Value::value,
                ActionScope, List<Context, Scope<Actions...>>,
                ActionScope, List<Context, Scope<>>
            >;
            using Next = ActionNext<IfResult>;
            using Result = typename Next::Result;
            static inline constexpr bool Break = Next::Break;
        };

        template<class Context, class Condition, class ...Actions>
        struct Action<Context, lib::typetraits::While<Condition, Actions...>>
        {
            using Value = Calc<Context, Condition>;
            using WhileResult = If<
                Value::value,
                ActionScope, List<Context, Scope<Actions...>>,
                ActionScope, List<Context, Scope<>>
            >;
            using Next = ActionNext<WhileResult>;
            static inline constexpr bool Break = Next::Break || !Value::value;
            using Result = If<
                !Break,
                ActionResult, List<typename Next::Result, lib::typetraits::While<Condition, Actions...>>,
                Constant, List<typename Next::Result>
            >;
        };

        template<template<class...> class Function, class ...Params>
        struct CallF
        {
            using Body = typename Function<Params...>::Body;
            // Create Context with params
            using Result = ActionScope<void, Body>;
        };

        template<template<class...> class Function, class ...Params>
        using Call = typename CallF<Function, Params...>::Result;
    }

    template<template<class...> class Function, class ...Params>
    using Call = impl::Call<Function, Params...>;

}
