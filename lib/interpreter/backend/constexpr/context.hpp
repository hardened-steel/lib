#pragma once
#include <string_view>
#include <lib/interpreter/backend/constexpr/root.hpp>
#include <lib/interpreter/backend/constexpr/typeinfo.hpp>
#include <lib/interpreter/backend/constexpr/memory.hpp>
#include <lib/lazy.string.hpp>

namespace lib::interpreter::const_expr {
    struct VariableInfo
    {
        std::string_view name;
        std::string_view type;
        Pointer ptr = 0;
    };

    class Context;

    class FunctionPtr
    {
        const ast::Statement* body;
        void (*ptr)(const ast::Statement&, Context&, Pointer&);
    public:
        template <class FunctionBody>
        constexpr FunctionPtr(const FunctionBody& body) noexcept
        : body(&body)
        , ptr(
            [](const ast::Statement& body, Context& ctx, Pointer& sp) {
                static_cast<const FunctionBody&>(body)(ctx, sp);
            }
        )
        {}

        constexpr FunctionPtr(const FunctionPtr& other) = default;
        constexpr FunctionPtr& operator=(const FunctionPtr& other) = default;

        constexpr void operator()(Context& context, Pointer& sp) const
        {
            ptr(*body, context, sp);
        }
    };

    struct FunctionInfo
    {
        std::string_view name;
        std::string_view rtype;
        lib::span<const ast::Parameter> params;
        FunctionPtr ptr;
    };

    struct TypeInfo
    {
        std::string_view name;
        std::size_t      size;
    };

    struct ScopeInfo
    {
        lib::span<FunctionInfo> functions;
        lib::span<VariableInfo> variables;
        lib::span<TypeInfo> types;
    };

    class Context
    {
        Context* parent;
        ScopeInfo scope;
    public:
        Memory& memory;
    private:
        template <class Context, class Name>
        constexpr static auto& get_variable(Context* ctx, const Name& name)
        {
            if (ctx == nullptr) {
                throw std::out_of_range("indentifier not found");
            }
            for (auto& var: ctx->scope.variables) {
                if (var.name == name) {
                    return var;
                }
            }
            return get_variable(ctx->get_parent(), name);
        }
        template <class Context, class Name>
        constexpr static auto& get_type(Context* ctx, const Name& name)
        {
            if (ctx == nullptr) {
                throw std::out_of_range("type not found");
            }
            for (auto& type: ctx->scope.types) {
                if (type.name == name) {
                    return type;
                }
            }
            return get_type(ctx->get_parent(), name);
        }
        template <class Context, class Name>
        constexpr static auto& get_function(Context* ctx, const Name& name, lib::span<const std::string_view> ptypes)
        {
            if (ctx == nullptr) {
                throw std::out_of_range("function not found");
            }
            for (auto& function: ctx->scope.functions) {
                if (function.name == name) {
                    if (function.params.size() == ptypes.size()) {
                        bool found = true;
                        for (std::size_t i = 0; i < ptypes.size(); ++i) {
                            if (function.params[i].type != ptypes[i]) {
                                found = false;
                                break;
                            }
                        }
                        if (found) {
                            return function;
                        }
                    }
                }
            }
            return get_function(ctx->get_parent(), name, ptypes);
        }
    public:
        constexpr Context(Context* parent, Memory& memory, ScopeInfo scope) noexcept
        : parent(parent)
        , scope(scope)
        , memory(memory)
        {}

        constexpr const Context* get_parent() const noexcept
        {
            return parent;
        }

        constexpr Context* get_parent() noexcept
        {
            return parent;
        }

        constexpr Pointer operator[](std::string_view name) const
        {
            return get_variable(this, name).ptr;
        }
    public:
        // expressions
        template <class Lhs, class Rhs>
        constexpr TypeInfo type(const ast::Sum<Lhs, Rhs>& sum)
        {
            return type(fn("operator: lhs + rhs")(sum.lhs, sum.rhs));
        }
        template <class Lhs, class Rhs>
        constexpr TypeInfo operator()(const ast::Sum<Lhs, Rhs>& sum, Pointer& sp)
        {
            return fn("operator: lhs + rhs")(sum.lhs, sum.rhs)(*this, sp);
        }

        constexpr TypeInfo type(const ast::Variable& var)
        {
            auto& var_info = get_variable(this, var.name);
            return get_type(this, var_info.type);
        }
        constexpr TypeInfo operator()(const ast::Variable& var, Pointer& sp)
        {
            auto& var_info = get_variable(this, var.name);
            auto& typeinfo = get_type(this, var_info.type);
            sp -= typeinfo.size;
            const auto name = typeinfo.name + lib::string("::constructor");
            auto ptr = sp;
            fn(name)(literal(sp), literal(var_info.ptr))(*this, ptr);
            return typeinfo;
        }

        template <class T>
        constexpr TypeInfo type(const ast::Literal<T>& literal)
        {
            using Type = TypeTrait<T>;
            return get_type(this, Type::name);
        }
        template <class T>
        constexpr TypeInfo operator()(const ast::Literal<T>& literal, Pointer& sp)
        {
            using Type = TypeTrait<T>;
            std::cout << "push[" << (sp - Type::size) << "]: " << literal.value << std::endl;
            Type::save(memory, sp -= Type::size, literal.value);
            return get_type(this, Type::name);
        }

        template <class Name, class Params, std::size_t ...I>
        constexpr TypeInfo type(const ast::FunctionCall<Name, Params>& fcall, std::index_sequence<I...>)
        {
            const std::array ptypes = {
                type(std::get<I>(fcall.params)).name
                ...
            };
            const FunctionInfo& function = get_function(this, fcall.name, ptypes);
            return get_type(this, function.rtype);
        }
        template <class Name, class Params, std::size_t ...I>
        constexpr TypeInfo type(const ast::FunctionCall<Name, Params>& fcall)
        {
            return type(fcall, std::make_index_sequence<std::tuple_size_v<Params>>{});
        }
        template <class Name, class Params, std::size_t ...I>
        constexpr TypeInfo call(const ast::FunctionCall<Name, Params>& fcall, Pointer& sp, std::index_sequence<I...>)
        {

            const std::array ptypes = {
                type(std::get<I>(fcall.params)).name
                ...
            };
            const FunctionInfo& function = get_function(this, fcall.name, ptypes);
            sp -= get_type(this, function.rtype).size;

            auto ptr = sp;
            std::array args = {
                VariableInfo {
                    "return::value",
                    function.rtype,
                    sp
                },
                VariableInfo {
                    function.params[I].name,
                    ptypes[I],
                    (std::get<I>(fcall.params)(*this, ptr), ptr)
                } ...
            };

            Context context(this, memory, ScopeInfo{{}, args, {}});
            std::cout << "function call[" << sp << "]: " << fcall.name << std::endl;
            function.ptr(context, ptr);

            return get_type(this, function.rtype);
        }

        template <class Name, class Params>
        constexpr TypeInfo operator()(const ast::FunctionCall<Name, Params>& fcall, Pointer& sp)
        {
            return call(fcall, sp, std::make_index_sequence<std::tuple_size_v<Params>>{});
        }
    public:
        // statements
        template <class RValue>
        constexpr bool operator()(const ast::Return<RValue>& ret, Pointer& sp)
        {
            auto typeinfo = ret.expression(*this, sp);
            std::cout << "return: " << sp << std::endl;
            const auto name = typeinfo.name + lib::string("::constructor");
            fn(name)(literal((*this)["return::value"]), literal(sp))(*this, sp);
            return true;
        }

        template <class ...Statements, std::size_t ...I>
        constexpr bool run(const ast::Scope<Statements...>& scope, Pointer& sp, std::index_sequence<I...>)
        {
            using Variables = std::array<VariableInfo, ast::GetVariables<Statements...>::size>;
            Variables variables;

            Context context(this, memory, ScopeInfo{{}, variables, {}});

            bool returned = false;
            (
                (returned || (returned = std::get<I>(scope.statemets)(context, sp))),
                ...
            );
            return returned;
        }

        template <class ...Statements>
        constexpr bool operator()(const ast::Scope<Statements...>& scope, Pointer sp)
        {
            return run(scope, sp, std::make_index_sequence<sizeof...(Statements)>{});
        }

        template <class InitExpression>
        constexpr bool operator()(const ast::VariableDeclaration<InitExpression>& vardecl, Pointer& sp)
        {
            return false;
        }

        template <class Fn>
        constexpr auto operator()(const ast::FunctorBody<Fn>& body, Pointer sp)
        {
            body.functor(*this, sp);
        }
    };

    template <class Name, class RType, class Parameters, class Body>
    constexpr auto import_function(const ast::Function<Name, RType, Parameters, Body>& function)
    {
        return FunctionInfo {
            function.name,
            function.rtype,
            function.params,
            function.body
        };
    }

    template <class Module, std::size_t ...FI>
    constexpr auto import_functions(const Module& module, std::index_sequence<FI...>)
    {
        return std::array<FunctionInfo, sizeof...(FI)> {
            import_function(std::get<FI>(module.functions)) ...
        };
    }
    template <class Module>
    constexpr auto import_functions(const Module& module)
    {
        return import_functions(module, std::make_index_sequence<Module::fsize>{});
    }

    template <class Module, std::size_t ...FI>
    constexpr auto import_types(const Module& module, std::index_sequence<FI...>)
    {
        return std::array<FunctionInfo, sizeof...(FI)> {
            import_type(std::get<FI>(module.functions)) ...
        };
    }
    template <class Module>
    constexpr auto import_types(const Module& module)
    {
        return import_types(module, std::make_index_sequence<Module::tsize>{});
    }

    template <class Module, class ...TArgs>
    constexpr auto call(const Module& module, std::string_view function, const TArgs& ...args)
    {
        Pointer sp = 64;
        std::array<std::uint8_t, 256> storage = {0};
        Memory memory(storage);

        auto functions = import_functions(module);
        std::array types {
            TypeInfo {"void", 0}, TypeInfo {"u32", 4}
        };
        Context context(nullptr, memory, ScopeInfo{functions, {}, types});

        const auto expression = fn(function)(literal(args) ...);
        expression(context, sp);

        return storage;
    }
}
