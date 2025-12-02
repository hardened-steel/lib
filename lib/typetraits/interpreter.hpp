#pragma once
#include <lib/typename.hpp>
#include <lib/array.hpp>
#include <lib/concept.hpp>
#include <lib/typetraits/if.hpp>
#include <lib/typetraits/set.hpp>
#include <lib/typetraits/map.hpp>


namespace lib::typetraits::interpreter {
    template <class ...Actions>
    using Scope = List<Actions...>;

    template <auto Message>
    struct Error;

    template <std::size_t N, const StaticString<N>* Message>
    struct Error<Message>
    {
        constexpr static inline auto message = *Message; 
    };

    namespace impl {

        template <class Variables = Map<>, class Parent = void>
        struct Context {};

        template <class Variables>
        struct Context<Variables, void> {};

        template <class Context, class VarName, class Value>
        struct CreateVariableF;

        template <class Context, class VarName, class Value>
        using CreateVariable = typename CreateVariableF<Context, VarName, Value>::Result;

        template <class Variables, class Parent, class VarName, class Value>
        struct CreateVariableF<Context<Variables, Parent>, VarName, Value>
        {
            constexpr static inline auto message = "variable \'" + lib::type_name<VarName> + "\' already exists in this scope";
            using Result = If<
                !exists<Variables, VarName>,
                Context, List<Insert<Variables, Map<VarName, Value>>, Parent>,
                Constant, List<Error<&message>>
            >;
        };

        template <class Context, class VarName, class Value>
        struct WriteVariableF;

        template <class Context, class VarName, class Value>
        using WriteVariable = typename WriteVariableF<Context, VarName, Value>::Result;

        template <class Variables, class VarName, class Value>
        struct WriteVariableF<Context<Variables, void>, VarName, Value>
        {
            using NewVariables = Insert<Variables, Map<VarName, Value>>;
            constexpr static inline auto message = "write to undefined variable \'" + lib::type_name<VarName> + "\'";
            using Result = If<
                exists<Variables, VarName>,
                Context, List<Insert<Variables, Map<VarName, Value>>, void>,
                Constant, List<Error<&message>>
            >;
        };

        template <class Variables, class Parent, class VarName, class Value>
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

        template <class Context, class VarName>
        struct ReadVariableF;

        template <class Context, class VarName>
        using ReadVariable = typename ReadVariableF<Context, VarName>::Result;

        template <class Variables, class VarName>
        struct ReadVariableF<Context<Variables, void>, VarName>
        {
            constexpr static inline auto message = "undefined variable \'" + lib::type_name<VarName> + "\'";
            using Result = If<
                exists<Variables, VarName>,
                Find, List<Variables, VarName>,
                Constant, List<Error<&message>>
            >;
        };

        template <class Variables, class Parent, class VarName>
        struct ReadVariableF<Context<Variables, Parent>, VarName>
        {
            using Result = If<
                exists<Variables, VarName>,
                Find, List<Variables, VarName>,
                ReadVariable, List<Parent, VarName>
            >;
        };

    }

    template <class VarName, class Value>
    struct CreateVariable {};

    template <class VarName, class Value>
    struct Write {};

    namespace impl {
        template <class Lhs, class Rhs>
        struct AddF
        {
            using Result = AddF;
        };

        template <class Lhs, class Rhs>
        using Add = typename AddF<Lhs, Rhs>::Result;

        template <auto Lhs, auto Rhs>
        struct AddF<Value<Lhs>, Value<Rhs>>
        {
            using Result = Value<Lhs + Rhs>;
        };

        template <auto Lhs, class Rhs>
        struct AddF<Error<Lhs>, Rhs>
        {
            using Result = Error<Lhs>;
        };

        template <class Lhs, auto Rhs>
        struct AddF<Lhs, Error<Rhs>>
        {
            using Result = Error<Rhs>;
        };
    }

    template <class Lhs, class Rhs>
    using Add = impl::Add<Lhs, Rhs>;

    template <class Expr>
    struct Return {};

    namespace impl {
        template <class Lhs, class Rhs>
        struct EqF
        {
            using Result = EqF;
        };

        template <class Lhs, class Rhs>
        using Eq = typename EqF<Lhs, Rhs>::Result;

        template <auto Lhs, auto Rhs>
        struct EqF<Value<Lhs>, Value<Rhs>>
        {
            using Result = Value<Lhs == Rhs>;
        };
    }

    template <class Lhs, class Rhs>
    using Eq = impl::Eq<Lhs, Rhs>;

    namespace impl {
        template <class Lhs, class Rhs>
        struct MulF
        {
            using Result = MulF;
        };

        template <class Lhs, class Rhs>
        using Mul = typename MulF<Lhs, Rhs>::Result;

        template <auto Lhs, auto Rhs>
        struct MulF<Value<Lhs>, Value<Rhs>>
        {
            using Result = Value<Lhs * Rhs>;
        };
    }

    template <class Lhs, class Rhs>
    using Mul = impl::Mul<Lhs, Rhs>;

    namespace impl {
        template <class Value>
        struct NotF
        {
            using Result = NotF;
        };

        template <class Value>
        using Not = typename NotF<Value>::Result;

        template <auto IValue>
        struct NotF<Value<IValue>>
        {
            using Result = Value<!IValue>;
        };
    }

    template <class Value>
    using Not = impl::Not<Value>;

    namespace impl {
        template <class Value>
        struct DecF
        {
            using Result = DecF;
        };

        template <class Value>
        using Dec = typename DecF<Value>::Result;

        template <auto IValue>
        struct DecF<Value<IValue>>
        {
            using Result = Value<IValue - 1>;
        };
    }

    template <class Value>
    using Dec = impl::Dec<Value>;

    namespace impl {
        template <class Value>
        struct IncF
        {
            using Result = IncF;
        };

        template <class Value>
        using Inc = typename IncF<Value>::Result;

        template <auto IValue>
        struct IncF<Value<IValue>>
        {
            using Result = Value<IValue + 1>;
        };
    }

    template <class Value>
    using Inc = impl::Inc<Value>;

    template <class Condition, class ...Actions>
    struct If {};

    template <class Condition, class ...Actions>
    struct While {};

    namespace impl {

        template <class Value, class Statement>
        struct Action
        {
            using Result = Value;
            static inline constexpr bool Break = true; // NOLINT
        };

        template <class Parent, class Body>
        struct ActionScopeF
        {
            using Result = For<Body, Context<Map<>, Parent>, Action>;
        };

        template <class Parent, class Body>
        using ActionScope = typename ActionScopeF<Parent, Body>::Result;

        template <class Function, class Context>
        struct CallF
        {
            using Body = typename Function::Body;
            using Result = ActionScope<Context, Body>;
        };

        template <class Function, class Context = void>
        using Call = typename CallF<Function, Context>::Result;

        template <typename T>
        struct IsFunctionF
        {
            template <typename U, class Body> struct SFINAE {};
            template <typename U> static char Test(SFINAE<U, typename U::Body>*);
            template <typename U> static int Test(...);
            constexpr static inline bool Result = sizeof(Test<T>(nullptr)) == sizeof(char); // NOLINT
        };

        template <class T>
        constexpr inline bool IsFunction = IsFunctionF<T>::Result; // NOLINT

        template <class Context, class Name>
        struct CalcF
        {
            using Result = lib::typetraits::If<
                IsFunction<Name>,
                Call, List<Name, Context>,
                ReadVariable, List<Context, Name>
            >;
        };

        template <class Context, class Expr>
        using Calc = typename CalcF<Context, Expr>::Result;

        template <class Context, class Lhs, class Rhs>
        struct CalcF<Context, AddF<Lhs, Rhs>>
        {
            using Result = Calc<Context, Add<Calc<Context, Lhs>, Calc<Context, Rhs>>>;
        };

        template <class Context, class Lhs, class Rhs>
        struct CalcF<Context, MulF<Lhs, Rhs>>
        {
            using Result = Calc<Context, Mul<Calc<Context, Lhs>, Calc<Context, Rhs>>>;
        };

        template <class Context, class Lhs, class Rhs>
        struct CalcF<Context, EqF<Lhs, Rhs>>
        {
            using Result = Calc<Context, Eq<Calc<Context, Lhs>, Calc<Context, Rhs>>>;
        };

        template <class Context, class Expr>
        struct CalcF<Context, NotF<Expr>>
        {
            using Result = Calc<Context, Not<Calc<Context, Expr>>>;
        };

        template <class Context, class Expr>
        struct CalcF<Context, IncF<Expr>>
        {
            using Result = Calc<Context, Inc<Calc<Context, Expr>>>;
        };

        template <class Context, auto IValue>
        struct CalcF<Context, Value<IValue>>
        {
            using Result = Value<IValue>;
        };

        template <class Context, auto IValue>
        struct CalcF<Context, Error<IValue>>
        {
            using Result = Error<IValue>;
        };

        template <class Context>
        struct CheckContext
        {
            using Result = Context;
            static inline constexpr bool Break = true; // NOLINT
        };

        template <class Variables, class Parent>
        struct CheckContext<Context<Variables, Parent>>
        {
            using Result = Parent;
            static inline constexpr bool Break = false; // NOLINT
        };

        template <class Context, class Statement>
        using ActionResult = typename Action<Context, Statement>::Result;

        template <class Context, class VarName, class Value>
        struct Action<Context, lib::typetraits::interpreter::CreateVariable<VarName, Value>>
        {
            using Result = CreateVariable<Context, VarName, Value>;
            static inline constexpr bool Break = false; // NOLINT
        };

        template <class Context, class VarName, class Expr>
        struct Action<Context, lib::typetraits::interpreter::Write<VarName, Expr>>
        {
            using Value = Calc<Context, Expr>;
            using Result = WriteVariable<Context, VarName, Value>;
            static inline constexpr bool Break = false; // NOLINT
        };

        template <class Variables, class Parent, class Expr>
        struct Action<Context<Variables, Parent>, lib::typetraits::interpreter::Return<Expr>>
        {
            using Result = Calc<Context<Variables, Parent>, Expr>;
            static inline constexpr bool Break = true; // NOLINT
        };

        template <class Context, class Condition, class ...Actions>
        struct Action<Context, lib::typetraits::interpreter::If<Condition, Actions...>>
        {
            using Value = Calc<Context, Condition>;
            using IfResult = lib::typetraits::If<
                Value::value,
                ActionScope, List<Context, Scope<Actions...>>,
                ActionScope, List<Context, Scope<>>
            >;
            using Next = CheckContext<IfResult>;
            using Result = typename Next::Result;
            static inline constexpr bool Break = Next::Break; // NOLINT
        };

        template <class Context, class Condition, class ...Actions>
        struct Action<Context, lib::typetraits::interpreter::While<Condition, Actions...>>
        {
            using Value = Calc<Context, Condition>;
            using WhileResult = lib::typetraits::If<
                Value::value,
                ActionScope, List<Context, Scope<Actions...>>,
                ActionScope, List<Context, Scope<>>
            >;
            using Next = CheckContext<WhileResult>;
            static inline constexpr bool Break = Next::Break || !Value::value; // NOLINT
            using Result = lib::typetraits::If<
                !Break,
                ActionResult, List<typename Next::Result, lib::typetraits::interpreter::While<Condition, Actions...>>,
                Constant, List<typename Next::Result>
            >;
        };
    }

    namespace impl {
        struct ForIt;

        template <class List, class Value>
        struct ListGet;

        template <class List, auto Index>
        struct ListGet<List, Value<Index>>
        {
            using Result = Get<List, Index>;
        };

        template <class List>
        struct ListSize
        {
            using Result = Value<List::size>;
        };
    }

    template <class Var, class List, class ...Actions>
    using For = If<Value<true>,
        CreateVariable<impl::ForIt, Value<0>>,
        While<Not<Eq<impl::ForIt, Value<List::size>>>,
            /*CreateVariable<Var, impl::ListGet<List, impl::ForIt>>,
            Actions...,*/
            Inc<impl::ForIt>
        >
    >;

    template <class Function>
    using Call = impl::Call<Function, void>;

    struct Function
    {
        template <class Function>
        using Call = lib::typetraits::interpreter::Call<Function>;

        template <class Value>
        using Dec = lib::typetraits::interpreter::impl::Dec<Value>;

        template <class Value>
        using Inc = lib::typetraits::interpreter::impl::Inc<Value>;

        template <class Condition, class ...Actions>
        using If = lib::typetraits::interpreter::If<Condition, Actions...>;

        template <class Condition, class ...Actions>
        using While = lib::typetraits::interpreter::While<Condition, Actions...>;

        template <class Var, class List, class ...Actions>
        using For = lib::typetraits::interpreter::For<Var, List, Actions...>;

        template <class Value>
        using Not = lib::typetraits::interpreter::impl::Not<Value>;

        template <class Lhs, class Rhs>
        using Mul = lib::typetraits::interpreter::impl::Mul<Lhs, Rhs>;

        template <class Lhs, class Rhs>
        using Eq = lib::typetraits::interpreter::impl::Eq<Lhs, Rhs>;

        template <class Lhs, class Rhs>
        using Add = lib::typetraits::interpreter::impl::Add<Lhs, Rhs>;

        template <class Expr>
        using Return = lib::typetraits::interpreter::Return<Expr>;

        template <class VarName, class Value>
        using CreateVariable = lib::typetraits::interpreter::CreateVariable<VarName, Value>;

        template <class VarName, class Value>
        using Write = lib::typetraits::interpreter::Write<VarName, Value>;

        template <class ...Actions>
        using Scope = lib::typetraits::List<Actions...>;

        template <auto IValue>
        using Value = lib::typetraits::Value<IValue>;
    };

    namespace test {
        template <class Param>
        struct Factorial: interpreter::Function
        {
            struct Result;
            struct Counter;
            using Body = Scope<
                CreateVariable<Result, Value<1>>,
                If<Eq<Param, Value<0>>,
                    Return<Result>
                >,
                CreateVariable<Counter, Value<1>>,
                While<Not<Eq<Counter, Param>>,
                    Write<Result, Mul<Result, Counter>>,
                    Write<Counter, Inc<Counter>>
                >,
                Return<Mul<Result, Counter>>
            >;
        };

        struct ErrorFunction: interpreter::Function
        {
            struct Variable;
            using Body = Scope<
                Write<Variable, Value<42>>,
                Return<Variable>
            >;
        };

        template <class A, class B>
        struct FooFunction: interpreter::Function
        {
            using Body = Scope<
                Return<Add<A, B>>
            >;
        };

        template <class IValue>
        struct BarFunction: interpreter::Function
        {
            struct Result;
            using Body = Scope<
                CreateVariable<Result, IValue>,
                Return<Add<FooFunction<Result, Value<42>>, Value<0>>>
            >;
        };

        static_assert(Call<Factorial<Value<5>>>::value == 120);
        static_assert(Call<Factorial<Value<4>>>::value == 24);
        static_assert(Call<Factorial<Value<0>>>::value == 1);
        static_assert(Call<ErrorFunction>::message == "write to undefined variable 'lib::typetraits::interpreter::test::ErrorFunction::Variable'");
        static_assert(Call<BarFunction<Value<10>>>::value == 52);
        static_assert(lib::typetraits::interpreter::impl::IsFunction<BarFunction<Value<10>>>);
    }
}
