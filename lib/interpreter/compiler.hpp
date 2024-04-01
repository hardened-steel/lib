#pragma once
#include <lib/interpreter/ast.hpp>
#include <lib/array.hpp>
#include <lib/tuple.hpp>

namespace lib::interpreter {
    using Pointer = std::uint32_t;

    enum TypeCode: std::uint32_t
    {
        Void,
        U8, U16, U32, U64,
        Ptr,
        StructBegin, StructEnd
    };

    template<class T>
    struct TypeTrait;

    template<>
    struct TypeTrait<std::uint32_t>
    {
        /*constexpr*/ static inline auto name = lib::StaticString("u32");
        /*constexpr*/ static inline auto size = sizeof(std::uint32_t);
        /*constexpr*/ static inline auto save = [](auto& memory, Pointer sp, std::uint32_t value) {
            memory.save(sp, value);
        };
        /*constexpr*/ static inline auto load = [](auto& memory, Pointer sp) {
            return memory.load(sp);
        };
    };

    template<class ...TArgs>
    /*constexpr*/ auto make_code(TArgs&& ...args) {
        return std::array<std::string, sizeof...(TArgs)>{
            std::forward<TArgs>(args)...
        };
    }

    struct VariableInfo
    {
        std::string_view name;
        std::string_view type;
        Pointer offset;
    };

    struct FunctionInfo
    {
        std::string_view name;
        std::string_view rtype;
        lib::span<const ast::Parameter> params;
    };

    struct FunctionCallSkip
    {
        std::string_view name;
        lib::span<const ast::Parameter> params;
        Pointer offset;

        /*constexpr*/ FunctionCallSkip(const FunctionInfo& call, Pointer offset) noexcept
        : name(call.name), params(call.params)
        , offset(offset)
        {}
        /*constexpr*/ FunctionCallSkip(const FunctionCallSkip&) = default;
        /*constexpr*/ FunctionCallSkip& operator=(const FunctionCallSkip&) = default;
    };

    template<class ...TArgs>
    /*constexpr*/ auto make_skips(TArgs&& ...args) {
        return std::array<FunctionCallSkip, sizeof...(TArgs)>{
            std::forward<TArgs>(args)...
        };
    }

    struct TypeInfo
    {
        std::string_view name;
        std::size_t      size;
        std::size_t      code;
    };

    template<std::size_t N>
    using TypeCodes = std::array<std::uint32_t, N>;

    template<std::size_t N>
    using TypeInfos = std::array<TypeInfo, N>;

    template<std::size_t C, std::size_t I>
    struct Types
    {
        TypeCodes<C> codes;
        TypeInfos<I> infos;

        /*constexpr*/ Types(const TypeCodes<C>& codes, const TypeInfos<I>& infos) noexcept
        : codes(codes), infos(infos)
        {}
        template<class TI>
        /*constexpr*/ Types(const std::array<TypeCode, C>&, const std::array<TI, I>&) noexcept
        : codes(codes), infos(infos)
        {}
        /*constexpr*/ Types(const Types&) = default;

        template<class Name>
        /*constexpr*/ auto& find(const Name& name) const
        {
            for(const auto& info: infos) {
                if (info.name == name) {
                    return info;
                }
            }
            const auto message = lib::string("type not found: ") + name;
            throw std::out_of_range(std::string(message.begin(), message.end()));
        }
    };

    template<std::size_t C, std::size_t I>
    Types(const TypeCodes<C>&, const TypeInfos<I>&) -> Types<C, I>;

    template<std::size_t C, class TI, std::size_t I>
    Types(const std::array<TypeCode, C>&, const std::array<TI, I>&) -> Types<C, I>;

    /*constexpr*/ inline Types<4, 4> init_types (
        {Void, U8, U16, U32},
        {
            TypeInfo{"void", 0, 0}, TypeInfo{"u8", 1, 1}, TypeInfo{"u16", 2, 2}, TypeInfo{"u32", 4, 3}
        }
    );

    /*constexpr*/ inline Types<0, 0> null_types ({}, {});
    /*constexpr*/ inline std::array<VariableInfo, 0> null_variables {};
    /*constexpr*/ inline std::array<FunctionInfo, 0> null_functions {};
    inline const auto null_text = std::make_tuple(make_code(), make_skips());

    template<class Parent, class Types, class Variables, class Functions, class Text>
    struct CompilerContext
    {
        Parent parent;

        Types types;
        Variables variables;
        Functions functions;

        Text text;

        /*constexpr*/ CompilerContext
        (
            const Parent& parent,
            const Types& types,
            const Variables& variables,
            const Functions& functions,
            const Text& text
        )
        noexcept
        : parent(parent), types(types), variables(variables), functions(functions)
        , text(text)
        {}

        template<class Name>
        /*constexpr*/ auto& find_variable(const Name& name) const
        {
            for(const auto& var: variables) {
                if(var.name == name) {
                    return var;
                }
            }
            return parent.find_variable(name);
        }

        template<class Name>
        /*constexpr*/ auto& find_type(const Name& name) const
        {
            for(const auto& type: types.infos) {
                if (type.name == name) {
                    return type;
                }
            }
            return parent.find_type(name);
        }

        template<class Name>
        /*constexpr*/ auto& find_function(const Name& name, lib::span<const TypeInfo> ptypes) const
        {
            for(const auto& fn: functions) {
                if(fn.name == name) {
                    if (fn.params.size() == ptypes.size()) {
                        bool found = true;
                        for (std::size_t i = 0; i < fn.params.size(); ++i) {
                            if (fn.params[i].type != ptypes[i].name) {
                                found = false;
                                break;
                            }
                        }
                        if (found) {
                            return fn;
                        }
                    }
                }
            }
            return parent.find_function(name, ptypes);
        }
    };

    template<class Types, class Variables, class Functions, class Text>
    struct CompilerContext<void, Types, Variables, Functions, Text>
    {
        Types types;
        Variables variables;
        Functions functions;

        Text text;

        /*constexpr*/ CompilerContext
        (
            const Types& types,
            const Variables& variables,
            const Functions& functions,
            const Text& text
        )
        noexcept
        : types(types), variables(variables), functions(functions)
        , text(text)
        {}

        template<class Name>
        /*constexpr*/ auto& find_variable(const Name& name) const
        {
            for(const auto& var: variables) {
                if(var.name == name) {
                    return var;
                }
            }
            const auto message = lib::string("variable not found: ") + name;
            throw std::out_of_range(std::string(message.begin(), message.end()));
        }

        template<class Name>
        /*constexpr*/ auto& find_type(const Name& name) const
        {
            for(const auto& type: types.infos) {
                if (type.name == name) {
                    return type;
                }
            }
            const auto message = lib::string("type not found: ") + name;
            throw std::out_of_range(std::string(message.begin(), message.end()));
        }

        template<class Name>
        /*constexpr*/ auto& find_function(const Name& name, lib::span<const TypeInfo> ptypes) const
        {
            for(const auto& fn: functions) {
                if(fn.name == name) {
                    if (fn.params.size() == ptypes.size()) {
                        bool found = true;
                        for (std::size_t i = 0; i < fn.params.size(); ++i) {
                            if (fn.params[i].type != ptypes[i].name) {
                                found = false;
                                break;
                            }
                        }
                        if (found) {
                            return fn;
                        }
                    }
                }
            }
            const auto message = lib::string("function not found: ") + name;
            throw std::out_of_range(std::string(message.begin(), message.end()));
        }
    };

    template<class Parent, class Types, class Variables, class Functions, class Text>
    /*constexpr*/ auto make_context
    (
        const Parent& parent,
        const Types& types,
        const Variables& variables,
        const Functions& functions,
        const Text& text
    )
    {
        return CompilerContext<Parent, Types, Variables, Functions, Text>(parent, types, variables, functions, text);
    }

    template<class Types, class Variables, class Functions, class Text>
    /*constexpr*/ auto make_context
    (
        const Types& types,
        const Variables& variables,
        const Functions& functions,
        const Text& text
    )
    {
        return CompilerContext<void, Types, Variables, Functions, Text>(types, variables, functions, text);
    }

    template<class Context, class Name, class Params, std::size_t ...I>
    /*constexpr*/ auto compile
    (
        const Context& ctx,
        const ast::FunctionCall<Name, Params>& call,
        std::index_sequence<I...>
    )
    {
        const std::tuple args = {
            compile(ctx, std::get<I>(call.params)) ...
        };
        const std::array ptypes = {
            std::get<0>(std::get<I>(args)) ...
        };
        const auto& function = ctx.find_function(call.name, ptypes);
        const auto& typeinfo = ctx.find_type(function.rtype);

        return std::make_tuple(
            typeinfo,
            [=](Pointer sp_offset, Pointer code_offset) {
                //offset += typeinfo.size;
                /*std::cout << "SP - " << typeinfo.size << " # offset = " << offset << ", type = " << typeinfo.name << std::endl;
                (
                    [&] {
                        std::get<1>(std::get<I>(args))(offset);
                        offset += ptypes[I].size;
                    }(),
                    ...
                );
                *///std::cout << "PUSH &" << function.name << std::endl;
                //std::cout << "CALL" << std::endl;

                auto start = make_code("SP - " + std::to_string(typeinfo.size));
                sp_offset += typeinfo.size;
                std::tuple codes = {
                    [&] {
                        auto [code, skips] = std::get<1>(std::get<I>(args))(sp_offset, code_offset + 1);
                        sp_offset += ptypes[I].size;
                        code_offset += code.size();
                        return std::make_tuple(code, skips);
                    }() ...
                };
                auto end = make_code(
                    "PUSH &Function",
                    "CALL"
                );
                return std::make_tuple(
                    concat(start, std::get<0>(std::get<I>(codes))..., end),
                    concat(
                        std::get<1>(std::get<I>(codes))...,
                        make_skips(
                            FunctionCallSkip(function, code_offset)
                        )
                    )
                );
            }
        );
    }

    template<class Context, class Name, class Params>
    /*constexpr*/ auto compile(const Context& ctx, const ast::FunctionCall<Name, Params>& call)
    {
        return compile(ctx, call, std::make_index_sequence<std::tuple_size_v<Params>>{});
    }

    template<class Context>
    /*constexpr*/ auto compile(const Context& ctx, const ast::Variable& var)
    {
        const auto& var_info = ctx.find_variable(var.name);
        const auto& typeinfo = ctx.find_type(var_info.type);
        const auto name = typeinfo.name + lib::string("::constructor");
        return std::make_tuple(
            typeinfo,
            [=](Pointer sp_offset, Pointer code_offset) {
                return std::make_tuple(
                    make_code(
                        "SP - " + std::to_string(typeinfo.size),
                        "SP push",
                        "PUSH SP + " + std::to_string(var_info.offset + sp_offset + typeinfo.size + sizeof(Pointer)),
                        "COPY"
                    ),
                    make_skips()
                );
                /*
                sp_offset += typeinfo.size;
                std::cout << "SP - " << typeinfo.size << " # offset = " << sp_offset << ", type = " << typeinfo.name << std::endl;
                sp_offset += sizeof(Pointer);
                std::cout << "SP push # offset = " << sp_offset << std::endl;
                std::cout << "PUSH SP + " << var_info.offset + sp_offset << " # push input parameter " << var.name << std::endl;
                std::cout << "COPY" << std::endl;
                */
            }
        );
    }

    template<class Context, class Lhs, class Rhs>
    /*constexpr*/ auto compile(const Context& ctx, const ast::Sum<Lhs, Rhs>& sum)
    {
        return compile(ctx, fn("operator: lhs + rhs")(sum.lhs, sum.rhs));
    }

    template<class Context, class LType>
    /*constexpr*/ auto compile(const Context& ctx, const ast::Literal<LType>& literal)
    {
        using Type = TypeTrait<LType>;
        const auto& typeinfo = ctx.find_type(Type::name);
        return std::make_tuple(
            typeinfo,
            [name = typeinfo.name](Pointer offset) {
                std::cout << "SP - " << Type::size << " # offset = " << offset << ", type = " << typeinfo.name << std::endl;
                std::cout << "ASM for: " << name << std::endl;
            }
        );
    }

    template<class Context, class RValue>
    /*constexpr*/ auto compile(const Context& ctx, const ast::Return<RValue>& ret)
    {
        const auto& var_info = ctx.find_variable("return::value");
        const auto& typeinfo = ctx.find_type(var_info.type);
        const auto [rtype, generator] = compile(ctx, ret.expression);

        auto code_offset = std::get<0>(ctx.text).size();
        auto [code, skips] = generator(0, code_offset);
        /*
        std::cout << "PUSH SP + " << var_info.offset + offset + rtype.size << " # push ptr of return::value " << std::endl;
        std::cout << "PUSH SP + " << sizeof(Pointer) << " # push input parameter "<< std::endl;
        std::cout << "COPY" << std::endl;
        std::cout << "SP + " << var_info.offset + offset + rtype.size << std::endl;
        std::cout << "RET" << std::endl;
        */
        return make_context(
            ctx.types, ctx.variables, ctx.functions,
            make_tuple(
                concat(
                    std::get<0>(ctx.text),
                    code,
                    make_code(
                        "PUSH SP + " + std::to_string(var_info.offset + rtype.size),
                        "PUSH SP + " + std::to_string(sizeof(Pointer)),
                        "COPY",
                        "SP + " + std::to_string(var_info.offset),
                        "RET"
                    )
                ),
                concat(std::get<1>(ctx.text), skips)
            )
        );
    }

    template<
        class Context,
        class ...Statements,
        std::size_t ...I
    >
    /*constexpr*/ auto compile_scope
    (
        const Context& ctx,
        const ast::Scope<Statements...>& scope,
        std::index_sequence<I...>
    )
    {
        (compile(ctx, std::get<I>(scope.operators)), ...);
    }

    template<
        class Context,
        class Statement,
        class ...Statements
    >
    /*constexpr*/ auto compile_scope
    (
        const Context& ctx,
        const ast::Scope<Statement, Statements...>& scope
    )
    {
        return compile_scope(
            compile(ctx, head(scope.operators)),
            ast::Scope<Statements...>(tail(scope.operators))
        );
    }

    template<
        class Context,
        class Function,
        std::size_t ...I
    >
    /*constexpr*/ auto compile_function
    (
        const Context& ctx,
        const Function& function,
        std::index_sequence<I...>
    )
    {
        Pointer offset = 4;
        std::array args = {
            VariableInfo {
                std::get<I>(function.params).name,
                std::get<I>(function.params).type,
                [&] {
                    auto ptr = offset;
                    offset += ctx.types.find(std::get<I>(function.params).type).size;
                    return ptr;
                }()
            } ...,
            VariableInfo {
                "return::value",
                function.rtype,
                offset
            }
        };
        return compile_scope(
            make_context(ctx.types, lib::concat(ctx.variables, args), ctx.functions, ctx.text),
            function.body
        );
    }

    template<class Context, class Function>
    /*constexpr*/ auto compile_function
    (
        const Context& ctx,
        const Function& function
    )
    {
        return compile_function(ctx, function, std::make_index_sequence<Function::psize>{});
    }

    // statements
    template<
        class Parent, class Types, class Variables, class Functions, class Text,
        class Name, class RType, class Parameters, class Body
    >
    /*constexpr*/ auto operator+
    (
        const CompilerContext<Parent, Types, Variables, Functions, Text>& ctx,
        const ast::Function<Name, RType, Parameters, Body>& function
    )
    {
        std::array<FunctionInfo, 1> info {
            function.name,
            function.rtype
        };
        compile_function(ctx, function);
        return make_context(ctx.types, ctx.variables, lib::concat(ctx.functions, info), ctx.text);
    }

    // module
    template<class Module, std::size_t ...I>
    /*constexpr*/ auto compile_module(const Module& module, std::index_sequence<I...>)
    {
        auto context = make_context(init_types, null_variables, null_functions, null_text);
        return (context + ... + std::get<I>(module.statements));
    }
    template<class ...Statements>
    /*constexpr*/ auto compile(const ast::Module<Statements...>& module)
    {
        return compile_module(module, std::make_index_sequence<ast::Module<Statements...>::size>{});
    }
}
