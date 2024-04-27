#pragma once
#include <limits>
#include <lib/numeric.hpp>
#include <lib/ratio.hpp>
#include <lib/literal.hpp>
#include <lib/typetraits/set.hpp>

namespace lib {
    namespace details::units {
        namespace impl {
            using namespace lib::typetraits;
        }

        template<class Ratio, class Types>
        struct Units
        {
            using Coefficient = Ratio;
            using Dimension = Types;
        };

        namespace impl {
            template<class Units, class Unit>
            struct InsertF;

            template<class Units, class Unit>
            using Insert = typename InsertF<Units, Unit>::Result;

            template<class Ratio, class Unit>
            struct InsertF<Units<Ratio, typetraits::List<>>, Unit>
            {
                using Result = Units<Ratio, typetraits::List<Unit>>;
            };
        }
        
        template<class Units, class Unit>
        using Insert = impl::Insert<Units, Unit>;

        namespace impl {
            template<class Units, class Unit>
            struct InsertFrontF;

            template<class Units, class Unit>
            using InsertFront = typename InsertFrontF<Units, Unit>::Result;

            template<class Ratio, class ...Types, class Unit>
            struct InsertFrontF<Units<Ratio, List<Types...>>, Unit>
            {
                using Result = Units<Ratio, List<Unit, Types...>>;
            };
        }
        
        template<class Units, class Unit>
        using InsertFront = impl::InsertFront<Units, Unit>;

        namespace impl {
            template<class Lhs, class Rhs>
            struct MergeBinaryF;

            template<class Lhs, class Rhs>
            using MergeBinary = typename MergeBinaryF<Lhs, Rhs>::Result;

            template<class RatioA, class ATypes, class RatioB, class Head, class ...Tail>
            struct MergeBinaryF<Units<RatioA, ATypes>, Units<RatioB, List<Head, Tail...>>>
            {
                using Result = MergeBinary<Insert<Units<RatioA, ATypes>, Head>, Units<RatioB, List<Tail...>>>;
            };

            template<class RatioA, class ATypes, class RatioB>
            struct MergeBinaryF<Units<RatioA, ATypes>, Units<RatioB, List<>>>
            {
                using Result = Units<std::ratio_multiply<RatioA, RatioB>, ATypes>;
            };

            template<class ...Units>
            struct MergeF;

            template<class ...Units>
            using Merge = typename MergeF<Units...>::Result;

            template<class Lhs, class Head, class ...Tail>
            struct MergeF<Lhs, Head, Tail...>
            {
                using Result = typename MergeF<MergeBinary<Lhs, Head>, Tail...>::Result;
            };

            template<class Lhs, class Rhs>
            struct MergeF<Lhs, Rhs>
            {
                using Result = MergeBinary<Lhs, Rhs>;
            };
        }
        
        template<class ...Units>
        using Merge = impl::Merge<Units...>;
    }

    namespace units {
        template<class ...Units>
        struct Multiplying;

        template<class Unit, class ...Units>
        struct Multiplying<Unit, Units...>
        {
            using Dimension = Multiplying;
            constexpr static inline StaticString<1> dot = "*";

            constexpr static auto name() noexcept
            {
                return (type_name<Unit> + ... + (dot + type_name<Units>));
            }
            constexpr static auto symbol() noexcept
            {
                return (Unit::symbol() + ... + (dot + Units::symbol()));
            }
        };

        template<>
        struct Multiplying<>
        {
            using Dimension = Multiplying;
        };

        template<class Unit, int P>
        struct TDegree
        {
            using Dimension = TDegree;

            constexpr static auto name() noexcept
            {
                return type_name<Unit> + "^" + value_string<P>;
            }

            constexpr static auto symbol() noexcept
            {
                return "(" + Unit::symbol() + "^" + value_string<P> + ")";
            }
        };

        template<class T, int P>
        using Degree = typename TDegree<T, P>::Dimension;

        template<class T>
        struct TDegree<T, 0>
        {
            using Dimension = Multiplying<>;
        };

        template<class ...Units, int P>
        struct TDegree<Multiplying<Units...>, P>
        {
            using Dimension = Multiplying<Degree<Units, P>...>;
        };

        template<class T, int P1, int P2>
        struct TDegree<TDegree<T, P1>, P2>
        {
            using Dimension = Degree<T, P1 * P2>;
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
        constexpr inline bool has_coefficient = lib::detect<T, HasCoefficientT>;

        template<class T, bool = has_coefficient<T>>
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
            using Result = details::units::Units<std::ratio<1>, typetraits::List<Degree<Unit, 1>>>;
        };

        template<class Unit>
        using Canonical = typename TCanonical<Unit>::Result;

        template<class ...Units>
        struct TCanonical<Multiplying<Units...>>
        {
            using Result = details::units::Units<std::ratio<1>, typetraits::List<Degree<Units, 1>...>>;
        };

        template<>
        struct TCanonical<Multiplying<>>
        {
            using Result = details::units::Units<std::ratio<1>, typetraits::List<>>;
        };

        template<class Unit>
        struct TSimplify
        {
            using Result = Unit;
        };

        template<class Unit>
        using Simplify = typename TSimplify<Unit>::Result;

        template<std::intmax_t Num, std::intmax_t Dem, class ...Units>
        struct TSimplify<details::units::Units<std::ratio<Num, Dem>, typetraits::List<Units...>>>
        {
            using Result = Simplify<Multiplying<Simplify<Units>...>>;
        };

        template<class Unit>
        struct TSimplify<TDegree<Unit, 1>>
        {
            using Result = Unit;
        };

        template<class Unit>
        struct TSimplify<Multiplying<Unit>>
        {
            using Result = Simplify<Unit>;
        };

        template<class UnitFrom, class UnitTo>
        struct Convert
        {
            using Coefficient = std::conditional_t <
                std::is_same_v<UnitFrom, UnitTo>,
                std::ratio<1>,
                std::ratio<0>
            >;
        };

        template<class UnitFrom, class UnitTo>
        using ConvertCoefficient = std::conditional_t <
            std::is_same_v<
                GetCoefficient<Convert<typename UnitFrom::Dimension, typename UnitTo::Dimension>>, std::ratio<0>
            >,
            lib::typetraits::If <
                std::is_same_v<
                    GetCoefficient<Convert<typename UnitTo::Dimension, typename UnitFrom::Dimension>>, std::ratio<0>
                >,
                lib::typetraits::Constant, lib::typetraits::List<std::ratio<0>>,
                std::ratio_divide, lib::typetraits::List<
                    std::ratio<1>,
                    GetCoefficient<Convert<typename UnitTo::Dimension, typename UnitFrom::Dimension>>
                >
            >,
            GetCoefficient<Convert<typename UnitFrom::Dimension, typename UnitTo::Dimension>>
        >;

        template<class UnitFrom, class UnitTo>
        constexpr inline bool can_convert = !std::is_same_v<ConvertCoefficient<UnitFrom, UnitTo>, std::ratio<0>>;

        template<class UnitFrom, class UnitTo, int P>
        struct Convert<TDegree<UnitFrom, P>, TDegree<UnitTo, P>>
        {
            using ResultCoefficient = std::conditional_t <
                std::is_same_v<ConvertCoefficient<UnitFrom, UnitTo>, std::ratio<0>>,
                lib::typetraits::If <
                    std::is_same_v<ConvertCoefficient<UnitTo, UnitFrom>, std::ratio<0>>,
                    lib::typetraits::Constant, lib::typetraits::List<std::ratio<0>>,
                    std::ratio_divide, lib::typetraits::List<std::ratio<1>, ConvertCoefficient<UnitTo, UnitFrom>>
                >,
                ConvertCoefficient<UnitFrom, UnitTo>
            >;

            template<class Ratio, class Pow>
            using RatioPow = lib::RatioPow<Ratio, Pow::value>;

            using Coefficient = lib::typetraits::If <
                std::is_same_v<ResultCoefficient, std::ratio<0>>,
                lib::typetraits::Constant, lib::typetraits::List<std::ratio<0>>,
                RatioPow, lib::typetraits::List<ResultCoefficient, lib::typetraits::Value<P>>
            >;
        };

        template<class ...UnitsFrom, class ...UnitsTo>
        struct Convert<Multiplying<UnitsFrom ...>, Multiplying<UnitsTo ...>>
        {
            using Unit = lib::details::units::Merge<
                Canonical<Multiplying<UnitsTo ...>>,
                Canonical<Simplify<Degree<Multiplying<UnitsFrom ...>, -1>>>
            >;
            using Coefficient = std::conditional_t<
                std::is_same_v<typename Unit::Dimension, lib::typetraits::List<>>,
                GetCoefficient<Unit>,
                std::ratio<0>
            >;
        };
    }

    namespace details::units::impl {
        template<class Ratio, class Head, int HeadP, class ...Tail, class Unit, int UnitP>
        struct InsertF<Units<Ratio, List<lib::units::TDegree<Head, HeadP>, Tail...>>, lib::units::TDegree<Unit, UnitP>>
        {
            constexpr static inline auto can_convert_head = lib::units::can_convert<Head, Unit>;
            constexpr static inline auto can_convert_tail = (lib::units::can_convert<Tail, lib::units::TDegree<Unit, UnitP>> || ...);
            using Result = std::conditional_t<
                can_convert_head,
                Units<
                    std::ratio_multiply<
                        Ratio,
                        lib::units::ConvertCoefficient<
                            lib::units::TDegree<Head, HeadP>, lib::units::TDegree<Unit, HeadP>
                        >
                    >,
                    std::conditional_t<
                        HeadP + UnitP == 0,
                        List<Tail...>,
                        List<lib::units::TDegree<Head, HeadP + UnitP>, Tail...>
                    >
                >,
                std::conditional_t<
                    (lib::type_name<Unit> < lib::type_name<Head>) && (!can_convert_tail),
                    Units<Ratio, List<lib::units::TDegree<Unit, UnitP>, lib::units::TDegree<Head, HeadP>, Tail...>>,
                    InsertFront<
                        typename InsertF<
                            Units<Ratio, List<Tail...>>,
                            lib::units::TDegree<Unit, UnitP>
                        >::Result,
                        lib::units::TDegree<Head, HeadP>
                    >
                >
            >;
        };
    }

    namespace units {
        template<class ...Units>
        struct TMultiply
        {
            using Result = details::units::Merge<Canonical<Units>...>;
        };

        template<class ...Units>
        using Multiply = Simplify<typename TMultiply<typename Units::Dimension...>::Result>;

        template<class UnitA, class UnitB>
        using Divide = Multiply<UnitA, Simplify<Degree<typename UnitB::Dimension, -1>>>;
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
            std::ratio<
                gcd(RatioA::num, RatioB::num),
                lcm(RatioA::den, RatioB::den)
            >
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

    template<class ToQuantity, class IUnit, class IType, class IRatio>
    constexpr ToQuantity quantity_cast(const QuantityBase<IUnit, IType, IRatio>& quantity) noexcept
    {
        using OUnit = typename ToQuantity::Unit;
        using OType = typename ToQuantity::Type;
        using ORatio = typename ToQuantity::Ratio;

        static_assert(units::can_convert<IUnit, OUnit>);
        using Coefficient = units::ConvertCoefficient<IUnit, OUnit>;
        return ToQuantity(cast<ORatio, OType, std::ratio_divide<IRatio, Coefficient>, IType>(quantity.count()));
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
            class IUnit, class Other, class ORatio,
            typename = lib::Require<
                std::is_convertible_v<const Other&, Type>,
                std::is_floating_point_v<Type> || ((std::ratio_divide<ORatio, Ratio>::den == 1) && !std::is_floating_point_v<Other>)
            >
        >
        constexpr QuantityBase(const QuantityBase<IUnit, Other, ORatio>& other) noexcept
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
    constexpr auto operator + (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        using CQuantity = CommonQuantity<Quantity<Unit, A, ARatio>, Quantity<Unit, B, BRatio>>;
        return CQuantity(CQuantity(a).count() + CQuantity(b).count());
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator - (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        using CQuantity = CommonQuantity<Quantity<Unit, A, ARatio>, Quantity<Unit, B, BRatio>>;
        return CQuantity(CQuantity(a).count() - CQuantity(b).count());
    }

    template<class AUnit, class A, class ARatio, class BUnit, class B, class BRatio>
    constexpr auto operator * (const Quantity<AUnit, A, ARatio>& a, const Quantity<BUnit, B, BRatio>& b) noexcept
    {
        using Unit = typename units::TMultiply<typename AUnit::Dimension, typename BUnit::Dimension>::Result;
        using Type = std::common_type_t<A, B>;

        using CQuantity = Quantity<
            typename units::Dimension<units::Simplify<Unit>>::Type,
            Type,
            lib::RatioMultiply<ARatio, BRatio, units::GetCoefficient<Unit>>
        >;
        return CQuantity(static_cast<Type>(a.count()) * static_cast<Type>(b.count()));
    }

    template<class AUnit, class A, class ARatio, class BUnit, class B, class BRatio>
    constexpr auto operator / (const Quantity<AUnit, A, ARatio>& a, const Quantity<BUnit, B, BRatio>& b) noexcept
    {
        using Unit = typename units::TMultiply<
            typename AUnit::Dimension,
            units::Simplify<units::Degree<typename BUnit::Dimension, -1>>
        >::Result;
        using Type = std::common_type_t<A, B>;

        constexpr auto num = gcd(ARatio::num, BRatio::den);
        constexpr auto den = ARatio::den * BRatio::num;

        using Ratio = std::ratio_divide<std::ratio<num>, std::ratio<den>>;
        using CQuantity = Quantity<
            typename units::Dimension<units::Simplify<Unit>>::Type,
            Type,
            std::ratio_multiply<units::GetCoefficient<Unit>, Ratio>
        >;

        return CQuantity(
            (static_cast<Type>(a.count()) * lcm(ARatio::num, BRatio::den))
            /
            (static_cast<Type>(b.count()))
        );
    }

    template<class UnitA, class A, class ARatio, class UnitB, class B, class BRatio>
    constexpr auto operator == (const Quantity<UnitA, A, ARatio>& a, const Quantity<UnitB, B, BRatio>& b) noexcept
    {
        static_assert(units::can_convert<UnitA, UnitB>);
        using CoefficientAB = units::ConvertCoefficient<UnitA, UnitB>;
        using CoefficientBA = units::ConvertCoefficient<UnitB, UnitA>;
        
        if constexpr (std::ratio_greater_v<CoefficientAB, CoefficientBA>) {
            using CQuantity = CommonQuantity<
                Quantity<UnitA, A, ARatio>,
                Quantity<UnitA, B, BRatio>
            >;

            return CQuantity(a).count() == CQuantity(b).count();
        } else {
            using CQuantity = CommonQuantity<
                Quantity<UnitB, A, ARatio>,
                Quantity<UnitB, B, BRatio>
            >;

            return CQuantity(a).count() == CQuantity(b).count();
        }
    }

    template<class UnitA, class A, class ARatio, class UnitB, class B, class BRatio>
    constexpr auto operator != (const Quantity<UnitA, A, ARatio>& a, const Quantity<UnitB, B, BRatio>& b) noexcept
    {
        return !(a == b);
    }

    template<class UnitA, class A, class ARatio, class UnitB, class B, class BRatio>
    constexpr auto operator < (const Quantity<UnitA, A, ARatio>& a, const Quantity<UnitB, B, BRatio>& b) noexcept
    {
        static_assert(units::can_convert<UnitA, UnitB>);
        using CoefficientAB = units::ConvertCoefficient<UnitA, UnitB>;
        using CoefficientBA = units::ConvertCoefficient<UnitB, UnitA>;
        
        if constexpr (std::ratio_greater_v<CoefficientAB, CoefficientBA>) {
            using CQuantity = CommonQuantity<
                Quantity<UnitA, A, ARatio>,
                Quantity<UnitA, B, BRatio>
            >;

            return CQuantity(a).count() < CQuantity(b).count();
        } else {
            using CQuantity = CommonQuantity<
                Quantity<UnitB, A, ARatio>,
                Quantity<UnitB, B, BRatio>
            >;

            return CQuantity(a).count() < CQuantity(b).count();
        }
    }

    template<class UnitA, class A, class ARatio, class UnitB, class B, class BRatio>
    constexpr auto operator <= (const Quantity<UnitA, A, ARatio>& a, const Quantity<UnitB, B, BRatio>& b) noexcept
    {
        static_assert(units::can_convert<UnitA, UnitB>);
        using CoefficientAB = units::ConvertCoefficient<UnitA, UnitB>;
        using CoefficientBA = units::ConvertCoefficient<UnitB, UnitA>;
        
        if constexpr (std::ratio_greater_v<CoefficientAB, CoefficientBA>) {
            using CQuantity = CommonQuantity<
                Quantity<UnitA, A, ARatio>,
                Quantity<UnitA, B, BRatio>
            >;

            return CQuantity(a).count() <= CQuantity(b).count();
        } else {
            using CQuantity = CommonQuantity<
                Quantity<UnitB, A, ARatio>,
                Quantity<UnitB, B, BRatio>
            >;

            return CQuantity(a).count() <= CQuantity(b).count();
        }
    }

    template<class UnitA, class A, class ARatio, class UnitB, class B, class BRatio>
    constexpr auto operator > (const Quantity<UnitA, A, ARatio>& a, const Quantity<UnitB, B, BRatio>& b) noexcept
    {
        static_assert(units::can_convert<UnitA, UnitB>);
        using CoefficientAB = units::ConvertCoefficient<UnitA, UnitB>;
        using CoefficientBA = units::ConvertCoefficient<UnitB, UnitA>;
        
        if constexpr (std::ratio_greater_v<CoefficientAB, CoefficientBA>) {
            using CQuantity = CommonQuantity<
                Quantity<UnitA, A, ARatio>,
                Quantity<UnitA, B, BRatio>
            >;

            return CQuantity(a).count() > CQuantity(b).count();
        } else {
            using CQuantity = CommonQuantity<
                Quantity<UnitB, A, ARatio>,
                Quantity<UnitB, B, BRatio>
            >;

            return CQuantity(a).count() > CQuantity(b).count();
        }
    }

    template<class UnitA, class A, class ARatio, class UnitB, class B, class BRatio>
    constexpr auto operator >= (const Quantity<UnitA, A, ARatio>& a, const Quantity<UnitB, B, BRatio>& b) noexcept
    {
        static_assert(units::can_convert<UnitA, UnitB>);
        using CoefficientAB = units::ConvertCoefficient<UnitA, UnitB>;
        using CoefficientBA = units::ConvertCoefficient<UnitB, UnitA>;
        
        if constexpr (std::ratio_greater_v<CoefficientAB, CoefficientBA>) {
            using CQuantity = CommonQuantity<
                Quantity<UnitA, A, ARatio>,
                Quantity<UnitA, B, BRatio>
            >;

            return CQuantity(a).count() >= CQuantity(b).count();
        } else {
            using CQuantity = CommonQuantity<
                Quantity<UnitB, A, ARatio>,
                Quantity<UnitB, B, BRatio>
            >;

            return CQuantity(a).count() >= CQuantity(b).count();
        }
    }
}
