#pragma once
#include <lib/test.hpp>
#include <lib/tuple.hpp>
#include <lib/mutex.hpp>
#include <lib/fp/executor.hpp>
#include <lib/typetraits/tag.hpp>
#include <lib/typetraits/list.hpp>
#include <lib/raw.storage.hpp>
#include <lib/index.sequence.hpp>
#include <lib/semaphore.hpp>

#include <memory>
#include <coroutine>


namespace lib::fp {

    template <class T, class ...Targs>
    class Fn;

    template <class Ts, class Impl>
    struct FnTypeF;

    template <class ...Ts, class Impl>
    struct FnTypeF<typetraits::List<Ts...>, Impl>
    {
        using Type = Fn<Ts..., Impl>;
    };

    template <class Ts, class Impl>
    using FnType = typename FnTypeF<Ts, Impl>::Type;


    struct Dynamic;

    template <class T>
    struct ImplValue;

    template <class T>
    struct FnWrapper
    {
        using Type = Fn<T, ImplValue<T>>;
    };

    template <class ...TArgs>
    struct FnWrapper<Fn<TArgs...>>
    {
        using Type = Fn<TArgs...>;
    };

    template <class T>
    using Wrapper = typename FnWrapper<std::remove_cvref_t<T>>::Type;

    template <class T>
    decltype(auto) wrap(T&& value)
    {
        return Wrapper<T>(std::forward<T>(value));
    }

    template <class ...TArgs>
    decltype(auto) wrap(Fn<TArgs...>&& value)
    {
        return std::move(value);
    }

    template <class ...TArgs>
    decltype(auto) wrap(const Fn<TArgs...>& value)
    {
        return value;
    }

    template <class F, class ...TArgs>
    struct ImplFCall
    {
        using Function = F;
        using PTypes = typetraits::List<TArgs...>;
        using Params = std::tuple<Wrapper<TArgs>...>;
    };

    template <class TImpl, class ...Ts>
    struct ImplFCallAddF;

    template <class F, class ...TArgs, class ...Ts>
    struct ImplFCallAddF<ImplFCall<F, TArgs...>, Ts...>
    {
        using Type = ImplFCall<F, TArgs..., Ts...>;
    };

    template <class TImpl, class ...Ts>
    using ImplFCallAdd = typename ImplFCallAddF<TImpl, Ts...>::Type;

    template <class T>
    struct IsFunctionF
    {
        constexpr static inline bool value = false;
    };

    template <class T, class ...TArgs>
    struct IsFunctionF<Fn<T, TArgs...>>
    {
        constexpr static inline bool value = true;
    };

    template <class T>
    constexpr bool is_function = IsFunctionF<T>::value;


    template <class T>
    using Val = Fn<T, Dynamic>;

    template <class T>
    class Fn<T, ImplValue<T>>
    {
    public: // public types
        using ITypes = typetraits::List<>;
        using IParam = void;
        using Result = T;
        using TFImpl = ImplValue<T>;

    private:
        T value;

    public: // object interface
        template <class ...TArgs>
        requires std::constructible_from<T, TArgs&&...>
        Fn(TArgs&& ...args) noexcept(std::is_nothrow_constructible_v<T, TArgs&&...>)
        : value(std::forward<TArgs>(args)...)
        {}

        Fn(T&& value)
            noexcept(std::is_nothrow_move_constructible_v<T>)
            requires (!std::is_array_v<T>)
        : value(std::move(value))
        {}

        Fn(const T& value)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires (!std::is_array_v<T>)
        : value(value)
        {}

        template <std::size_t ...I>
        Fn(const T& array, std::index_sequence<I...>)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires (std::is_array_v<T>)
        : value {array[I]...}
        {}

        Fn(const T& value)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires (std::is_array_v<T>)
        : Fn(value, std::make_index_sequence<std::extent_v<T>>{})
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
        const auto& operator()([[maybe_unused]] IExecutor& executor) const noexcept
        {
            return value;
        }

        auto& operator * () const noexcept
        {
            return *this;
        }

        [[nodiscard]] bool ready() const noexcept
        {
            return true;
        }

        [[nodiscard]] const T& get() const noexcept
        {
            return value;
        }

        bool subscribe([[maybe_unused]] IExecutor& executor, [[maybe_unused]] IAwaiter& awaiter)
        {
            return true;
        }
    };

    template <class T>
    requires (!std::is_invocable_v<std::remove_cvref_t<T>>)
    Fn(T&& value) -> Fn<std::remove_cvref_t<T>, ImplValue<std::remove_cvref_t<T>>>;


    namespace details {
        class Awaiter: public IAwaiter
        {
            mutable Semaphore semaphore;

        public:
            void wakeup(IExecutor*) noexcept final
            {
                semaphore.release();
            }

            void wait() noexcept
            {
                semaphore.acquire();
            }

            void wait(std::size_t count) noexcept
            {
                while (count-- > 0) {
                    semaphore.acquire();
                }
            }
        };

        template <class T>
        class IResult
        {
        public:
            [[nodiscard]] virtual bool ready() const noexcept = 0;
            [[nodiscard]] virtual const T& get() const = 0;
            [[nodiscard]] virtual bool subscribe(IExecutor& executor, IAwaiter& awaiter) = 0;
            [[nodiscard]] virtual const T& wait(IExecutor& executor) = 0;
            virtual ~IResult() noexcept = default;
        };

        template <class T>
        class Result: public IResult<T>
        {
            enum class State {
                Stop, Running, Ready, Failed
            };

            State state = State::Stop;
            RawStorage<typetraits::List<T, std::exception_ptr>> result;

            mutable data_structures::DLList subscribers;
            mutable Mutex mutex;

        private:
            void emit(IExecutor* executor) const noexcept
            {
                std::lock_guard lock(mutex);
                for (auto& subscriber: subscribers.Range<IAwaiter>()) {
                    subscriber.wakeup(executor);
                }
            }

            virtual void add_task(IExecutor&) noexcept {};

        public:
            ~Result() override
            {
                switch (state) {
                    case State::Ready:
                        result.destroy(std::in_place_type<T>);
                        break;
                    case State::Failed:
                        result.destroy(std::in_place_type<std::exception_ptr>);
                        break;
                    default:
                        break;
                }
            }

            template <class U>
            void set(IExecutor* executor, U&& value) noexcept
            {
                result.emplace(std::in_place_type<T>, std::forward<U>(value));
                state = State::Ready;
                emit(executor);
            }

            void set(typetraits::TTag<std::exception_ptr>, IExecutor* executor, const std::exception_ptr& error) noexcept
            {
                result.emplace(std::in_place_type<std::exception_ptr>, error);
                state = State::Failed;
                emit(executor);
            }

            [[nodiscard]] bool ready() const noexcept final
            {
                std::lock_guard lock(mutex);
                return state == State::Ready || state == State::Failed;
            }

            [[nodiscard]] const T& get() const final
            {
                std::lock_guard lock(mutex);
                switch (state) {
                    case State::Ready:  return *result.ptr(std::in_place_type<T>);
                    case State::Failed: rethrow_exception(*result.ptr(std::in_place_type<std::exception_ptr>));
                    default: throw std::runtime_error("value is not ready");
                }
            }

            [[nodiscard]] bool subscribe(IExecutor& executor, IAwaiter& awaiter) final
            {
                std::lock_guard lock(mutex);
                switch (state) {
                    case State::Stop:
                        subscribers.PushBack(awaiter);
                        state = State::Running;
                        add_task(executor);
                        return false;
                    case State::Running:
                        subscribers.PushBack(awaiter);
                        return false;
                    case State::Failed:
                    case State::Ready:
                        return true;
                }
                return false;
            }

            [[nodiscard]] const T& wait(IExecutor& executor) final
            {
                Awaiter awaiter;
                if (!subscribe(executor, awaiter)) {
                    executor.run();
                    awaiter.wait();
                }
                return get();
            }
        };

        template <class T>
        struct FunctionPtrT
        {
            using Type = std::shared_ptr<T>;
        };

        template <class R, class ...TArgs>
        struct FunctionPtrT<R(*)(TArgs...)>
        {
            using Type = R(*)(TArgs...);
        };

        template <class T, class ...TArgs>
        struct FunctionPtrT<Fn<T, TArgs...>>
        {
            using Type = Fn<T, TArgs...>;
        };

        template <class T>
        using FunctionPtr = typename FunctionPtrT<T>::Type;

        template <class R, class ...TArgs>
        auto* CreateFunctionPtr(const R(*function)(TArgs...)) noexcept
        {
            return function;
        }

        template <class T>
        auto CreateFunctionPtr(T&& function)
        {
            return std::make_shared<std::remove_cvref_t<T>>(std::forward<T>(function));
        }

        template <class T, class ...TArgs>
        auto& CreateFunctionPtr(const Fn<T, TArgs...>& function) noexcept
        {
            return function;
        }

        template <class T, bool Cast>
        decltype(auto) unwrap(T&& arg, typetraits::VTag<Cast>)
        {
            if constexpr (Cast) {
                return std::forward<T>(arg);
            } else {
                return std::forward<T>(arg).get();
            }
        }

        template <class T, std::size_t A, std::size_t B>
        decltype(auto) check_cast(T&& arg, typetraits::VTag<A, B>)
        {
            return unwrap(std::forward<T>(arg), typetraits::tag_v<A == B>);
        }

        template <class Function, class TArgs, std::size_t I, class Indexes>
        struct CheckCast;

        template <class Function, class TArgs, std::size_t I, std::size_t ...Is>
        struct CheckCast<Function, TArgs, I, std::index_sequence<Is...>>
        {
            constexpr static inline bool value = std::invocable<Function, decltype(check_cast(std::get<Is>(std::declval<TArgs>()), typetraits::tag_v<I, Is>))...>; 
        };

        template <class Function, std::size_t I, class TArgs>
        concept could_call_without_cast = CheckCast<Function, TArgs, I, std::make_index_sequence<std::tuple_size_v<TArgs>>>::value;

        template <class Function, class ER, class FR, class TArgs>
        class FunctionResultImpl;

        template <class Function, class R, class ...TArgs>
        class FunctionResultImpl<Function, R, R, typetraits::List<TArgs...>>
            : public details::Result<R>
            , private IExecutor::ITask
            , private IAwaiter
        {
            details::FunctionPtr<Function> function;
            std::tuple<TArgs...> args;
            mutable std::size_t args_ready = 0;

        public:
            FunctionResultImpl(details::FunctionPtr<Function>&& function, std::tuple<TArgs...> args)
                noexcept(std::is_nothrow_move_constructible_v<Function>)
            : function(std::move(function)), args(args)
            {}

            FunctionResultImpl(const details::FunctionPtr<Function>& function, std::tuple<TArgs...> args)
                noexcept(std::is_nothrow_copy_constructible_v<Function>)
            : function(function), args(args)
            {}

        private:
            void add_task(IExecutor& executor) noexcept final
            {
                executor.add(this);
            }

            template <std::size_t ...I>
            void apply(IExecutor* executor, std::index_sequence<I...>) noexcept
            try {
                details::Result<R>::set(executor, (*function)(details::unwrap(std::get<I>(args), typetraits::tag_v<details::could_call_without_cast<Function, I, std::tuple<TArgs...>>>)...));
            } catch (...) {
                details::Result<R>::set(typetraits::tag_t<std::exception_ptr>, executor, std::current_exception());
            }

            void wakeup(IExecutor* executor) noexcept final
            {
                if (++args_ready == sizeof...(TArgs)) {
                    executor->add(this);
                }
            }

            template <std::size_t I>
            std::size_t subscribe_arg(IExecutor* executor) noexcept
            {
                if constexpr (could_call_without_cast<Function, I, std::tuple<TArgs...>>) {
                    return 1U;
                } else {
                    if (std::get<I>(args).subscribe(*executor, *this)) {
                        return 1U;
                    } else {
                        return 0U;
                    }
                }
            }

            template <std::size_t ...I>
            void run(IExecutor* executor, std::index_sequence<I...>) noexcept
            {
                args_ready = (subscribe_arg<I>(executor) + ... + 0);
                if (args_ready == sizeof...(TArgs)) {
                    executor->add(this);
                }
            }

            void run(IExecutor* executor) noexcept final
            {
                if (args_ready == 0) {
                    return run(executor, std::make_index_sequence<sizeof...(TArgs)>{});
                }
                if (args_ready != 0) {
                    return apply(executor, std::make_index_sequence<sizeof...(TArgs)>{});
                }
            }
        };

        template <class Function, class R>
        class FunctionResultImpl<Function, R, R, typetraits::List<>>: public details::IResult<R>
        {
            std::optional<R> value;
            std::exception_ptr exception;

        public:
            FunctionResultImpl(const details::FunctionPtr<Function>& function, const std::tuple<>&) noexcept
            {
                try {
                    value.emplace((*function)());
                } catch (...) {
                    exception = std::current_exception();
                }
            }

            [[nodiscard]] bool ready() const noexcept final
            {
                return true;
            }

            [[nodiscard]] const R& get() const final
            {
                if (exception) {
                    std::rethrow_exception(exception);
                }
                return *value;
            }

            [[nodiscard]] bool subscribe(IExecutor&, IAwaiter&) final
            {
                return true;
            }

            [[nodiscard]] const R& wait(IExecutor&) final
            {
                return get();
            }
        };

        template <class Function, class R, class Impl, class ...TArgs>
        class FunctionResultImpl<Function, R, Fn<R, Impl>, typetraits::List<TArgs...>>
            : public details::IResult<R>
            , private IExecutor::ITask
            , private IAwaiter
        {
            details::FunctionPtr<Function> function;
            std::tuple<TArgs...> args;
            mutable std::size_t args_ready = 0;

            enum class State {
                Stop, Prepare, Running, Ready
            };
            State state = State::Stop;
            RawStorage<Fn<R, Impl>> result;
            std::exception_ptr exception = nullptr;

            mutable data_structures::DLList subscribers;
            mutable Mutex mutex;

        private:
            void emit(IExecutor* executor) const noexcept
            {
                for (auto& subscriber: subscribers.Range<IAwaiter>()) {
                    subscriber.wakeup(executor);
                }
            }

        public:
            FunctionResultImpl(const details::FunctionPtr<Function>& function, const std::tuple<TArgs...>& args)
                noexcept(std::is_nothrow_copy_constructible_v<Function>)
            : function(function), args(args)
            {}

            ~FunctionResultImpl() override
            {
                if (exception == nullptr && (state == State::Ready || state == State::Running)) {
                    result.destroy();
                }
            }

            [[nodiscard]] bool ready() const noexcept final
            {
                std::lock_guard lock(mutex);
                return state == State::Ready;
            }

            [[nodiscard]] const R& get() const final
            {
                if (exception) {
                    rethrow_exception(exception);
                }
                return result.ptr()->get();
            }

            bool subscribe(IExecutor& executor, IAwaiter& awaiter) final
            {
                std::lock_guard lock(mutex);
                switch (state) {
                    case State::Stop:
                        subscribers.PushBack(awaiter);
                        state = State::Prepare;
                        executor.add(this);
                        return false;
                    case State::Prepare:
                    case State::Running:
                        subscribers.PushBack(awaiter);
                        return false;
                    case State::Ready:
                        return true;
                }
                return false;
            }

            const R& wait(IExecutor& executor) final
            {
                Awaiter awaiter;
                if (!subscribe(executor, awaiter)) {
                    executor.run();
                    awaiter.wait();
                }
                return get();
            }

        private:
            template <std::size_t ...I>
            void apply(IExecutor* executor, std::index_sequence<I...>) noexcept
            try {
                auto& fn = *result.emplace((*function)(details::unwrap(std::get<I>(args), typetraits::tag_v<details::could_call_without_cast<Function, I, std::tuple<TArgs...>>>)...));
                if (fn.subscribe(*executor, *this)) {
                    state = State::Ready;
                    emit(executor);
                } else {
                    state = State::Running;
                }
            } catch (...) {
                state = State::Ready;
                exception = std::current_exception();
                emit(executor);
            }

            void wakeup(IExecutor* executor) noexcept final
            {
                std::lock_guard lock(mutex);
                if (state == State::Prepare) {
                    if (++args_ready == sizeof...(TArgs)) {
                        executor->add(this);
                    }
                }
                if (state == State::Running) {
                    state = State::Ready;
                    emit(executor);
                }
            }

            template <std::size_t I>
            std::size_t subscribe_arg(IExecutor* executor) noexcept
            {
                if constexpr (could_call_without_cast<Function, I, std::tuple<TArgs...>>) {
                    return 1U;
                } else {
                    if (std::get<I>(args).subscribe(*executor, *this)) {
                        return 1U;
                    } else {
                        return 0U;
                    }
                }
            }

            template <std::size_t ...I>
            void run(IExecutor* executor, std::index_sequence<I...>) noexcept
            {
                args_ready = (subscribe_arg<I>(executor) + ... + 0);
                if (args_ready == sizeof...(TArgs)) {
                    executor->add(this);
                }
            }

            void run(IExecutor* executor) noexcept final
            {
                if (args_ready != 0) {
                    return apply(executor, std::make_index_sequence<sizeof...(TArgs)>{});
                }
                if (args_ready == 0) {
                    return run(executor, std::make_index_sequence<sizeof...(TArgs)>{});
                }
            }
        };

        template <class Function, class R, class ...TArgs>
        class FunctionResult: public FunctionResultImpl<Function, R, std::invoke_result_t<Function, typename TArgs::Result ...>, typetraits::List<TArgs...>>
        {
        public:
            using FunctionResultImpl<Function, R, std::invoke_result_t<Function, typename TArgs::Result ...>, typetraits::List<TArgs...>>::FunctionResultImpl;
        };
    }

    template <class T, class Function, class ...TArgs>
    class Fn<T, ImplFCall<Function, TArgs...>>
    {
        friend Fn<T, Dynamic>;

    public: // public types
        using ITypes = typetraits::List<>;
        using IParam = void;
        using Result = T;
        using TFImpl = ImplFCall<Function, TArgs...>;

        using PType = std::tuple<Wrapper<TArgs>...>;
        using FunctionResult = details::FunctionResult<Function, T, TArgs...>;

    private:
        std::shared_ptr<FunctionResult> result;

    public: // object interface
        Fn(details::FunctionPtr<Function>&& function) requires (sizeof...(TArgs) == 0)
        : result(std::make_shared<FunctionResult>(std::move(function), PType()))
        {}

        Fn(details::FunctionPtr<Function>&& function, PType&& args) requires (sizeof...(TArgs) != 0)
        : result(std::make_shared<FunctionResult>(std::move(function), std::move(args)))
        {}

        Fn(const details::FunctionPtr<Function>& function) requires (sizeof...(TArgs) == 0)
        : result(std::make_shared<FunctionResult>(function, PType()))
        {}

        Fn(const details::FunctionPtr<Function>& function, const PType& args) requires (sizeof...(TArgs) != 0)
        : result(std::make_shared<FunctionResult>(function, args))
        {}

        Fn(Function&& function) requires (sizeof...(TArgs) == 0 && !std::is_same_v<details::FunctionPtr<Function>, Function>)
        : result(std::make_shared<FunctionResult>(details::CreateFunctionPtr(std::move(function)), PType()))
        {}

        Fn(Function&& function, PType&& args) requires (sizeof...(TArgs) != 0 && !std::is_same_v<details::FunctionPtr<Function>, Function>)
        : result(std::make_shared<FunctionResult>(details::CreateFunctionPtr(std::move(function)), std::move(args)))
        {}

        Fn(const Function& function) requires (sizeof...(TArgs) == 0 && !std::is_same_v<details::FunctionPtr<Function>, Function>)
        : result(std::make_shared<FunctionResult>(details::CreateFunctionPtr(function), PType()))
        {}

        Fn(const Function& function, const PType& args) requires (sizeof...(TArgs) != 0 && !std::is_same_v<details::FunctionPtr<Function>, Function>)
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

        bool subscribe(IExecutor& executor, IAwaiter& awaiter)
        {
            return result->subscribe(executor, awaiter);
        }
    };

    template <class Function>
    requires std::is_invocable_v<std::remove_cvref_t<Function>>
    Fn(Function&& function) -> Fn<std::invoke_result_t<std::remove_cvref_t<Function>>, ImplFCall<std::remove_cvref_t<Function>>>;


    namespace details {
        template <class T>
        class Promise: private IAwaiter, private IExecutor::ITask
        {
        public:
            using Type = T;
            using Handle = std::coroutine_handle<Promise>;
            class CoroResult: public Result<Type>
            {
                Promise* promise;

            public:
                CoroResult(Promise* promise) noexcept
                : promise(promise)
                {}

                void add_task(IExecutor& executor) noexcept final
                {
                    executor.add(promise);
                }
            };

        private:
            std::shared_ptr<CoroResult> result = std::make_shared<CoroResult>(this);
            mutable IExecutor* executor = nullptr;

        private:
            void run(IExecutor* executor) noexcept final
            {
                this->executor = executor;
                Handle::from_promise(*this).resume();
            }

            void wakeup(IExecutor* executor) noexcept final
            {
                executor->add(this);
            }

        public:
            [[nodiscard]] Val<Type> get_return_object() const noexcept
            {
                return Val<Type>(typetraits::tag_t<CoroResult>, result);
            }

            auto initial_suspend() const noexcept // NOLINT
            {
                return std::suspend_always{};
            }

            auto final_suspend() const noexcept // NOLINT
            {
                return std::suspend_never{};
            }

            void unhandled_exception() noexcept
            {
                result->set(typetraits::tag_t<std::exception_ptr>, executor, std::current_exception());
            }

            template <class U>
            void return_value(U&& value)
            {
                result->set(executor, std::forward<U>(value));
            }

            template <class U>
            [[nodiscard]] auto await_transform(U&& value) const noexcept
            {
                struct Awaiter: std::suspend_never
                {
                    U&& value;

                    [[nodiscard]] U&& await_resume() const noexcept
                    {
                        return std::forward<U>(value);
                    }
                };
                return Awaiter{{}, std::forward<U>(value)};
            }

            template <class U, class Impl>
            [[nodiscard]] auto await_transform(Fn<U, Impl> function) noexcept(std::is_nothrow_move_constructible_v<Fn<U, Impl>>)
            {
                class Awaiter
                {
                    using Function = Fn<U, Impl>;
                    Function function;

                public:
                    Awaiter(Function function) noexcept(std::is_nothrow_move_constructible_v<Function>)
                    : function(std::move(function))
                    {}

                public:
                    [[nodiscard]] bool await_ready() const noexcept
                    {
                        return function.ready();
                    }

                    [[nodiscard]] bool await_suspend(std::coroutine_handle<Promise> handle) noexcept
                    {
                        auto& promise = handle.promise();
                        auto* executor = promise.executor;
                        return !function.subscribe(*executor, promise);
                    }

                    [[nodiscard]] decltype(auto) await_resume() const
                    {
                        return function.get();
                    }
                };

                return Awaiter(std::move(function));
            }
        };
    }

    template <class T>
    class Fn<T, Dynamic>
    {
        friend details::Promise<T>;

    public: // public types
        using ITypes = typetraits::List<>;
        using IParam = void;
        using Result = T;
        using TFImpl = Dynamic;

        using promise_type = details::Promise<T>;   

    private:
        std::shared_ptr<details::IResult<T>> result;  

    private:
        explicit Fn(typetraits::TTag<typename details::Promise<T>::CoroResult>, std::shared_ptr<details::IResult<T>> result) noexcept
        : result(std::move(result))
        {}

    public: // object interface
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (!std::is_same_v<std::remove_cvref_t<Function>, Fn> && std::is_invocable_r_v<T, std::remove_cvref_t<Function>>)
        Fn(Function&& function)
        : result(std::make_shared<typename Fn<T, ImplFCall<std::remove_cvref_t<Function>>>::FunctionResult>(std::forward<Function>(function)))
        {}

        template <class Function, class ...TArgs>
        Fn(const Fn<T, ImplFCall<Function, TArgs...>>& function) noexcept
        : result(function.result)
        {}

        Fn(T value)
        {
            class Impl: public details::Result<T>
            {
            public:
                Impl(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
                {
                    details::Result<T>::set(nullptr, std::move(value));
                }
            };

            result = std::make_shared<Impl>(std::move(value));
        }

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

        [[nodiscard]] const T& get() const
        {
            return result->get();
        }

        bool subscribe(IExecutor& executor, IAwaiter& awaiter)
        {
            return result->subscribe(executor, awaiter);
        }
    };

    unittest {
        SimpleExecutor executor;

        Fn fcall = [] { return 42; };
        check(fcall(executor) == 42);
        Fn value = 13;
        check(value(executor) == 13);
    };

    namespace details {
        inline Val<int> test_coro()
        {
            co_return 42;
        }

        unittest {
            SimpleExecutor executor;

            const auto function = test_coro();
            check(function(executor) == 42);
        }

        inline Val<int> test_coro(Val<int> param)
        {
            co_return co_await param + co_await 1;
        }

        unittest {
            SimpleExecutor executor;

            const auto function = test_coro(test_coro());
            check(function(executor) == 43);
        }
    }


    namespace details {
        template <class FTs, class TTs>
        struct IsConvertibleF
        {
            constexpr static inline bool value =
                FTs::size <= TTs::size
                &&
                IsConvertibleF<typetraits::List<FTs, TTs>, std::make_index_sequence<FTs::size>>::value
            ;
        };

        template <class FTs, class TTs, std::size_t ...I>
        struct IsConvertibleF<typetraits::List<FTs, TTs>, std::index_sequence<I...>>
        {
            constexpr static inline bool value = true && (std::is_convertible_v<typetraits::Get<FTs, I>, typetraits::Get<TTs, I>> && ...); // NOLINT
        };

        template <class FTs, class TTs>
        constexpr bool is_convertible = IsConvertibleF<FTs, TTs>::value;
    }


    template <class I, class ...Ts>
    class Fn
    {
    public: // public types
        using ITypes = typetraits::Concat<typetraits::List<I>, typetraits::PopBack<typetraits::List<Ts...>, 2>>;
        using IParam = I;
        using Result = typetraits::Last<typetraits::PopBack<typetraits::List<Ts...>>>;
        using TFImpl = typetraits::Last<typetraits::List<Ts...>>;

        using TArgs = typetraits::PopBack<typetraits::List<I, Ts...>>;
        using PType = typename TFImpl::Params;
        using Function = typename TFImpl::Function;

    private:
        details::FunctionPtr<Function> function;
        PType args;

    public: // object interface
        Fn(details::FunctionPtr<Function>&& function)
            requires (std::tuple_size_v<PType> == 0)
        : function(std::move(function)), args(PType())
        {}

        Fn(details::FunctionPtr<Function>&& function, PType&& args)
            requires (std::tuple_size_v<PType> != 0)
        : function(std::move(function)), args(std::move(args))
        {}

        Fn(const details::FunctionPtr<Function>& function)
            requires (std::tuple_size_v<PType> == 0)
        : function(function), args(PType())
        {}

        Fn(const details::FunctionPtr<Function>& function, const PType& args)
            requires (std::tuple_size_v<PType> != 0)
        : function(function), args(args)
        {}

        Fn(Function&& function)
            requires (std::tuple_size_v<PType> == 0 && !std::is_same_v<details::FunctionPtr<Function>, Function>)
        : function(details::CreateFunctionPtr(std::move(function))), args(PType())
        {}

        Fn(Function&& function, PType&& args)
            requires (std::tuple_size_v<PType> != 0 && !std::is_same_v<details::FunctionPtr<Function>, Function>)
        : function(details::CreateFunctionPtr(std::move(function))), args(std::move(args))
        {}

        Fn(const Function& function)
            requires (std::tuple_size_v<PType> == 0 && !std::is_same_v<details::FunctionPtr<Function>, Function>)
        : function(details::CreateFunctionPtr(function)), args(PType())
        {}

        Fn(const Function& function, const PType& args)
            requires (std::tuple_size_v<PType> != 0 && !std::is_same_v<details::FunctionPtr<Function>, Function>)
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
            &&
            details::is_convertible<
                typetraits::List<typename Wrapper<std::remove_cvref_t<Args>>::Result ...>,
                ITypes
            >
        )
        auto operator () (Args&& ...iargs) const
        {
            using OType = FnType<typetraits::PopFront<TArgs, sizeof...(Args)>, ImplFCallAdd<TFImpl, Wrapper<std::remove_cvref_t<Args>> ...>>;
            return OType(function, std::tuple_cat(args, std::make_tuple(wrap(std::forward<Args>(iargs)) ...)));
        }
    };

    template <class R, class ...TArgs>
    Fn(R(*function)(TArgs...)) -> Fn<TArgs..., R, ImplFCall<R(*)(TArgs...)>>;

    unittest {
        SimpleExecutor executor;

        int (*function)(int, int) = [] (int lhs, int rhs) noexcept {
            return lhs + rhs;
        };
        Fn sum = function;
        check(sum(1, 2)(executor) == 3);
        check(sum(3)(2)(executor) == 5);
    }

    unittest {
        SimpleExecutor executor;

        const auto sum_coro = [](Val<int> lhs, Val<int> rhs) -> Val<int> {
            co_return co_await lhs + co_await rhs;
        };

        Fn<int, int, int, ImplFCall<decltype(sum_coro)>> sum = sum_coro;
        check(sum(0, details::test_coro())(executor) == 42);
        check(sum(1, 2)(executor) == 3);
        check(sum(3)(2)(executor) == 5);
    }

    unittest {
        SimpleExecutor executor;

        int (*inc_fn)(int) = [] (int value) noexcept {
            return value + 1;
        };
        Fn inc = inc_fn;

        const auto dec = [](Val<int> value) -> Val<int> {
            co_return co_await value - 1;
        };
        check(dec(inc(0))(executor) == 0);
    }

    unittest {
        SimpleExecutor executor;

        std::string (*do_concat)(std::string_view lhs, const char* rhs) = [](std::string_view lhs, const char* rhs) -> std::string {
            std::string result(lhs.begin(), lhs.end());
            result.append(rhs);
            return result;
        };
        Fn concat = do_concat;
        check(concat(std::string_view("hello"), "world")(executor) == "helloworld");
        check(concat(std::string_view("hello"))("world")(executor) == "helloworld");
    }
}
