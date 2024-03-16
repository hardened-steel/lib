#pragma once
#include <lib/interpreter/ast/statement.hpp>
#include <lib/interpreter/ast/expression.hpp>
#include <lib/interpreter/ast/scope.hpp>
#include <lib/static.string.hpp>
#include <lib/concept.hpp>

namespace lib::interpreter {
    namespace ast {
        struct Parameter
        {
            std::string_view type;
            std::string_view name;

            constexpr explicit Parameter(std::string_view type, std::string_view name) noexcept
            : type(type), name(name)
            {}
            constexpr Parameter(const Parameter& other) = default;
            constexpr Parameter& operator=(const Parameter& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        struct Param
        {
            constexpr auto operator()(std::string_view type, std::string_view name) const noexcept
            {
                return Parameter(type, name);
            }
        };

        template<class Name, class RType, class Parameters, class Body>
        struct FunctionDefinition
        {
            RType      rtype;
            Name       name;
            Parameters params;
            Body       body;

            constexpr explicit FunctionDefinition
            (
                const RType& rtype, const Name& name,
                const Parameters& params, const Body& body
            )
            noexcept
            : rtype(rtype), name(name), params(params), body(body)
            {}
            constexpr FunctionDefinition(const FunctionDefinition& other) = default;
            constexpr FunctionDefinition& operator=(const FunctionDefinition& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        template<class Fn>
        struct FunctorBody: Statement
        {
            Fn functor;

            template<class F>
            constexpr explicit FunctorBody(F&& functor) noexcept
            : functor(std::forward<F>(functor))
            {}
            constexpr FunctorBody(const FunctorBody& other) = default;
            constexpr FunctorBody& operator=(const FunctorBody& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        template<class Name, class RType, class Parameters>
        struct FunctionDeclaration
        {
            Name       rtype;
            RType      name;
            Parameters params;

            constexpr explicit FunctionDeclaration
            (
                const RType& rtype, const Name& name,
                const Parameters& params
            )
            noexcept
            : rtype(rtype), name(name), params(params)
            {}

            constexpr FunctionDeclaration(const FunctionDeclaration& other) = default;
            constexpr FunctionDeclaration& operator=(const FunctionDeclaration& other) = default;

            template<class ...Statements, typename = lib::Require<std::is_base_of_v<Statement, Statements>...>>
            constexpr auto operator()(const Statements& ...statements) const noexcept
            {
                return FunctionDefinition(rtype, name, params, scope(statements...));
            }

            template<class Function, typename = lib::Require<!std::is_base_of_v<Statement, Function>>>
            constexpr auto operator()(const Function& function) const noexcept
            {
                return FunctionDefinition(rtype, name, params, FunctorBody<Function>(function));
            }

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        template<class Name, class RType>
        struct FunctionName
        {
            RType rtype;
            Name  name;

            constexpr FunctionName(const RType& rtype, const Name& name) noexcept
            : rtype(rtype), name(name)
            {}
            constexpr FunctionName(const FunctionName& other) = default;
            constexpr FunctionName& operator=(const FunctionName& other) = default;

            template<class ...Params, typename = lib::Require<std::is_same_v<Parameter, Params> ...>>
            constexpr auto operator()(const Params& ...params) const noexcept
            {
                return FunctionDeclaration(
                    rtype, name,
                    std::array<Parameter, sizeof...(Params)>{params...}
                );
            }

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        template<class Name, class Params>
        struct FunctionCall: Expression
        {
            Name   name;
            Params params;

            template<class T>
            constexpr explicit FunctionCall(T&& name, const Params& params) noexcept
            : name(std::forward<T>(name)), params(params)
            {}
            constexpr FunctionCall(const FunctionCall& other) = default;
            constexpr FunctionCall& operator=(const FunctionCall& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        template<class Name>
        struct GetFunction
        {
            Name name;

            template<class T>
            constexpr explicit GetFunction(T&& name) noexcept
            : name(std::forward<T>(name))
            {}
            constexpr GetFunction(const GetFunction& other) = default;
            constexpr GetFunction& operator=(const GetFunction& other) = default;

            template<class ...Params, typename = lib::Require<std::is_base_of_v<Expression, Params>...>>
            constexpr auto operator()(const Params& ...params) const noexcept
            {
                return FunctionCall<Name, std::tuple<Params...>>(name, std::make_tuple(params...));
            }

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        struct Function
        {
            template<class Name, class RType>
            constexpr auto operator()(const RType& rtype, const Name& name) const noexcept
            {
                using N = std::conditional_t<std::is_array_v<Name>, lib::StaticString<std::extent_v<Name> - 1>, Name>;
                using R = std::conditional_t<std::is_array_v<RType>, lib::StaticString<std::extent_v<RType> - 1>, RType>;
                return FunctionName<N, R>(rtype, name);
            }

            template<class Name>
            constexpr auto operator()(const Name& name) const noexcept
            {
                using N = std::conditional_t<std::is_array_v<Name>, lib::StaticString<std::extent_v<Name> - 1>, Name>;
                return GetFunction<N>(name);
            }
        };
    }

    constexpr inline ast::Param param;
    constexpr inline ast::Function fn;
}
