#pragma once
#include <lib/fp/base.function.hpp>
#include <lib/fp/details/function.hpp>
#include <lib/fp/impl.value.hpp>
#include <lib/fp/details/signature.parser.hpp>

#include <memory>
#include <utility>


namespace lib::fp {
    namespace details {
        template <class TImpl, class ...Ts>
        struct ImplFCallAddF;

        template <class F, class ...TArgs, class ...Ts>
        struct ImplFCallAddF<ImplFCall<F, TArgs...>, Ts...>
        {
            using Type = ImplFCall<F, TArgs..., Ts...>;
        };

        template <class TImpl, class ...Ts>
        using ImplFCallAdd = typename ImplFCallAddF<TImpl, Ts...>::Type;
    }

    namespace details {
        template <class Ts, class Impl>
        struct FnTypeF;

        template <class ...Ts, class Impl>
        struct FnTypeF<typetraits::List<Ts...>, Impl>
        {
            using Type = Fn<signature<Ts...>, Impl>;
        };

        template <class Ts, class Impl>
        using FnType = typename FnTypeF<Ts, Impl>::Type;
    }

    struct any {}; // NOLINT

    template <class T, class Function, class ...TArgs, Signature<T> Signature>
    class Fn<Signature, ImplFCall<Function, TArgs...>>
    {
        friend Fn<signature<T>, Dynamic>;

    public: // public types
        using ITypes = typetraits::List<>;
        using IParam = void;
        using Result = T;
        using TFImpl = ImplFCall<Function, TArgs...>;

        using PType = std::tuple<details::Wrapper<TArgs>...>;
        using FunctionResult = details::FunctionResult<Function, T, TArgs...>;

    private:
        std::shared_ptr<FunctionResult> result;

    public: // object interface
        Fn(FunctionPtr<Function>&& function) requires (sizeof...(TArgs) == 0)
        : result(std::make_shared<FunctionResult>(std::move(function), PType()))
        {}

        Fn(FunctionPtr<Function>&& function, PType&& args) requires (sizeof...(TArgs) != 0)
        : result(std::make_shared<FunctionResult>(std::move(function), std::move(args)))
        {}

        Fn(const FunctionPtr<Function>& function) requires (sizeof...(TArgs) == 0)
        : result(std::make_shared<FunctionResult>(function, PType()))
        {}

        Fn(const FunctionPtr<Function>& function, const PType& args) requires (sizeof...(TArgs) != 0)
        : result(std::make_shared<FunctionResult>(function, args))
        {}

        Fn(Function&& function) requires (sizeof...(TArgs) == 0 && !std::is_same_v<FunctionPtr<Function>, Function>)
        : result(std::make_shared<FunctionResult>(details::CreateFunctionPtr(std::move(function)), PType()))
        {}

        Fn(Function&& function, PType&& args) requires (sizeof...(TArgs) != 0 && !std::is_same_v<FunctionPtr<Function>, Function>)
        : result(std::make_shared<FunctionResult>(details::CreateFunctionPtr(std::move(function)), std::move(args)))
        {}

        Fn(const Function& function) requires (sizeof...(TArgs) == 0 && !std::is_same_v<FunctionPtr<Function>, Function>)
        : result(std::make_shared<FunctionResult>(details::CreateFunctionPtr(function), PType()))
        {}

        Fn(const Function& function, const PType& args) requires (sizeof...(TArgs) != 0 && !std::is_same_v<FunctionPtr<Function>, Function>)
        : result(std::make_shared<FunctionResult>(details::CreateFunctionPtr(function), args))
        {}

        Fn() = delete;
        Fn(const Fn& other) = default;
        Fn(Fn&& other) = default;
        Fn& operator=(const Fn& other) = default;
        Fn& operator=(Fn&& other) = default;

    public: // currying interface
        const Fn& operator () () const noexcept
        {
            return *this;
        }

    public: // runtime interface
        const auto& operator()(IExecutor& executor) const
        {
            return result->wait(executor);
        }

        auto& operator * () const noexcept
        {
            return *this;
        }

        [[nodiscard]] bool ready() const noexcept
        {
            return result->ready();
        }

        [[nodiscard]] const T& get() const noexcept
        {
            return result->get();
        }

        bool subscribe(IExecutor& executor, IAwaiter& awaiter) noexcept
        {
            return result->subscribe(executor, awaiter);
        }
    };

    template <class Function>
    requires std::is_invocable_v<std::remove_cvref_t<Function>>
    Fn(Function&& function) -> Fn<signature<std::invoke_result_t<std::remove_cvref_t<Function>>>, ImplFCall<std::remove_cvref_t<Function>>>;

    unittest {
        SimpleExecutor executor;

        Fn fcall = [] { return 42; };
        check(fcall(executor) == 42);
    };

    template <class ...Ts, Signature<Ts...> FSignature, class Impl>
    class Fn<FSignature, Impl>
    {
    public: // public types
        using ITypes = typetraits::PopBack<typetraits::List<Ts...>>;
        using IParam = typetraits::Head<typetraits::List<Ts...>>;
        using Result = typetraits::Last<typetraits::List<Ts...>>;
        using TFImpl = Impl;

        using TArgs = typetraits::List<Ts...>;
        using PType = typename TFImpl::Params;
        using Function = typename TFImpl::Function;

    private:
        FunctionPtr<Function> function;
        PType args;

    public: // object interface
        Fn(FunctionPtr<Function>&& function)
            requires (std::tuple_size_v<PType> == 0)
        : function(std::move(function)), args(PType())
        {}

        Fn(FunctionPtr<Function>&& function, PType&& args)
            requires (std::tuple_size_v<PType> != 0)
        : function(std::move(function)), args(std::move(args))
        {}

        Fn(const FunctionPtr<Function>& function)
            requires (std::tuple_size_v<PType> == 0)
        : function(function), args(PType())
        {}

        Fn(const FunctionPtr<Function>& function, const PType& args)
            requires (std::tuple_size_v<PType> != 0)
        : function(function), args(args)
        {}

        Fn(Function&& function)
            requires (std::tuple_size_v<PType> == 0 && !std::is_same_v<FunctionPtr<Function>, Function>)
        : function(details::CreateFunctionPtr(std::move(function))), args(PType())
        {}

        Fn(Function&& function, PType&& args)
            requires (std::tuple_size_v<PType> != 0 && !std::is_same_v<FunctionPtr<Function>, Function>)
        : function(details::CreateFunctionPtr(std::move(function))), args(std::move(args))
        {}

        Fn(const Function& function)
            requires (std::tuple_size_v<PType> == 0 && !std::is_same_v<FunctionPtr<Function>, Function>)
        : function(details::CreateFunctionPtr(function)), args(PType())
        {}

        Fn(const Function& function, const PType& args)
            requires (std::tuple_size_v<PType> != 0 && !std::is_same_v<FunctionPtr<Function>, Function>)
        : function(details::CreateFunctionPtr(function)), args(args)
        {}

        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

    public: // currying interface
        template <class ...Args>
        requires (
            sizeof...(Args) <= ITypes::size
            &&
            sizeof...(Args) != 0
        )
        auto operator () (Args&& ...iargs) const
        {
            using NewTypeList = typetraits::PopFront<typetraits::List<Ts...>, sizeof...(Args)>;
            using OType = details::FnType<NewTypeList, details::ImplFCallAdd<TFImpl, details::Wrapper<Args> ...>>;
            return OType(function, std::tuple_cat(args, std::make_tuple(details::wrap(std::forward<Args>(iargs)) ...)));
        }

        auto& get() noexcept
        {
            return *this;
        }

        auto& get() const noexcept
        {
            return *this;
        }
    };

    template <class R, class ...TArgs>
    Fn(R(*function)(TArgs...)) -> Fn<signature<TArgs..., R>, ImplFCall<R(*)(TArgs...)>>;

    unittest {
        SimpleExecutor executor;

        int (*function)(int, int) = [] (int lhs, int rhs) noexcept {
            return lhs + rhs;
        };
        Fn sum = function;
        check(sum(1, 2)(executor) == 3);
        check(sum(3)(2)(executor) == 5);
    }
}
