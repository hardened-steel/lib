#pragma once
#include <tuple>
#include <lib/interpreter/ast/statement.hpp>
#include <lib/interpreter/ast/var.hpp>
#include <lib/typetraits/set.hpp>

namespace lib::interpreter {
    namespace ast {
        template<class Name, class Fields>
        struct Type
        {
            Name name;
            Fields fields;

            constexpr Type(const Name& name, const Fields& fields) noexcept
            : name(name), fields(fields)
            {}
            constexpr Type(const Type& other) = default;
            constexpr Type& operator=(const Type& other) = default;
        };

        template<class Name>
        struct TypeName
        {
            Name name;

            template<class ...Fields>
            constexpr auto operator()(const Fields& ...fields) noexcept
            {
                return Type<Name, std::tuple<Fields...>>(name, std::make_tuple(fields...));
            }
        };

        template<class Type, class Name>
        struct Field
        {
            Type type;
            Name name;
        };

        struct FieldCreator
        {
            template<class Type, class Name>
            constexpr auto operator()(const Type& type, const Name& name)
            {
                return Field<Type, Name>(type, name);
            }
        };

        struct TypeCreator
        {
            template<class Name>
            constexpr auto operator()(const Name& name)
            {
                return Type<Name>(name);
            }
        };
    }

    constexpr inline ast::TypeCreator type;
    constexpr inline ast::FieldCreator field;
}
