#pragma once
#include <numeric>
#include <ratio>
#include <lib/concept.hpp>
#include <lib/typename.hpp>

namespace lib {

    namespace units {
        template<class ...Units>
        struct Multiplying
        {
            using Type = Multiplying<Units...>;
            using Dimension = Type;
        };

        template<class T, int P = 1>
        struct Degree
        {
            using Type = Degree;
            using Dimension = Type;
        };

        template<class T>
        struct Degree<T, 0>
        {
            using Type = Multiplying<>;
        };

        template<class T>
        struct Degree<T, 1>
        {
            using Type = T;
        };

        template<class ...Units, int P>
        struct Degree<Multiplying<Units...>, P>
        {
            using Type = Multiplying<typename Degree<Units, P>::Type...>;
        };

        template<class T, int P1, int P2>
        struct Degree<Degree<T, P1>, P2>
        {
            using Type = typename Degree<T, P1 * P2>::Type;
        };

        template<class T, int P>
        struct Degree<Degree<T, P>, 1>
        {
            using Type = typename Degree<T, P>::Type;
        };

        template<class Unit>
        struct Multiplying<Unit>
        {
            using Type = Unit;
            using Dimension = Type;
        };

        template<class Unit, int P>
        struct Multiplying<Degree<Unit, P>>
        {
            using Type = typename Degree<Unit, P>::Type;
        };

        template<class T>
        struct Dimension
        {
            using Type = T;
        };
    }

    namespace details::units {

        template<class ...Units>
        using Multiplying = lib::units::Multiplying<Units...>;

        template<class ...Units>
        using MultiplyingT = typename Multiplying<Units...>::Type;

        template<class T, int P>
        using Degree = lib::units::Degree<T, P>;

        template<class T, int P>
        using DegreeT = typename Degree<T, P>::Type;

        template<class Unit>
        struct Simplefy
        {
            using Type = Unit;
        };

        template<class Unit>
        using SimplefyT = typename Simplefy<Unit>::Type;

        template<class ...Units>
        struct Simplefy<lib::units::Multiplying<Units...>>
        {
            using Type = MultiplyingT<SimplefyT<Units>...>;
        };

        template<class Unit, int P>
        struct Simplefy<Degree<Unit, P>>
        {
            using Type = DegreeT<SimplefyT<Unit>, P>;
        };

        template<class Unit>
        struct Simplefy<Multiplying<Unit>>
        {
            using Type = SimplefyT<Unit>;
        };

        template<class UnitsA, class UnitsB>
        struct ConcatT;

        template<class UnitsA, class UnitsB>
        using Concat = typename ConcatT<UnitsA, UnitsB>::Result;

        template<class ...UnitsA, class ...UnitsB>
        struct ConcatT<Multiplying<UnitsA...>, Multiplying<UnitsB...>>
        {
            using Result = Multiplying<UnitsA..., UnitsB...>;
        };

        template<class UnitA, class ...UnitsB>
        struct ConcatT<UnitA, Multiplying<UnitsB...>>
        {
            using Result = Multiplying<UnitA, UnitsB...>;
        };

        template<class UnitA, class UnitB>
        struct CompareT
        {
            constexpr static inline bool Result = CompareT<Degree<UnitA, 1>, Degree<UnitB, 1>>::Result;
        };

        template<class UnitA, class UnitB>
        constexpr static inline bool Compare = CompareT<UnitA, UnitB>::Result;

        template<class UnitA, int Pa, class UnitB, int Pb>
        struct CompareT<Degree<UnitA, Pa>, Degree<UnitB, Pb>>
        {
            constexpr static inline bool Result = lib::type_name<UnitA> < lib::type_name<UnitB>;
        };

        template<class UnitA, class UnitB, int Pb>
        struct CompareT<UnitA, Degree<UnitB, Pb>>
        {
            constexpr static inline bool Result = Compare<Degree<UnitA, 1>, Degree<UnitB, Pb>>;
        };

        template<class UnitA, int Pa, class UnitB>
        struct CompareT<Degree<UnitA, Pa>, UnitB>
        {
            constexpr static inline bool Result = Compare<Degree<UnitA, Pa>, Degree<UnitB, 1>>;
        };

        template<class UnitA, class UnitB>
        struct MultiplyT
        {
            using Result = std::conditional_t<Compare<UnitA, UnitB>, Multiplying<UnitA, UnitB>, Multiplying<UnitB, UnitA>>;
        };

        template<class UnitA, class UnitB>
        using Multiply = SimplefyT<typename MultiplyT<UnitA, UnitB>::Result>;

        /*template<class UnitA, int Pa, class UnitB, int Pb>
        struct MultiplyT<Degree<UnitA, Pa>, Degree<UnitB, Pb>>
        {
            using Result = std::conditional_t<Compare<Degree<UnitA, Pa>, Degree<UnitB, Pb>>, Multiplying<DegreeT<UnitA, Pa>, DegreeT<UnitB, Pb>>, Multiplying<DegreeT<UnitB, Pb>, DegreeT<UnitA, Pa>>>;
        };*/

        template<class Unit, int Pb>
        struct MultiplyT<Unit, Degree<Unit, Pb>>
        {
            using Result = DegreeT<Unit, Pb + 1>;
        };

        template<class Unit, int Pa>
        struct MultiplyT<Degree<Unit, Pa>, Unit>
        {
            using Result = DegreeT<Unit, Pa + 1>;
        };

        template<class Unit, int Pa, int Pb>
        struct MultiplyT<Degree<Unit, Pa>, Degree<Unit, Pb>>
        {
            using Result = DegreeT<Unit, Pa + Pb>;
        };

        template<class Unit>
        struct MultiplyT<Unit, Unit>
        {
            using Result = DegreeT<Unit, 2>;
        };

        template<class Units, class Unit>
        struct MultiplyInsertT;

        template<class Units, class Unit>
        using MultiplyInsert = typename MultiplyInsertT<Units, Unit>::Result;

        template<class Head, class ...Tail, class Unit>
        struct MultiplyInsertT<Multiplying<Head, Tail...>, Unit>
        {
            using Result = std::conditional_t<
                Compare<Unit, Head>,
                Multiplying<Unit, Head, Tail...>,
                std::conditional_t<
                    Compare<Head, Unit>,
                    typename MultiplyT<Multiplying<Head>, MultiplyInsert<Multiplying<Tail...>, Unit>>::Result,
                    typename MultiplyT<Multiply<Head, Unit>, Multiplying<Tail...>>::Result
                >
            >;
        };

        template<class Unit>
        struct MultiplyInsertT<Multiplying<>, Unit>
        {
            using Result = Multiplying<Unit>;
        };

        template<class ...Units, class Unit>
        struct MultiplyT<Multiplying<Units...>, Unit>
        {
            using Result = MultiplyInsert<Multiplying<Units...>, Unit>;
        };

        template<class ...Units, class Unit>
        struct MultiplyT<Unit, Multiplying<Units...>>
        {
            using Result = MultiplyInsert<Multiplying<Units...>, Unit>;
        };

        template<class UnitsA, class UnitsB>
        struct MultiplyInsertsT;

        template<class UnitsA, class UnitsB>
        using MultiplyInserts = typename MultiplyInsertsT<UnitsA, UnitsB>::Result;

        template<class ...UnitsA, class Unit, class ...UnitsB>
        struct MultiplyInsertsT<Multiplying<UnitsA...>, Multiplying<Unit, UnitsB...>>
        {
            using Result = MultiplyInserts<MultiplyInsert<Multiplying<UnitsA...>, Unit>, Multiplying<UnitsB...>>;
        };

        template<class ...UnitsA>
        struct MultiplyInsertsT<Multiplying<UnitsA...>, Multiplying<>>
        {
            using Result = Multiplying<UnitsA...>;
        };

        template<class ...UnitsA, class ...UnitsB>
        struct MultiplyT<Multiplying<UnitsA...>, Multiplying<UnitsB...>>
        {
            using Result = MultiplyInserts<Multiplying<UnitsA...>, Multiplying<UnitsB...>>;
        };

        template<>
        struct MultiplyT<Multiplying<>, Multiplying<>>
        {
            using Result = Multiplying<>;
        };
    }

    namespace units {
        template<class ...Units>
        struct MultiplyT;

        template<class ...Units>
        using Multiply = typename MultiplyT<Units...>::Type;

        template<class Unit, class ...Units>
        struct MultiplyT<Unit, Units...>
        {
            using Type = details::units::Multiply<Unit, Multiply<Units...>>;
        };

        template<class UnitA, class UnitB>
        struct MultiplyT<UnitA, UnitB>
        {
            using Type = details::units::Multiply<UnitA, UnitB>;
        };

        template<>
        struct MultiplyT<>
        {
            using Type = Multiplying<>;
        };

        template<class UnitA, class UnitB>
        using Devide = Multiply<UnitA, details::units::SimplefyT<typename Degree<UnitB, -1>::Type>>;
    }

    template<class Unit, class T, class Ratio = std::ratio<1>>
    class Quantity;

    template<class U, class T, class R>
    class QuantityBase;

    template<class ToQuantity, class Unit, class IType, class IRatio>
    constexpr ToQuantity quantity_cast(const QuantityBase<Unit, IType, IRatio>& quantity) noexcept;

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
        using Unit  = typename units::Dimension<units::Multiply<typename AUnit::Dimension, typename BUnit::Dimension>>::Type;
        using Ratio = std::ratio_multiply<ARatio, BRatio>;

        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;
        return CommonQuantity(cast<Ratio, Type, ARatio>(a.count()) * cast<Ratio, Type, BRatio>(b.count()));
    }

    template<class AUnit, class A, class ARatio, class BUnit, class B, class BRatio>
    constexpr auto operator/ (const Quantity<AUnit, A, ARatio>& a, const Quantity<BUnit, B, BRatio>& b) noexcept
    {
        using Unit = typename units::Dimension<units::Devide<typename AUnit::Dimension, typename BUnit::Dimension>>::Type;
        using Type = std::common_type_t<A, B>;

        if constexpr(std::ratio_less_v<BRatio, ARatio>) {
            using Ratio = std::ratio_divide<BRatio, ARatio>;
            using CommonQuantity = Quantity<Unit, Type, std::ratio<1>>;
            return CommonQuantity(
                cast<Ratio, Type, ARatio>(a.count()) / cast<Ratio, Type, BRatio>(b.count())
            );
        } else {
            using Ratio = std::ratio_divide<ARatio, BRatio>;
            using CommonQuantity = Quantity<Unit, Type, std::milli>;
            return CommonQuantity(
                cast<Ratio, Type, std::ratio_multiply<ARatio, std::kilo>>(a.count())
                /
                cast<Ratio, Type, BRatio>(b.count())
            );
        }
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator== (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        constexpr auto num = std::gcd(ARatio::num, BRatio::num);
        constexpr auto den = std::gcd(ARatio::den, BRatio::den);
        using Ratio = std::ratio<num, (ARatio::den / den) * BRatio::den>;

        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;
        return CommonQuantity(a).count() == CommonQuantity(b).count();
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator!= (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        return !(a == b);
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator< (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        constexpr auto num = std::gcd(ARatio::num, BRatio::num);
        constexpr auto den = std::gcd(ARatio::den, BRatio::den);
        using Ratio = std::ratio<num, (ARatio::den / den) * BRatio::den>;

        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;
        return CommonQuantity(a).count() < CommonQuantity(b).count();
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator<= (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        constexpr auto num = std::gcd(ARatio::num, BRatio::num);
        constexpr auto den = std::gcd(ARatio::den, BRatio::den);
        using Ratio = std::ratio<num, (ARatio::den / den) * BRatio::den>;

        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;
        return CommonQuantity(a).count() <= CommonQuantity(b).count();
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator> (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        constexpr auto num = std::gcd(ARatio::num, BRatio::num);
        constexpr auto den = std::gcd(ARatio::den, BRatio::den);
        using Ratio = std::ratio<num, (ARatio::den / den) * BRatio::den>;

        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;
        return CommonQuantity(a).count() > CommonQuantity(b).count();
    }

    template<class Unit, class A, class ARatio, class B, class BRatio>
    constexpr auto operator>= (const Quantity<Unit, A, ARatio>& a, const Quantity<Unit, B, BRatio>& b) noexcept
    {
        constexpr auto num = std::gcd(ARatio::num, BRatio::num);
        constexpr auto den = std::gcd(ARatio::den, BRatio::den);
        using Ratio = std::ratio<num, (ARatio::den / den) * BRatio::den>;

        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<Unit, Type, Ratio>;
        return CommonQuantity(a).count() >= CommonQuantity(b).count();
    }
}