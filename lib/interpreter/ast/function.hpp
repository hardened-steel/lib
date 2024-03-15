#pragma once
#include <lib/interpreter/ast/statement.hpp>
#include <lib/interpreter/ast/expression.hpp>
#include <lib/interpreter/ast/scope.hpp>
#include <lib/static.string.hpp>
#include <lib/concept.hpp>

namespace lib::interpreter {
    namespace ast {
        template<std::size_t T, std::size_t N>
        struct Parameter: Expression
        {
            lib::StaticString<T> type;
            lib::StaticString<N> name;

            constexpr explicit Parameter(lib::StaticString<T> type, lib::StaticString<N> name) noexcept
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
            template<std::size_t T, std::size_t N>
            constexpr auto operator()(const char (&type)[T], const char (&name)[N]) const noexcept
            {
                return Parameter<T - 1, N - 1>(type, name);
            }
        };

        template<std::size_t R, std::size_t N, class Params, class Body>
        struct FunctionDefinition
        {
            lib::StaticString<R> rtype;
            lib::StaticString<N> name;
            Params               params;
            Body                 body;

            constexpr explicit FunctionDefinition
            (
                lib::StaticString<R> rtype, lib::StaticString<N> name,
                const Params& params, const Body& body
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

        template<std::size_t R, std::size_t N, class ...Params>
        struct FunctionDeclaration
        {
            lib::StaticString<R>  rtype;
            lib::StaticString<N>  name;
            std::tuple<Params...> params;

            constexpr explicit FunctionDeclaration
            (
                lib::StaticString<R> rtype, lib::StaticString<N> name,
                const Params& ...params
            )
            noexcept
            : rtype(rtype), name(name), params(params...)
            {}
            constexpr explicit FunctionDeclaration
            (
                lib::StaticString<R> rtype, lib::StaticString<N> name,
                const std::tuple<Params...>& params
            )
            noexcept
            : rtype(rtype), name(name), params(params)
            {}

            constexpr FunctionDeclaration(const FunctionDeclaration& other) = default;
            constexpr FunctionDeclaration& operator=(const FunctionDeclaration& other) = default;

            template<class ...Statements, typename = lib::Require<std::is_base_of_v<Statement, Statements>...>>
            constexpr auto operator()(const Statements& ...statements) const noexcept
            {
                return FunctionDefinition<R, N, std::tuple<Params...>, Scope<Statements...>> (
                    rtype, name, params, scope(statements...)
                );
            }

            template<class Function, typename = lib::Require<!std::is_base_of_v<Statement, Function>>>
            constexpr auto operator()(const Function& function) const noexcept
            {
                return FunctionDefinition<R, N, std::tuple<Params...>, Function>(rtype, name, params, function);
            }

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        template<std::size_t R, std::size_t N>
        struct FunctionName
        {
            lib::StaticString<R> rtype;
            lib::StaticString<N> name;

            constexpr explicit FunctionName(lib::StaticString<R> rtype, lib::StaticString<N> name) noexcept
            : rtype(rtype), name(name)
            {}
            constexpr FunctionName(const FunctionName& other) = default;
            constexpr FunctionName& operator=(const FunctionName& other) = default;

            template<class ...Params>
            constexpr auto operator()(const Params& ...params) const noexcept
            {
                return FunctionDeclaration<R, N, Params...>(rtype, name, params...);
            }

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        template<class ...Params>
        struct FunctionCall: Expression
        {
            std::string_view      name;
            std::tuple<Params...> params;

            constexpr explicit FunctionCall(std::string_view name, const Params& ...params) noexcept
            : name(name), params(params...)
            {}
            constexpr FunctionCall(const FunctionCall& other) = default;
            constexpr FunctionCall& operator=(const FunctionCall& other) = default;

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        struct GetFunction
        {
            std::string_view name;

            constexpr explicit GetFunction(std::string_view name) noexcept
            : name(name)
            {}
            constexpr GetFunction(const GetFunction& other) = default;
            constexpr GetFunction& operator=(const GetFunction& other) = default;

            template<class ...Params, typename = lib::Require<std::is_base_of_v<Expression, Params>...>>
            constexpr auto operator()(const Params& ...params) const noexcept
            {
                return FunctionCall<Params...>(name, params...);
            }

            template<class Context, class ...TArgs>
            constexpr auto operator()(Context& ctx, TArgs&& ...args) const
            {
                return ctx(*this, std::forward<TArgs>(args)...);
            }
        };

        struct Function
        {
            template<std::size_t R, std::size_t N>
            constexpr auto operator()(const char (&rtype)[R], const char (&name)[N]) const noexcept
            {
                return FunctionName<R - 1, N - 1>(rtype, name);
            }

            constexpr auto operator()(std::string_view name) const noexcept
            {
                return GetFunction(name);
            }
        };
    }

    constexpr inline ast::Param param;
    constexpr inline ast::Function fn;
}
