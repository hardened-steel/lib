#pragma once
#include <type_traits>
#include <lib/typetraits/if.hpp>

namespace lib::typetraits::interpreter {

    namespace impl {
        template <class List, class IContext, template <class Context, class Element> class Action>
        struct ForF;

        template <class List, class IContext, template <class Context, class Element> class Action>
        using For = typename ForF<List, IContext, Action>::Result;

        template <template <class...> class TList, class Head, class ...Tail, class IContext, template <class Context, class Element> class Action>
        struct ForF<TList<Head, Tail...>, IContext, Action>
        {
            using ActionResult = Action<IContext, Head>;

            template <class T>
            using Next = For<List<Tail...>, T, Action>;

            using Result = If<
                !ActionResult::Break,
                Next, List<typename ActionResult::Result>,
                Constant, List<typename ActionResult::Result>
            >;
        };

        template <template <class...> class TList, class IContext, template <class Context, class Element> class Action>
        struct ForF<TList<>, IContext, Action>
        {
            using Result = IContext;
        };
    }
}
