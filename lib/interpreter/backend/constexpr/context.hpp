#pragma once
#include <string_view>
#include <lib/interpreter/backend/constexpr/root.hpp>
#include <lib/interpreter/backend/constexpr/typeinfo.hpp>
#include <lib/interpreter/backend/constexpr/memory.hpp>

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
        template<class FunctionBody>
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
        template<class Context>
        constexpr static auto& get_variable(Context* ctx, std::string_view name)
        {
            if (ctx == nullptr) {
                std::cout << "indentifier not found: " << name << std::endl;
                throw std::out_of_range("indentifier not found");
            }
            for (auto& var: ctx->scope.variables) {
                if (var.name == name) {
                    return var;
                }
            }
            return get_variable(ctx->get_parent(), name);
        }
        template<class Context>
        constexpr static auto& get_type(Context* ctx, std::string_view name)
        {
            if (ctx == nullptr) {
                std::cout << "type not found: " << name << std::endl;
                throw std::out_of_range("type not found");
            }
            for (auto& type: ctx->scope.types) {
                if (type.name == name) {
                    return type;
                }
            }
            return get_type(ctx->get_parent(), name);
        }
        template<class Context>
        constexpr static auto& get_function(Context* ctx, std::string_view name, lib::span<const std::string_view> ptypes)
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
        template<class Lhs, class Rhs>
        constexpr TypeInfo operator()(const ast::Sum<Lhs, Rhs>& sum, Pointer& sp)
        {
            const TypeInfo ltype = sum.lhs(*this, sp);
            const TypeInfo rtype = sum.rhs(*this, sp);
            return ltype;
        }

        constexpr TypeInfo operator()(const ast::Variable& var, Pointer& sp)
        {
            auto& var_info = get_variable(this, var.name);
            auto& typeinfo = get_type(this, var_info.type);
            sp -= typeinfo.size;
            fn(typeinfo.name + lib::string("::constructor"))(literal(var_info.ptr), literal(sp))(*this, sp);
            return typeinfo;
        }

        template<class T>
        constexpr TypeInfo operator()(const ast::Literal<T>& literal, Pointer& sp)
        {
            using Type = TypeTrait<T>;
            Type::save(memory, sp -= Type::size, literal.value);
            return get_type(this, Type::name);
        }

        template<class Name, class Params, std::size_t ...I>
        constexpr TypeInfo call(const ast::FunctionCall<Name, Params>& fcall, Pointer& sp, std::index_sequence<I...>)
        {
            auto ptr = sp;
            struct Parameter
            {
                std::string_view type;
                Pointer ptr;
            };
            const std::array params = {
                [&] {
                    auto type = std::get<I>(fcall.params)(*this, ptr);
                    return Parameter {type.name, ptr};
                }()
                ...
            };
            const std::array ptypes = {
                params[I].type ...
            };
            const FunctionInfo& function = get_function(this, fcall.name, ptypes);
            std::array args = {
                VariableInfo {
                    "__function_result__",
                    function.rtype,
                    sp
                },
                VariableInfo {
                    function.params[I].name,
                    params[I].type,
                    params[I].ptr
                } ...
            };

            Context context(this, memory, ScopeInfo{{}, args, {}});
            function.ptr(context, sp);
            return get_type(this, function.rtype);
        }

        template<class Name, class Params>
        constexpr TypeInfo operator()(const ast::FunctionCall<Name, Params>& fcall, Pointer& sp)
        {
            return call(fcall, sp, std::make_index_sequence<std::tuple_size_v<Params>>{});
        }
    public:
        // statements
        template<class RValue>
        constexpr bool operator()(const ast::Return<RValue>& ret, Pointer& sp)
        {
            ret.expression(*this, sp);
            return true;
        }

        template<class ...Statements, std::size_t ...I>
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

        template<class ...Statements>
        constexpr bool operator()(const ast::Scope<Statements...>& scope, Pointer sp)
        {
            return run(scope, sp, std::make_index_sequence<sizeof...(Statements)>{});
        }

        template<class InitExpression>
        constexpr bool operator()(const ast::VariableDeclaration<InitExpression>& vardecl, Pointer& sp)
        {
            return false;
        }

        template<class Fn>
        constexpr auto operator()(const ast::FunctorBody<Fn>& body, Pointer sp)
        {
            body.functor(*this, sp);
        }
    };
    
    template<class Name, class RType, class Parameters, class Body>
    constexpr auto import_function(const ast::FunctionDefinition<Name, RType, Parameters, Body>& function)
    {
        return FunctionInfo {
            function.name,
            function.rtype,
            function.params,
            function.body
        };
    }

    template<class Module, std::size_t ...FI>
    constexpr auto import_functions(const Module& module, std::index_sequence<FI...>)
    {
        return std::array<FunctionInfo, sizeof...(FI)> {
            import_function(std::get<FI>(module.functions)) ...
        };
    }
    template<class Module>
    constexpr auto import_functions(const Module& module)
    {
        return import_functions(module, std::make_index_sequence<Module::fsize>{});
    }

    template<class Module, class ...TArgs>
    constexpr auto call(const Module& module, std::string_view function, const TArgs& ...args)
    {
        Pointer sp = 64;
        std::array<std::uint8_t, 256> storage = {0};
        Memory memory = storage;

        auto functions = import_functions(module);

        Context context(nullptr, memory, ScopeInfo{functions, {}, {}});

        const auto expression = fn(function)(literal(args) ...);
        expression(context, sp);

        return storage;
    }
}
