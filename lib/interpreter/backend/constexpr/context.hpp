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

    using FunctionPtr = void (*)();

    struct FunctionInfo
    {
        std::string_view name;
        std::string_view type;
        
    };

    struct RootContext
    {};

    template<class Context>
    constexpr auto& get_variable(Context* ctx, std::string_view name)
    {
        if (ctx == nullptr) {
            std::cout << "indentifier not found: " << name << std::endl;
            throw std::out_of_range("indentifier not found");
        }
        for (auto& var: ctx->variables) {
            if (var.name == name) {
                return var;
            }
        }
        return get_variable(ctx->get_parent(), name);
    }

    template<class Context, std::size_t ...I>
    constexpr auto& get_function(Context* ctx, std::string_view name, std::index_sequence<I...>)
    {
        if (ctx == nullptr) {
            std::cout << "function not found: " << name << std::endl;
            throw std::out_of_range("function not found");
        }
        (
            std::get<I>(ctx->functions).name == name ? std::get<I>(ctx->functions)
        )
        return get_function(ctx->get_parent(), name);
    }

    template<class Parent, class Memory, class Variables, class Functions = std::tuple<>>
    struct Context
    {
        using ParentContext = std::conditional_t<std::is_same_v<Parent, RootContext>, Context, Parent>;
        ParentContext* parent;
        Memory&        memory;
        
        Functions functions;
        Variables variables;

        constexpr Context(Memory& memory, ParentContext* parent, const Functions& functions = Functions{}) noexcept
        : parent(parent), memory(memory), functions(functions)
        {}

        constexpr const ParentContext* get_parent() const noexcept
        {
            return parent;
        }

        constexpr ParentContext* get_parent() noexcept
        {
            return parent;
        }

        constexpr Pointer operator[](std::string_view name) const
        {
            return get_variable(this, name).ptr;
        }

        template<std::size_t N>
        constexpr auto type(const ast::Variable<N>& var) const
        {
            const auto& info = get_variable(this, var.name);
            return info.type;
        }

        template<class Lhs, class Rhs>
        constexpr auto type(const ast::Sum<Lhs, Rhs>& sum) const
        {
            const auto ltype = type(sum.lhs);
            const auto rtype = type(sum.rhs);

            return 0;
        }

        template<class RValue>
        constexpr bool operator()(const ast::Return<RValue>& ret, Pointer sp)
        {
            std::cout << "return: " << type(ret.expression) << std::endl;
            return true;
        }

        template<class ...Statements, std::size_t ...I>
        constexpr bool run(const ast::Scope<Statements...>& scope, Pointer sp, std::index_sequence<I...>)
        {
            using Variables = std::array<VariableInfo, ast::GetVariables<Statements...>::size>;
            Context<Context, Memory, Variables> vars(memory, this);
            bool returned = false;
            (
                (returned || (returned = std::get<I>(scope.statemets)(vars, sp))),
                ...
            );
            return returned;
        }

        template<class ...Statements>
        constexpr bool operator()(const ast::Scope<Statements...>& scope, Pointer sp)
        {
            return run(scope, sp, std::make_index_sequence<sizeof...(Statements)>{});
        }

        template<std::size_t N, class InitExpression>
        constexpr bool operator()(const ast::VariableDeclaration<N, InitExpression>& vardecl, Pointer sp)
        {
            /*const auto [type, ptr] = init(ctx, memory, sp);
            auto& var = ctx.get("");
            var.name = name;
            var.type = "";
            var.ptr  = sp;*/
            return false;
        }

        template<std::size_t N>
        constexpr auto operator()(const ast::Variable<N>& var, Pointer sp)
        {
            //ctx[]
            return std::array{1, 1};
        }
        template<std::size_t N>
        constexpr auto type(const ast::Variable<N>& var)
        {
            const auto& info = ctx.get(name);
        }
    };

    template<std::size_t FI, class Memory, class Module, class Function, class TArgs, std::size_t ...PI>
    constexpr void call
    (
        Memory& memory, Pointer sp,
        const Module& module, const Function& function,
        const TArgs& args, std::index_sequence<PI...>
    )
    {
        using ModuleFunctions = decltype(Module::functions);
        const std::array<FunctionInfo, std::tuple_size_v<Module::functions>> module_functions {

        };
        using ModuleFunction = std::tuple_element_t<FI, ModuleFunctions>;
        if constexpr (std::tuple_size_v<decltype(ModuleFunction::params)> == std::tuple_size_v<TArgs>) {
            const auto& module_function = std::get<FI>(module.functions);
            if (((TypeTrait<std::tuple_element_t<PI, TArgs>>::name == std::get<PI>(module_function.params).type) && ...)) {
                if (module_function.name == function) {
                    using Variables = std::array<VariableInfo, std::tuple_size_v<TArgs> + 1>;
                    Context<RootContext, Memory, Variables, ModuleFunctions> ctx(memory, nullptr, module.functions);
                    auto offset = sp;

                    const std::array<Pointer, std::tuple_size_v<TArgs>> ptrs {
                        (offset = offset - TypeTrait<std::tuple_element_t<PI, TArgs>>::size) ...
                    };
                    
                    (
                        TypeTrait<std::tuple_element_t<PI, TArgs>>::save(
                            memory, ptrs[PI], std::get<PI>(args)
                        ),
                        ...
                    );
                    (
                        (
                            ctx.variables[PI].name = std::get<PI>(module_function.params).name,
                            ctx.variables[PI].type = std::get<PI>(module_function.params).type,
                            ctx.variables[PI].ptr  = ptrs[PI]
                        ),
                        ...
                    );
                    ctx.variables.back().name = "return";
                    ctx.variables.back().type = module_function.rtype;
                    ctx.variables.back().ptr  = sp;
                    module_function.body(ctx, offset);
                }
            }
        }
    }

    template<class Module, class Function, class TArgs, std::size_t ...FI>
    constexpr auto call(const Module& module, const Function& function, const TArgs& args, std::index_sequence<FI...>)
    {
        Memory<256> memory;
        Pointer sp = 64;

        (call<FI>(memory, sp, module, function, args, std::make_index_sequence<std::tuple_size_v<TArgs>>{}), ...);
        return memory;
    }

    template<class Module, std::size_t N, class ...TArgs>
    constexpr auto call(const Module& module, const char (&function)[N], const TArgs& ...args)
    {
        return call (
            module,
            lib::StaticString<N - 1>(function),
            std::make_tuple(args...),
            std::make_index_sequence<Module::fsize>{}
        );
    }
}
