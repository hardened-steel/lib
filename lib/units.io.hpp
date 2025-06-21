#pragma once
#include "lib/units/count.hpp"
#include <lib/units.hpp>
#include <ratio>
#include <iostream>
#include <type_traits>

namespace lib {

    template <class Unit, class T, class Ratio>
    std::ostream& operator<<(std::ostream& stream, const Quantity<Unit, T, Ratio>& quantity)
    {
        constexpr auto symbol = Unit::symbol();
        if constexpr(sizeof(T) == 1) {
            return stream << (static_cast<int>(quantity.count()) * Ratio::num) / Ratio::den << " " << std::string_view(symbol.data(), symbol.size());
        } else {
            return stream << (quantity.count() * Ratio::num) / Ratio::den << " " << std::string_view(symbol.data(), symbol.size());
        }
    }

    namespace details {
        template <class Unit, class T, class Ratio>
        std::ostream& print(std::ostream& stream, lib::Quantity<Unit, T, Ratio> quantity, const char* prefix)
        {
            constexpr auto symbol = Unit::symbol();
            if constexpr(std::is_same_v<Unit, lib::units::Count>) {
                return stream << (static_cast<unsigned>(quantity.count()) * Ratio::num) / Ratio::den;
            }
            if constexpr(sizeof(T) == 1) {
                return stream << static_cast<int>(quantity.count()) << " " << prefix << std::string_view(symbol.data(), symbol.size());
            } else {
                return stream << quantity.count() << " " << prefix << std::string_view(symbol.data(), symbol.size());
            }
        }
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::atto> quantity)
    {
        return lib::details::print(stream, quantity, "a");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::femto> quantity)
    {
        return lib::details::print(stream, quantity, "f");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::pico> quantity)
    {
        return lib::details::print(stream, quantity, "p");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::nano> quantity)
    {
        return lib::details::print(stream, quantity, "n");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::micro> quantity)
    {
        return lib::details::print(stream, quantity, "μ");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::milli> quantity)
    {
        return lib::details::print(stream, quantity, "m");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::centi> quantity)
    {
        return lib::details::print(stream, quantity, "c");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::deci> quantity)
    {
        return lib::details::print(stream, quantity, "d");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::deca> quantity)
    {
        return lib::details::print(stream, quantity, "da");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::hecto> quantity)
    {
        return lib::details::print(stream, quantity, "h");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::kilo> quantity)
    {
        return lib::details::print(stream, quantity, "k");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::mega> quantity)
    {
        return lib::details::print(stream, quantity, "M");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::giga> quantity)
    {
        return lib::details::print(stream, quantity, "G");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::tera> quantity)
    {
        return lib::details::print(stream, quantity, "T");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::peta> quantity)
    {
        return lib::details::print(stream, quantity, "P");
    }

    template <class T, class Unit>
    std::ostream& operator<<(std::ostream& stream, Quantity<Unit, T, std::exa> quantity)
    {
        return lib::details::print(stream, quantity, "E");
    }

}
