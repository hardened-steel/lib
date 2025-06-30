#pragma once
#include <lib/test.hpp>
#include <lib/tuple.hpp>
#include <lib/mutex.hpp>
#include <lib/typetraits/tag.hpp>
#include <lib/typetraits/list.hpp>
#include <lib/data-structures/dl-list.hpp>
#include <lib/index.sequence.hpp>

#include <memory>
#include <vector>
#include <optional>
#include <coroutine>
#include <semaphore>


namespace lib::fp {

    class IExecutor
    {
    public:
        class ITask
        {
        public:
            virtual void run(IExecutor* executor) noexcept = 0;
        };

    public:
        virtual void add(ITask* task) noexcept = 0;
        virtual void run() noexcept = 0;
        virtual ITask* get() noexcept = 0;
    };

    class SimpleExecutor: public IExecutor
    {
        std::vector<ITask*> tasks;

        void add(ITask* task) noexcept final
        {
            tasks.push_back(task);
        }

        ITask* get() noexcept final
        {
            if (tasks.empty()) {
                return nullptr;
            }

            auto* task = tasks.back();
            tasks.pop_back();
            return task;
        }

        void run() noexcept final
        {
            while (auto* task = get()) {
                task->run(this);
            }
        }
    };

    class IAwaiter: public data_structures::DLListElement<>
    {
    public:
        virtual void wakeup() noexcept = 0;
    };

    struct I;
    struct O;

    template <class Direction, class T>
    struct TypeToParamsF
    {
        using Result = Direction(T);
    };

    template <class Direction, class ...Ts>
    struct TypeToParamsF<Direction, std::tuple<Ts...>>
    {
        using Result = Direction(Ts...);
    };

    template <class Direction, class T>
    using TypeToParams = typename TypeToParamsF<Direction, T>::Result;


    struct Dynamic;

    template <class ...Ts>
    struct ImplValue;

    template <class T>
    struct TypeToImplValueF
    {
        using Result = ImplValue<T>;
    };

    template <class ...Ts>
    struct TypeToImplValueF<std::tuple<Ts...>>
    {
        using Result = ImplValue<Ts...>;
    };

    template <class T>
    using TypeToImplValue = typename TypeToImplValueF<T>::Result;

    template <class F>
    struct ImplFCall;

    template <class I, class O, class Impl = Dynamic>
    class Fn;

    template <class T>
    struct IsFunctionF
    {
        constexpr static inline bool value = false;
    };

    template <class I, class O, class Impl>
    struct IsFunctionF<Fn<I, O, Impl>>
    {
        constexpr static inline bool value = true;
    };

    template <class T>
    constexpr bool is_function = IsFunctionF<T>::value;


    template <class ...Ts>
    using Val = Fn<I(), O(Ts...), Dynamic>;

    template <class ...Ts>
    class Fn<I(), O(Ts...), ImplValue<Ts...>>
    {
        static_assert(sizeof...(Ts) > 0);

    public: // type interface
        using IType = std::tuple<>;
        using OType = std::tuple<Ts...>;

    private:
        OType value;

    public: // object interface
        template <class ...TArgs>
        requires std::constructible_from<OType, TArgs&&...>
        Fn(TArgs&& ...args) noexcept(std::is_nothrow_constructible_v<OType, TArgs&&...>)
        : value(std::forward<TArgs>(args)...)
        {}

        Fn(OType&& value) noexcept(std::is_nothrow_move_constructible_v<OType>)
        : value(std::move(value))
        {}

        Fn(const OType& value) noexcept(std::is_nothrow_copy_constructible_v<OType>)
        : value(value)
        {}

        Fn() = delete;
        Fn(const Fn& other) = default;
        Fn(Fn&& other) = default;
        Fn& operator=(const Fn& other) = default;
        Fn& operator=(Fn&& other) = default;

    public: // currying interface
        const Fn& operator () (const IType&) const noexcept
        {
            return *this;
        }

        const Fn& operator () () const noexcept
        {
            return *this;
        }

    public: // runtime interface
        const auto& operator()([[maybe_unused]] IExecutor& executor) const noexcept
        {
            if constexpr (sizeof...(Ts) == 1) {
                return std::get<0>(value);
            } else {
                return value;
            }
        }

        [[nodiscard]] bool ready() const noexcept
        {
            return true;
        }

        [[nodiscard]] const OType& get() const noexcept
        {
            return value;
        }

        void subscribe([[maybe_unused]] IExecutor& executor, IAwaiter& awaiter)
        {
            awaiter.wakeup();
        }
    };

    template <class T>
    requires (!std::is_invocable_v<std::remove_cvref_t<T>>)
    Fn(T&& value) -> Fn<I(), TypeToParams<O, std::remove_cvref_t<T>>, TypeToImplValue<std::remove_cvref_t<T>>>;


    namespace details {
        template <class T>
        class Result
        {
            std::optional<T> result;
            std::exception_ptr exception = nullptr;

            mutable data_structures::DLList subscribers;
            mutable Mutex mutex;

        private:
            void emit() const noexcept
            {
                for (auto& subscriber: subscribers.Range<IAwaiter>()) {
                    subscriber.wakeup();
                }
            }

            virtual void add_task(IExecutor& executor) noexcept = 0;

        public:
            template <class U>
            void set(U&& value) noexcept
            {
                std::lock_guard lock(mutex);
                result.emplace(std::forward<U>(value));
                emit();
            }

            void set(typetraits::TTag<std::exception_ptr>, const std::exception_ptr& error) noexcept
            {
                exception = error;
                emit();
            }

            [[nodiscard]] bool ready() const noexcept
            {
                return exception != nullptr || result.has_value();
            }

            [[nodiscard]] const T& get() const noexcept
            {
                if (exception) {
                    rethrow_exception(exception);
                }
                return *result;
            }

            void subscribe(IExecutor& executor, IAwaiter& awaiter)
            {
                std::unique_lock lock(mutex);

                if (ready()) {
                    awaiter.wakeup();
                } else {
                    subscribers.PushBack(awaiter);
                    lock.unlock();
                    add_task(executor);
                }
            }

            const T& execute(IExecutor& executor)
            {
                class Awaiter: public IAwaiter
                {
                    mutable std::binary_semaphore sem {0};

                public:
                    void wakeup() noexcept final
                    {
                        sem.release();
                    }

                    void wait() noexcept
                    {
                        sem.acquire();
                    }
                };

                Awaiter awaiter;
                subscribe(executor, awaiter);
                executor.run();
                awaiter.wait();
                return get();
            }
        };
    }

    template <class ...Ts, class Function>
    class Fn<I(), O(Ts...), ImplFCall<Function>>
    {
        static_assert(sizeof...(Ts) > 0);

    public: // type interface
        using IType = std::tuple<>;
        using OType = std::tuple<Ts...>;

    public:
        class FunctionResult: public details::Result<OType>, private IExecutor::ITask
        {
            Function function;

        public:
            FunctionResult(Function&& function) noexcept(std::is_nothrow_move_constructible_v<Function>)
            : function(std::move(function))
            {}

            FunctionResult(const Function& function) noexcept(std::is_nothrow_copy_constructible_v<Function>)
            : function(function)
            {}

        private:
            void add_task(IExecutor& executor) noexcept final
            {
                executor.add(this);
            }

            void run([[maybe_unused]] IExecutor* executor) noexcept final
            try {
                details::Result<OType>::set(std::make_tuple(function()));
            } catch (...) {
                details::Result<OType>::set(typetraits::tag_t<std::exception_ptr>, std::current_exception());
            }
        };

    private:
        std::shared_ptr<FunctionResult> result;

    public: // object interface
        Fn(Function&& function)
        : result(std::make_shared<FunctionResult>(std::move(function)))
        {}

        Fn(const Function& function)
        : result(std::make_shared<FunctionResult>(function))
        {}

        Fn() = delete;
        Fn(const Fn& other) = default;
        Fn(Fn&& other) = default;
        Fn& operator=(const Fn& other) = default;
        Fn& operator=(Fn&& other) = default;

    public: // currying interface
        const Fn& operator () (const IType&) const noexcept
        {
            return *this;
        }

        const Fn& operator () () const noexcept
        {
            return *this;
        }

    public: // runtime interface
        const auto& operator()(IExecutor& executor) const noexcept
        {
            const auto& value = result->execute(executor);
            if constexpr (sizeof...(Ts) == 1) {
                return std::get<0>(value);
            } else {
                return value;
            }
        }

        [[nodiscard]] bool ready() const noexcept
        {
            return result->ready();
        }

        [[nodiscard]] const OType& get() const noexcept
        {
            return result->get();
        }

        void subscribe(IExecutor& executor, IAwaiter& awaiter)
        {
            result->subscribe(executor, awaiter);
        }
    };

    template <class Function>
    requires std::is_invocable_v<std::remove_cvref_t<Function>>
    Fn(Function&& function) -> Fn<I(), TypeToParams<O, std::invoke_result_t<std::remove_cvref_t<Function>>>, ImplFCall<std::remove_cvref_t<Function>>>;


    namespace details {
        template <class T>
        class Promise;

        template <class ...Ts>
        class Promise<std::tuple<Ts...>>: private IAwaiter, private IExecutor::ITask
        {
        public:
            using Type = std::tuple<Ts...>;
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

            void wakeup() noexcept final
            {
                executor->add(this);
            }

        public:
            [[nodiscard]] Val<Ts...> get_return_object() const noexcept
            {
                return Val<Ts...>(typetraits::tag_t<CoroResult>, result);
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
                result->set(typetraits::tag_t<std::exception_ptr>, std::current_exception());
            }

            template <class U>
            void return_value(U&& value)
            {
                result->set(std::forward<U>(value));
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

            template <class ...OParams, class Impl>
            [[nodiscard]] auto await_transform(Fn<I(), O(OParams...), Impl> function) noexcept(std::is_nothrow_move_constructible_v<Fn<I(), O(OParams...), Impl>>)
            {
                class Awaiter
                {
                    using Function = Fn<I(), O(OParams...), Impl>;
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

                    constexpr void await_suspend(std::coroutine_handle<Promise> handle) noexcept
                    {
                        auto& promise = handle.promise();
                        auto* executor = promise.executor;
                        function.subscribe(*executor, promise);
                    }

                    [[nodiscard]] decltype(auto) await_resume() const noexcept
                    {
                        if constexpr (sizeof...(OParams) == 1) {
                            return std::get<0>(function.get());
                        } else {
                            return function.get();
                        }
                    }
                };

                return Awaiter(std::move(function));
            }
        };
    }

    template <class ...Ts>
    class Fn<I(), O(Ts...), Dynamic>
    {
        static_assert(sizeof...(Ts) > 0);
        friend details::Promise<std::tuple<Ts...>>;

    public: // type interface
        using IType = std::tuple<>;
        using OType = std::tuple<Ts...>;

    private:
        std::shared_ptr<details::Result<OType>> result;

    public:
        using promise_type = details::Promise<OType>;        

    private:
        explicit Fn(typetraits::TTag<typename details::Promise<OType>::CoroResult>, std::shared_ptr<details::Result<OType>> result) noexcept
        : result(std::move(result))
        {}

    public: // object interface
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (!std::is_same_v<std::remove_cvref_t<Function>, Fn> && std::is_invocable_r_v<OType, std::remove_cvref_t<Function>>)
        Fn(Function&& function)
        : result(std::make_shared<typename Fn<I(), O(Ts...), ImplFCall<std::remove_cvref_t<Function>>>::FunctionResult>(std::forward<Function>(function)))
        {}

        Fn(OType value)
        {
            class Impl: public details::Result<OType>
            {
            public:
                Impl(OType value) noexcept(std::is_nothrow_move_constructible_v<OType>)
                {
                    set(std::move(value));
                }

                void add_task(IExecutor& executor) noexcept final
                {
                }
            };

            result = std::make_shared<Impl>(std::move(value));
        }

    public: // currying interface
        const Fn& operator () (const IType&) const noexcept
        {
            return *this;
        }

        const Fn& operator () () const noexcept
        {
            return *this;
        }

    public: // runtime interface
        const auto& operator()(IExecutor& executor) const noexcept
        {
            const auto& value = result->execute(executor);
            if constexpr (sizeof...(Ts) == 1) {
                return std::get<0>(value);
            } else {
                return value;
            }
        }

        [[nodiscard]] bool ready() const noexcept
        {
            return result->ready();
        }

        [[nodiscard]] const OType& get() const noexcept
        {
            return result->get();
        }
    
        void subscribe(IExecutor& executor, IAwaiter& awaiter)
        {
            result->subscribe(executor, awaiter);
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

    template <class T>
    struct FnWrapper
    {
        using Type = Fn<I(), O(T), ImplValue<T>>;
    };

    template <class ...IArgs, class ...OArgs, class Impl>
    struct FnWrapper<Fn<I(IArgs...), O(OArgs...), Impl>>
    {
        using Type = Fn<I(IArgs...), O(OArgs...), Impl>;
    };

    template <class T>
    using Wrapper = typename FnWrapper<std::remove_cvref_t<T>>::Type;

    template <class T, class Fn = Wrapper<T>>
    Fn wrap(T&& value)
    {
        return Fn(std::forward<T>(value));
    }

    namespace details {
        template <std::size_t Offset, class List>
        struct AddOffsetF;

        template <std::size_t Offset>
        struct AddOffsetF<Offset, typetraits::List<>>
        {
            using Result = typetraits::List<>;
        };

        template <std::size_t Offset, class Head, class ...Tail>
        struct AddOffsetF<Offset, typetraits::List<Head, Tail...>>
        {
            using Result = typetraits::Concat<
                typetraits::List<indexes::Add<Offset, Head>>,
                typename AddOffsetF<Offset, typetraits::List<Tail...>>::Result
            >;
        };

        template <std::size_t Offset, class List>
        using AddOffset = typename AddOffsetF<Offset, List>::Result;

        template <class TArgs, class ...IArgs>
        struct CarryingF;

        template <class TArgs>
        struct CarryingF<TArgs>
        {
            using IArgs = typetraits::List<>;
            using CIndx = typetraits::List<>;
            using OArgs = TArgs;
        };

        template<class Head, class ...Tail, class ...FIArgs, class ...FOArgs, class Impl, class ...Params>
        struct CarryingF<typetraits::List<Head, Tail...>, Fn<I(FIArgs...), O(FOArgs...), Impl>, Params...>
        {
            using IArgs = typetraits::Concat<typetraits::List<FIArgs...>, typename CarryingF<typetraits::List<Tail...>, Params...>::IArgs>;
            using CIndx = typetraits::Concat<
                typetraits::List<std::index_sequence_for<FIArgs...>>,
                AddOffset<sizeof...(FIArgs), typename CarryingF<typetraits::List<Tail...>, Params...>::CIndx>
            >;
            using OArgs = typename CarryingF<typetraits::List<Tail...>, Params...>::OArgs;
        };

        template<class ...Tail, class ...FIArgs, class ...FOArgs, class LImpl, class RImpl, class ...Params>
        struct CarryingF<typetraits::List<Fn<I(FIArgs...), O(FOArgs...), LImpl>, Tail...>, Fn<I(FIArgs...), O(FOArgs...), RImpl>, Params...>
        {
            using IArgs = typename CarryingF<typetraits::List<Tail...>, Params...>::IArgs;
            using CIndx = typetraits::Concat<
                typetraits::List<std::index_sequence_for<>>,
                typename CarryingF<typetraits::List<Tail...>, Params...>::CIndx
            >;
            using OArgs = typename CarryingF<typetraits::List<Tail...>, Params...>::OArgs;
        };

        template <class TArgs, class ...IArgs>
        using CarryingIArgs = typename CarryingF<TArgs, IArgs...>::IArgs;

        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<int, float, double>, Fn<I(int, float), O(int)>>, typetraits::List<int, float>>);
        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<int, float, double>, Fn<I(int, float), O(int)>, Fn<I(double), O(float)>>, typetraits::List<int, float, double>>);
        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<int, float, int, double>, Fn<I(int, float), O(int)>, Fn<I(), O(float)>, Fn<I(double), O(int)>>, typetraits::List<int, float, double>>);
        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<int, int, float, int, double>, Fn<I(), O(int)>, Fn<I(int, float), O(int)>, Fn<I(), O(float)>, Fn<I(double), O(double)>>, typetraits::List<int, float, double>>);
        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<Fn<I(int), O(double)>, float>, Fn<I(int), O(double)>>, typetraits::List<>>);


        template <class TArgs, class ...IArgs>
        using CarryingCIndx = typename CarryingF<TArgs, IArgs...>::CIndx;

        static_assert(std::is_same_v<CarryingCIndx<typetraits::List<int, float, double>, Fn<I(int, float), O(int)>>, typetraits::List<std::index_sequence<0, 1>>>);
        static_assert(std::is_same_v<CarryingCIndx<typetraits::List<int, float, double>, Fn<I(int, float), O(int)>, Fn<I(double), O(float)>>, typetraits::List<std::index_sequence<0, 1>, std::index_sequence<2>>>);
        static_assert(std::is_same_v<CarryingCIndx<typetraits::List<int, float, int, double>, Fn<I(int, float), O(int)>, Fn<I(), O(float)>, Fn<I(double), O()>>, typetraits::List<std::index_sequence<0, 1>, std::index_sequence<>, std::index_sequence<2>>>);

        template <class TArgs, class ...IArgs>
        using CarryingOArgs = typename CarryingF<TArgs, IArgs...>::OArgs;

        static_assert(std::is_same_v<CarryingOArgs<typetraits::List<int, float, double>, Fn<I(int, float), O(int)>>, typetraits::List<float, double>>);
        static_assert(std::is_same_v<CarryingOArgs<typetraits::List<int, float, double>, Fn<I(int, float), O(int)>, Fn<I(double), O(float)>>, typetraits::List<double>>);
        static_assert(std::is_same_v<CarryingOArgs<typetraits::List<int, float, int, double>, Fn<I(int, float), O(int)>, Fn<I(), O(int)>, Fn<I(double), O(int)>, Fn<I(), O(double)>>, typetraits::List<>>);
        static_assert(std::is_same_v<CarryingOArgs<typetraits::List<Fn<I(int), O(double)>, Fn<I(int), O(float)>>, Fn<I(int), O(double)>>, typetraits::List<Fn<I(int), O(float)>>>);


        template <class ...IArgs, class ...OArgs, class Impl, class Tuple, std::size_t ...Index>
        decltype(auto) apply(const Fn<I(IArgs...), O(OArgs...), Impl>& function, const Tuple& params, std::index_sequence<Index...>)
        {
            return function(std::get<Index>(params)...);
        }
    }

    template <class ...IParams, class ...OParams>
    class Fn<I(IParams...), O(OParams...), Dynamic>
    {
        static_assert(sizeof...(IParams) + sizeof...(OParams) > 0);

    public: // type interface
        using IType = std::tuple<IParams...>;
        using OType = std::tuple<OParams...>;

    private:
        class IFunction
        {
        public:
            [[nodiscard]] virtual Val<OParams...> run(Wrapper<IParams>...) const = 0;
        };

        std::shared_ptr<IFunction> ifunction;

    public: // object interface
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (
            !std::is_same_v<std::remove_cvref_t<Function>, Fn>
            &&
            (
                std::is_invocable_r_v<OType, std::remove_cvref_t<Function>, IParams...>
                ||
                std::is_invocable_r_v<Val<OParams...>, std::remove_cvref_t<Function>, IParams...>
            )
        )
        Fn(Function&& function)
        {
            class Impl: public IFunction
            {
                Function function;

            public:
                Impl(Function&& function)
                : function(std::forward<Function>(function))
                {}

                [[nodiscard]] Val<OParams...> run(Wrapper<IParams>... args) const final
                {
                    co_return co_await function(co_await args...);
                }
            };

            ifunction = std::make_shared<Impl>(std::forward<Function>(function));
        }

    private:
        template <class ...IArgs, class ...OArgs, class ...CIndx, class ...Args>
        [[nodiscard]] auto currying(typetraits::TTag<typetraits::List<IArgs...>>, typetraits::TTag<typetraits::List<OArgs...>>, typetraits::TTag<typetraits::List<CIndx...>>, Args ...cargs) const
        {
            auto function = [cargs..., ifunction = this->ifunction](IArgs ...iargs, OArgs ...oargs) -> Val<OParams...> {
                const auto iargs_list = std::tie(iargs...);
                auto result = co_await ifunction->run(details::apply(cargs, iargs_list, CIndx{})..., oargs...);
                co_return result;
            };
            return Fn<I(IArgs..., OArgs...), O(OParams...)>(std::move(function));
        }

        template <class ...Args>
        [[nodiscard]] auto currying(Args ...args) const
        {
            using IArgs = details::CarryingIArgs<typetraits::List<IParams...>, Args...>;
            using OArgs = details::CarryingOArgs<typetraits::List<IParams...>, Args...>;
            using CIndx = details::CarryingCIndx<typetraits::List<IParams...>, Args...>;

            if constexpr (std::is_same_v<typetraits::Concat<IArgs, OArgs>, typetraits::List<>>) {
                return ifunction->run(args...);
            } else {
                return currying(typetraits::tag_t<IArgs>, typetraits::tag_t<OArgs>, typetraits::tag_t<CIndx>, args...);
            }
        }

    public: // currying interface
        template <class ...IArgs>
        auto operator()(IArgs ...args) const
        {
            static_assert(sizeof...(IArgs) <= sizeof...(IParams));
            return currying(wrap(std::forward<IArgs>(args))...);
        }

        template <class ...IArgs>
        auto operator()(const std::tuple<IArgs...>& args) const
        {
            return std::apply(*this, args);
        }

        template <class ...IArgs, class Impl>
        auto operator()(const Fn<I(IArgs...), O(IParams...), Impl>& other) const
        {
            if constexpr (sizeof...(IArgs)) {
                auto function = [other, ifunction = this->ifunction](IArgs ...args) -> Val<OParams...> {
                    auto result = co_await ifunction->run(co_await other(args...));
                    co_return result;
                };
                return Fn<I(IArgs...), O(OParams...)>(std::move(function));
            } else {
                return ifunction->run(other);
            }
        }

        const Fn& operator()() const noexcept
        {
            return *this;
        }
    };

    template <class R, class ...TArgs>
    Fn(R(* function)(TArgs...)) -> Fn<I(TArgs...), TypeToParams<O, R>>;

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

        const auto sum_coro = [](int lhs, int rhs) -> Val<int> {
            co_return lhs + rhs;
        };

        Fn<I(int, int), O(int)> sum = sum_coro;
        check(sum(1, 2)(executor) == 3);
        check(sum(3)(2)(executor) == 5);
    }

    unittest {
        SimpleExecutor executor;

        Fn<I(std::string_view, const char*), O(std::string)> concat = [](std::string_view lhs, const char* rhs) -> std::string {
            std::string result(lhs.begin(), lhs.end());
            result.append(rhs);
            return result;
        };
        check(concat(std::string_view("hello"), "world")(executor) == "helloworld");
        check(concat(std::string_view("hello"))("world")(executor) == "helloworld");
    }
}
