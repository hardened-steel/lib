#pragma once
#include <limits>
#include <numeric>
#include <ratio>
#include <lib/concept.hpp>
#include <lib/typename.hpp>
#include <lib/static.string.hpp>

namespace lib {

    namespace units {
        template<class ...Units>
        struct Multiplying;

        template<class Unit, class ...Units>
        struct Multiplying<Unit, Units...>
        {
            using Dimension = Multiplying;
            constexpr static auto name() noexcept
            {
                return StaticString(type_name_array<Multiplying>);
            }
            constexpr static auto symbol() noexcept
            {
                constexpr StaticString dot(".");
                return Unit::symbol() + ((dot + Units::symbol()) + ...);
            }
        };

        template<>
        struct Multiplying<>
        {
            using Dimension = Multiplying;
        };

        template<class T, int P>
        struct TDegree
        {
            using Type = TDegree;
            using Dimension = Type;

            constexpr static auto symbol() noexcept
            {
                return (T::symbol() + StaticString("^") + toString<P>());
            }
        };

        template<class T, int P>
        using Degree = typename TDegree<T, P>::Type;

        template<class T>
        struct TDegree<T, 0>
        {
            using Type = Multiplying<>;
        };

        template<class ...Units, int P>
        struct TDegree<Multiplying<Units...>, P>
        {
            using Type = Multiplying<Degree<Units, P>...>;
        };

        template<class T, int P1, int P2>
        struct TDegree<TDegree<T, P1>, P2>
        {
            using Type = Degree<T, P1 * P2>;
        };

        template<class Unit>
        struct Multiplying<Unit>
        {
            using Dimension = Unit;
        };

        template<class T>
        struct Dimension
        {
            using Type = T;
        };

        template <typename T>
        using HasCoefficientT = typename T::Coefficient;

        template<class T>
        constexpr inline bool HasCoefficient = lib::detect<T, HasCoefficientT>;

        template<class T, bool = HasCoefficient<T>>
        struct GetCoefficientT
        {
            using Result = std::ratio<1>;
        };

        template<class T>
        struct GetCoefficientT<T, true>
        {
            using Result = typename T::Coefficient;
        };

        template<class T>
        using GetCoefficient = typename GetCoefficientT<T>::Result;

        template<class Unit>
        struct TCanonical
        {
            using Type = Multiplying<Degree<Unit, 1>>;
        };

        template<class Unit>
        using Canonical = typename TCanonical<Unit>::Type;

        template<class ...Units>
        struct TCanonical<Multiplying<Units...>>
        {
            using Type = Multiplying<Degree<Units, 1>...>;
        };

        template<class Unit, int P>
        struct TCanonical<TDegree<Unit, P>>
        {
            using Type = Multiplying<Degree<Unit, P>>;
        };

        template<class Unit>
        struct TSimplefy
        {
            using Type = Unit;
        };

        template<class Unit>
        using Simplefy = typename TSimplefy<Unit>::Type;

        template<class ...Units>
        struct TSimplefy<Multiplying<Units...>>
        {
            using Type = Multiplying<Simplefy<Units>...>;
        };

        template<class Unit, int P>
        struct TSimplefy<TDegree<Unit, P>>
        {
            using Type = Degree<Simplefy<Unit>, P>;
        };

        template<class Unit>
        struct TSimplefy<TDegree<Unit, 1>>
        {
            using Type = Simplefy<Unit>;
        };

        template<class Unit>
        struct TSimplefy<Multiplying<Unit>>
        {
            using Type = Simplefy<Unit>;
        };
    }

    namespace details::units {

        template<class Unit>
        using Canonical = lib::units::Canonical<Unit>;

        template<class ...Units>
        using Multiplying = lib::units::Multiplying<Units...>;

        template<class T, int P>
        using Degree = lib::units::Degree<T, P>;

        template<class T, int P>
        using TDegree = lib::units::TDegree<T, P>;

        template<class UnitsA, class UnitsB>
        struct TConcat;

        template<class UnitsA, class UnitsB>
        using Concat = typename TConcat<UnitsA, UnitsB>::Result;

        template<class ...UnitsA, class ...UnitsB>
        struct TConcat<Multiplying<UnitsA...>, Multiplying<UnitsB...>>
        {
            using Result = Multiplying<UnitsA..., UnitsB...>;
        };

        template<class UnitA, class UnitB>
        struct TCompare;

        template<class UnitA, class UnitB>
        constexpr static inline bool Compare = TCompare<UnitA, UnitB>::Result;

        template<class UnitA, int Pa, class UnitB, int Pb>
        struct TCompare<TDegree<UnitA, Pa>, TDegree<UnitB, Pb>>
        {
            constexpr static inline bool Result = lib::type_name<UnitA> < lib::type_name<UnitB>;
        };

        template<class UnitA, int Pa>
        struct TCompare<TDegree<UnitA, Pa>, Multiplying<>>
        {
            constexpr static inline bool Result = false;
        };

        template<class UnitB, int Pb>
        struct TCompare<Multiplying<>, TDegree<UnitB, Pb>>
        {
            constexpr static inline bool Result = true;
        };

        template<class UnitA, class UnitB>
        struct TMultiplyBinary;

        template<class UnitA, class UnitB>
        using MultiplyBinary = typename TMultiplyBinary<UnitA, UnitB>::Result;

        template<class UnitA, int Pa, class UnitB, int Pb>
        struct TMultiplyBinary<TDegree<UnitA, Pa>, TDegree<UnitB, Pb>>
        {
            using Result = std::conditional_t<
                Compare<TDegree<UnitA, Pa>, TDegree<UnitB, Pb>>,
                Multiplying<Degree<UnitA, Pa>, Degree<UnitB, Pb>>,
                Multiplying<Degree<UnitB, Pb>, Degree<UnitA, Pa>>
            >;
        };

        template<class Unit, int Pa, int Pb>
        struct TMultiplyBinary<TDegree<Unit, Pa>, TDegree<Unit, Pb>>
        {
            using Result = Multiplying<Degree<Unit, Pa + Pb>>;
        };

        template<class UnitA, int Pa>
        struct TMultiplyBinary<TDegree<UnitA, Pa>, Multiplying<>>
        {
            using Result = Multiplying<Degree<UnitA, Pa>>;
        };

        template<class UnitB, int Pb>
        struct TMultiplyBinary<Multiplying<>, TDegree<UnitB, Pb>>
        {
            using Result = Multiplying<Degree<UnitB, Pb>>;
        };

        template<class UnitA, class UnitB>
        struct TMultiply;

        template<class UnitA, class UnitB>
        using Multiply = typename TMultiply<UnitA, UnitB>::Result;

        template<class UnitA, int Pa, class UnitB, int Pb>
        struct TMultiply<Multiplying<TDegree<UnitA, Pa>>, Multiplying<TDegree<UnitB, Pb>>>
        {
            using Result = MultiplyBinary<TDegree<UnitA, Pa>, TDegree<UnitB, Pb>>;
        };

        template<class UnitsA, class UnitsB>
        struct TMultiplyInserts;

        template<class UnitsA, class UnitsB>
        using MultiplyInserts = typename TMultiplyInserts<UnitsA, UnitsB>::Result;

        template<class Units, class Unit>
        struct TMultiplyInsert;

        template<class Units, class Unit>
        using MultiplyInsert = typename TMultiplyInsert<Units, Unit>::Result;

        template<class Head, class ...Tail, class Unit>
        struct TMultiplyInsert<Multiplying<Head, Tail...>, Multiplying<Unit>>
        {
            constexpr static inline bool less = Compare<Unit, Head>;
            using Result = std::conditional_t<
                less,
                Multiplying<Unit, Head, Tail...>,
                Concat<
                    Canonical<Head>,
                    MultiplyInsert<Multiplying<Tail...>, Multiplying<Unit>>
                >
            >;
        };

        template<int Ph, class ...Tail, class Unit, int Pu>
        struct TMultiplyInsert<Multiplying<TDegree<Unit, Ph>, Tail...>, Multiplying<TDegree<Unit, Pu>>>
        {
            using Result = std::conditional_t<
                Pu + Ph == 0,
                Multiplying<Tail...>,
                Concat<
                    MultiplyBinary<TDegree<Unit, Ph>, TDegree<Unit, Pu>>,
                    Multiplying<Tail...>
                >
            >;
        };

        template<class ...Units>
        struct TMultiplyInsert<Multiplying<Units...>, Multiplying<>>
        {
            using Result = Multiplying<Units...>;
        };

        template<class Unit>
        struct TMultiplyInsert<Multiplying<>, Multiplying<Unit>>
        {
            using Result = Multiplying<Unit>;
        };

        template<class Unit>
        struct TMultiplyInsert<Multiplying<Unit>, Multiplying<>>
        {
            using Result = Multiplying<Unit>;
        };

        template<>
        struct TMultiplyInsert<Multiplying<>, Multiplying<>>
        {
            using Result = Multiplying<>;
        };

        template<class ...Units, class Unit>
        struct TMultiply<Multiplying<Unit>, Multiplying<Units...>>
        {
            using Result = MultiplyInsert<Multiplying<Units...>, Multiplying<Unit>>;
        };

        template<class ...UnitsA, class Unit, class ...UnitsB>
        struct TMultiplyInserts<Multiplying<UnitsA...>, Multiplying<Unit, UnitsB...>>
        {
            using Result = MultiplyInserts<MultiplyInsert<Multiplying<UnitsA...>, Multiplying<Unit>>, Multiplying<UnitsB...>>;
        };

        template<class ...UnitsA>
        struct TMultiplyInserts<Multiplying<UnitsA...>, Multiplying<>>
        {
            using Result = Multiplying<UnitsA...>;
        };

        template<class ...UnitsA, class ...UnitsB>
        struct TMultiply<Multiplying<UnitsA...>, Multiplying<UnitsB...>>
        {
            using Result = MultiplyInserts<Multiplying<UnitsA...>, Multiplying<UnitsB...>>;
        };
    }

    namespace units {
        template<class ...Units>
        struct TMultiply;

        template<class ...Units>
        using Multiply = typename TMultiply<Units...>::Type;

        template<class Unit, class ...Units>
        struct TMultiply<Unit, Units...>
        {
            using Result = details::units::Multiply<
                Canonical<typename Unit::Dimension>,
                typename TMultiply<Canonical<typename Units::Dimension>...>::Result
            >;
            using Type = Simplefy<Result>;
        };

        template<class UnitA, class UnitB>
        struct TMultiply<UnitA, UnitB>
        {
            using Result = details::units::Multiply<Canonical<typename UnitA::Dimension>, Canonical<typename UnitB::Dimension>>;
            using Type = Simplefy<Result>;
        };

        template<>
        struct TMultiply<>
        {
            using Result = Multiplying<>;
            using Type = Result;
        };

        template<class UnitA, class UnitB>
        using Devide = Multiply<UnitA, Simplefy<Degree<typename UnitB::Dimension, -1>>>;
    }

    template<class Unit, class T, class Ratio = lib::units::GetCoefficient<Unit>>
    class Quantity;

    template<class U, class T, class R>
    class QuantityBase;

    template<class ToQuantity, class Unit, class IType, class IRatio>
    constexpr ToQuantity quantity_cast(const QuantityBase<Unit, IType, IRatio>& quantity) noexcept;

    template<class QA, class QB>
    struct CommonQuantityT;

    template<class QA, class QB>
    using CommonQuantity = typename CommonQuantityT<QA, QB>::Result;

    template<class Unit, class T1, class RatioA, class T2, class RatioB>
    struct CommonQuantityT<Quantity<Unit, T1, RatioA>, Quantity<Unit, T2, RatioB>>
    {
        using Result = Quantity<
            Unit,
            std::common_type_t<T1, T2>,
            std::ratio<std::gcd(RatioA::num, RatioB::num), std::lcm(RatioA::den, RatioB::den)>
        >;
    };

    template<class Tag>
    struct Unit
    {
        template<class T>
        friend auto operator * (T quantity, Unit) noexcept
        {
            return Quantity<Tag, T, typename Tag::Ratio>(quantity);
        }
        template<std::intmax_t Num, std::intmax_t Den>
        friend auto operator * (std::ratio<Num, Den>, Unit) noexcept
        {
            using Ratio = std::ratio_multiply<std::ratio<Num, Den>, typename Tag::Ratio>;
            return Quantity<Tag, unsigned, Ratio>(1);
        }
        template<class T, class Ratio>
        auto operator()(const Quantity<Tag, T, Ratio>& quantity) const noexcept
        {
            return quantity_cast<Quantity<Tag, T>>(quantity);
        }
        template<class T>
        auto operator()(T quantity) const noexcept
        {
            return Quantity<Tag, T>(quantity);
        }
    };

    template<class ORatio, class OType, class IRatio, class IType>
    constexpr auto cast(const IType& quantity) noexcept
    {
        using Ratio = std::ratio_divide<IRatio, ORatio>;
        using CType = std::common_type_t<OType, IType, std::intmax_t>;

        constexpr auto den = static_cast<CType>(Ratio::den);
        constexpr auto num = static_cast<CType>(Ratio::num);

        if constexpr(num == 1) {
            if constexpr(den == 1) {
                return static_cast<OType>(quantity);
            } else {
                return static_cast<OType>(static_cast<CType>(quantity) / den);
            }
        } else {
            if constexpr(den == 1) {
                return static_cast<OType>(static_cast<CType>(quantity) * num);
            } else {
                return static_cast<OType>(static_cast<CType>(quantity) * num / den);
            }
        }
    }

    template<class ToQuantity, class Unit, class IType, class IRatio>
    constexpr ToQuantity quantity_cast(const QuantityBase<Unit, IType, IRatio>& quantity) noexcept
    {
        static_assert(std::is_same_v<Unit, typename ToQuantity::Unit>);
        using OType = typename ToQuantity::Type;
        using ORatio = typename ToQuantity::Ratio;
        
        return ToQuantity(cast<ORatio, OType, IRatio, IType>(quantity.count()));
    }

    template<class U, class T, class R>
    class QuantityBase
    {
    protected:
        T quantity = 0;
    public:
        using Ratio = R;
        using Type  = T;
        using Unit  = U;
    public:
        constexpr QuantityBase() noexcept = default;
        template<
            class Other,
            typename = lib::Require<
                std::is_convertible_v<const Other&, Type>,
                std::is_floating_point_v<Type> || !std::is_floating_point_v<Other>
            >
        >
        constexpr explicit QuantityBase(Other quantity) noexcept
        : quantity(static_cast<Type>(quantity))
        {}
        template<
            class Other, class ORatio,
            typename = lib::Require<
                std::is_convertible_v<const Other&, Type>,
                std::is_floating_point_v<Type> || ((std::ratio_divide<ORatio, Ratio>::den == 1) && !std::is_floating_point_v<Other>)
            >
        >
        constexpr QuantityBase(const QuantityBase<Unit, Other, ORatio>& other) noexcept
        : quantity(quantity_cast<QuantityBase>(other).count())
        {}
        QuantityBase(const QuantityBase&) = default;
        QuantityBase& operator=(const QuantityBase&) = default;
    public:
        Type count() const noexcept
        {
            return quantity;
        }
        constexpr auto operator-() const noexcept
        {
            return Quantity<U, std::make_signed_t<T>, R>(-quantity);
        }
        constexpr auto operator+() const noexcept
        {
            return Quantity<U, std::make_signed_t<T>, R>(-quantity);
        }
    };

    template<class U, class T, class R>
    class Quantity: public QuantityBase<U, T, R>
    {
        using Base = QuantityBase<U, T, R>;
    public:
        using Base::Base;
    };

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator+ (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        constexpr auto num = std::gcd(ARatio::num, BRatio::num);
        constexpr auto den = std::gcd(ARatio::den, BRatio::den);
        using Ratio = std::ratio<num, (ARatio::den / den) * BRatio::den>;

        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;
        return CommonQuantity(CommonQuantity(a).count() + CommonQuantity(b).count());
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator- (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        constexpr auto num = std::gcd(ARatio::num, BRatio::num);
        constexpr auto den = std::gcd(ARatio::den, BRatio::den);
        using Ratio = std::ratio<num, (ARatio::den / den) * BRatio::den>;

        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;
        return CommonQuantity(CommonQuantity(a).count() - CommonQuantity(b).count());
    }

    template<class AUnit, class A, class ARatio, class BUnit, class B, class BRatio>
    constexpr auto operator* (const Quantity<AUnit, A, ARatio>& a, const Quantity<BUnit, B, BRatio>& b) noexcept
    {
        using Unit  = typename units::Dimension<units::Multiply<AUnit, BUnit>>::Type;
        using Type = std::common_type_t<A, B>;

        using Ratio = std::ratio_multiply<ARatio, BRatio>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;

        return CommonQuantity(static_cast<Type>(a.count()) * static_cast<Type>(b.count()));
    }

    template<class AUnit, class A, class ARatio, class BUnit, class B, class BRatio>
    constexpr auto operator/ (const Quantity<AUnit, A, ARatio>& a, const Quantity<BUnit, B, BRatio>& b) noexcept
    {
        using Unit = typename units::Dimension<units::Devide<AUnit, BUnit>>::Type;
        using Type = std::common_type_t<A, B>;

        constexpr auto num = std::gcd(ARatio::num, BRatio::den);
        constexpr auto den = ARatio::den * BRatio::num;

        using Ratio = std::ratio_divide<std::ratio<num>, std::ratio<den>>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;

        return CommonQuantity(
            (a.count() * std::lcm(ARatio::num, BRatio::den))
            /
            (b.count())
        );
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator== (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        using CQ = CommonQuantity<Quantity<Unit, A, ARatio>, Quantity<Unit, B, BRatio>>;
        return CQ(a).count() == CQ(b).count();
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator!= (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        return !(a == b);
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator< (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        using CQ = CommonQuantity<Quantity<Unit, A, ARatio>, Quantity<Unit, B, BRatio>>;
        return CQ(a).count() < CQ(b).count();
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator<= (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        using CQ = CommonQuantity<Quantity<Unit, A, ARatio>, Quantity<Unit, B, BRatio>>;
        return CQ(a).count() <= CQ(b).count();
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator> (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        using CQ = CommonQuantity<Quantity<Unit, A, ARatio>, Quantity<Unit, B, BRatio>>;
        return CQ(a).count() > CQ(b).count();
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator>= (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        using CQ = CommonQuantity<Quantity<Unit, A, ARatio>, Quantity<Unit, B, BRatio>>;
        return CQ(a).count() >= CQ(b).count();
    }
}