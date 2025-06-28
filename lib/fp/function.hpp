#pragma once
#include <lib/test.hpp>
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

    class IWorker
    {
    public:
        class ITask
        {
        public:
            virtual void run(IWorker* worker) noexcept = 0;
        };

    public:
        virtual void add(ITask* task) noexcept = 0;
        virtual ITask* get() noexcept = 0;
    };

    class SimpleWorker: public IWorker
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
    };

    class IAwaiter: public data_structures::DLListElement<>
    {
    public:
        virtual void wakeup() noexcept = 0;
    };

    template <class T>
    class Fn;

    template <class T>
    class Fn<T()>
    {
        class IResult
        {
        public:
            [[nodiscard]] virtual bool ready() const noexcept = 0;
            [[nodiscard]] virtual const T& get() const noexcept = 0;
            virtual void subscribe(IWorker& worker, IAwaiter& awaiter) = 0;
        };

        std::shared_ptr<IResult> result;

    private:
        explicit Fn(typetraits::TTag<IResult>, std::shared_ptr<IResult> result) noexcept
        : result(std::move(result))
        {}

    public:
        class Promise: public IWorker::ITask, public IAwaiter
        {
            using Handle = std::coroutine_handle<Promise>;

            class Result: public IResult
            {
                Promise* promise;
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

            public:
                Result(Promise* promise) noexcept
                : promise(promise)
                {}

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

                [[nodiscard]] bool ready() const noexcept final
                {
                    return exception != nullptr || result.has_value();
                }

                [[nodiscard]] const T& get() const noexcept final
                {
                    if (exception) {
                        rethrow_exception(exception);
                    }
                    return *result;
                }

                void subscribe(IWorker& worker, IAwaiter& awaiter) final
                {
                    std::unique_lock lock(mutex);

                    if (ready()) {
                        awaiter.wakeup();
                    } else {
                        subscribers.PushBack(awaiter);
                        lock.unlock();
                        worker.add(promise);
                    }
                }
            };

            std::shared_ptr<Result> result = std::make_shared<Result>(this);
            mutable IWorker* worker = nullptr;

        private:
            void run(IWorker* worker) noexcept final
            {
                this->worker = worker;
                Handle::from_promise(*this).resume();
            }

            void wakeup() noexcept final
            {
                worker->add(this);
            }

        public:
            [[nodiscard]] Fn<T> get_return_object() const noexcept
            {
                return Fn<T>(typetraits::tag_t<IResult>, result);
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
            void return_value(Fn<U()> value)
            {
                result->set(value());
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

            template <class U>
            [[nodiscard]] auto await_transform(Fn<U> function) noexcept
            {
                class Awaiter
                {
                    Fn<U> function;

                public:
                    Awaiter( Fn<U> function) noexcept
                    : function(std::move(function))
                    {
                    }

                public:
                    [[nodiscard]] bool await_ready() const noexcept
                    {
                        return function.ready();
                    }

                    constexpr void await_suspend(std::coroutine_handle<Promise> handle) noexcept
                    {
                        auto& promise = handle.promise();
                        auto* worker = promise.worker;
                        function.subscribe(*worker, promise);
                    }

                    [[nodiscard]] decltype(auto) await_resume() const noexcept
                    {
                        return function.get();
                    }
                };

                return Awaiter(std::move(function));
            }
        };

        using promise_type = Promise;

    public:
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (!(std::is_same_v<std::remove_cvref_t<Function>, Fn<T()>> || std::is_same_v<std::remove_cvref_t<Function>, Fn<T>>))
        Fn(Function&& function)
        {
            class Impl: public IResult, public IWorker::ITask
            {
                Function function;
                std::optional<T> value;

                mutable data_structures::DLList subscribers;
                mutable Mutex mutex;

            private:
                void emit() const noexcept
                {
                    for (auto& subscriber: subscribers.Range<IAwaiter>()) {
                        subscriber.wakeup();
                    }
                }

            public:
                Impl(Function&& function)
                : function(std::forward<Function>(function))
                {}

                [[nodiscard]] bool ready() const noexcept final
                {
                    return value.has_value();
                }

                [[nodiscard]] const T& get() const noexcept final
                {
                    return *value;
                }

                void run(IWorker*) noexcept final
                {
                    std::lock_guard lock(mutex);
                    value = function();
                    emit();
                }

                void subscribe(IWorker& worker, IAwaiter& awaiter) final
                {
                    std::unique_lock lock(mutex);

                    if (ready()) {
                        lock.unlock();
                        awaiter.wakeup();
                    } else {
                        subscribers.PushBack(awaiter);
                        lock.unlock();
                        worker.add(this);
                    }
                }
            };

            result = std::make_shared<Impl>(std::forward<Function>(function));
        }

        Fn(T value)
        {
            class Impl: public IResult
            {
                T value;

            public:
                Impl(T value)
                : value(std::move(value))
                {}

                [[nodiscard]] bool ready() const noexcept final
                {
                    return true;
                }

                [[nodiscard]] const T& get() const noexcept final
                {
                    return value;
                }

                void subscribe(IWorker&, IAwaiter& awaiter) final
                {
                    awaiter.wakeup();
                }
            };

            result = std::make_shared<Impl>(std::move(value));
        }

        const T& operator()(IWorker& worker) const
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
            result->subscribe(worker, awaiter);
            while (auto* task = worker.get()) {
                task->run(&worker);
            }
            awaiter.wait();
            return result->get();
        }

        Fn<T()> operator()() const noexcept
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
    
        void subscribe(IWorker& worker, IAwaiter& awaiter)
        {
            result->subscribe(worker, awaiter);
        }
    };

    template <class T>
    class Fn: public Fn<T()>
    {
    public:
        using Fn<T()>::Fn;
    };

    template <class T>
    Fn(T value) ->Fn<T()>;

    unittest {
        SimpleWorker worker;

        Fn<int> function = [] { return 42; };
        check(function(worker) == 42);
        function = 13;
        check(function(worker) == 13);
    };

    namespace details {
        inline Fn<int> test_coro()
        {
            co_return 42;
        }

        unittest {
            SimpleWorker worker;

            const auto function = test_coro();
            check(function(worker) == 42);
        }

        inline Fn<int> test_coro(Fn<int> param)
        {
            co_return co_await param + co_await 1;
        }

        unittest {
            SimpleWorker worker;

            const auto function = test_coro(test_coro());
            check(function(worker) == 43);
        }
    }

    template <class T>
    struct FnWrapper
    {
        using Type = Fn<T()>;
    };

    template <class R, class ...TArgs>
    struct FnWrapper<Fn<R(TArgs...)>>
    {
        using Type = Fn<R(TArgs...)>;
    };

    template <class T>
    using Wrapper = typename FnWrapper<T>::Type;

    template <class T>
    Fn<T()> wrap(T value)
    {
        return value;
    }

    template <class R, class ...TArgs>
    Fn<R(TArgs...)> wrap(Fn<R(TArgs...)> fn) noexcept
    {
        return fn;
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

        template<class Head, class ...Tail, class R, class ...TArgs, class ...Params>
        struct CarryingF<typetraits::List<Head, Tail...>, Fn<R(TArgs...)>, Params...>
        {
            using IArgs = typetraits::Concat<typetraits::List<TArgs...>, typename CarryingF<typetraits::List<Tail...>, Params...>::IArgs>;
            using CIndx = typetraits::Concat<
                typetraits::List<std::index_sequence_for<TArgs...>>,
                AddOffset<sizeof...(TArgs), typename CarryingF<typetraits::List<Tail...>, Params...>::CIndx>
            >;
            using OArgs = typename CarryingF<typetraits::List<Tail...>, Params...>::OArgs;
        };

        template<class ...Tail, class R, class ...TArgs, class ...Params>
        struct CarryingF<typetraits::List<Fn<R(TArgs...)>, Tail...>, Fn<R(TArgs...)>, Params...>
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

        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<int, float, double>, Fn<int(int, float)>>, typetraits::List<int, float>>);
        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<int, float, double>, Fn<int(int, float)>, Fn<void(double)>>, typetraits::List<int, float, double>>);
        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<int, float, int, double>, Fn<int(int, float)>, Fn<int()>, Fn<void(double)>>, typetraits::List<int, float, double>>);
        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<int, int, float, int, double>, Fn<int()>, Fn<int(int, float)>, Fn<int()>, Fn<void(double)>>, typetraits::List<int, float, double>>);
        static_assert(std::is_same_v<CarryingIArgs<typetraits::List<Fn<double(int)>, float>, Fn<double(int)>>, typetraits::List<>>);


        template <class TArgs, class ...IArgs>
        using CarryingCIndx = typename CarryingF<TArgs, IArgs...>::CIndx;

        static_assert(std::is_same_v<CarryingCIndx<typetraits::List<int, float, double>, Fn<int(int, float)>>, typetraits::List<std::index_sequence<0, 1>>>);
        static_assert(std::is_same_v<CarryingCIndx<typetraits::List<int, float, double>, Fn<int(int, float)>, Fn<void(double)>>, typetraits::List<std::index_sequence<0, 1>, std::index_sequence<2>>>);
        static_assert(std::is_same_v<CarryingCIndx<typetraits::List<int, float, int, double>, Fn<int(int, float)>, Fn<int()>, Fn<void(double)>>, typetraits::List<std::index_sequence<0, 1>, std::index_sequence<>, std::index_sequence<2>>>);

        template <class TArgs, class ...IArgs>
        using CarryingOArgs = typename CarryingF<TArgs, IArgs...>::OArgs;

        static_assert(std::is_same_v<CarryingOArgs<typetraits::List<int, float, double>, Fn<int(int, float)>>, typetraits::List<float, double>>);
        static_assert(std::is_same_v<CarryingOArgs<typetraits::List<int, float, double>, Fn<int(int, float)>, Fn<float(double)>>, typetraits::List<double>>);
        static_assert(std::is_same_v<CarryingOArgs<typetraits::List<int, float, int, double>, Fn<int(int, float)>, Fn<int()>, Fn<void(double)>, Fn<double()>>, typetraits::List<>>);
        static_assert(std::is_same_v<CarryingOArgs<typetraits::List<Fn<double(int)>, Fn<float(int)>>, Fn<double(int)>>, typetraits::List<Fn<float(int)>>>);


        template <class R, class ...TArgs, class Tuple, std::size_t ...I>
        decltype(auto) apply(const Fn<R(TArgs...)>& function, const Tuple& params, std::index_sequence<I...>)
        {
            return function(std::get<I>(params)...);
        }
    }

    template <class R, class ...TArgs>
    class Fn<R(TArgs...)>
    {
        class IFunction
        {
        public:
            [[nodiscard]] virtual Fn<R()> run(Wrapper<TArgs>...) const = 0;
        };

        std::shared_ptr<IFunction> ifunction;

    public:
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (!std::is_same_v<std::remove_cvref_t<Function>, Fn>)
        Fn(Function&& function)
        {
            class Impl: public IFunction
            {
                Function function;

            public:
                Impl(Function&& function)
                : function(std::forward<Function>(function))
                {}

                [[nodiscard]] Fn<R()> run(Wrapper<TArgs>... args) const final
                {
                    co_return co_await function(co_await args...);
                }
            };

            auto nfunction = std::make_shared<Impl>(std::forward<Function>(function));
            ifunction = nfunction;
        }

    private:
        template <class ...IArgs, class ...OArgs, class ...CIndx, class ...Args>
        [[nodiscard]] auto currying(typetraits::TTag<typetraits::List<IArgs...>>, typetraits::TTag<typetraits::List<OArgs...>>, typetraits::TTag<typetraits::List<CIndx...>>, Args ...cargs) const
        {
            auto function = [cargs..., ifunction = this->ifunction](IArgs ...iargs, OArgs ...oargs) -> Fn<R()> {
                const auto iargs_list = std::tie(iargs...);
                auto result = co_await ifunction->run(details::apply(cargs, iargs_list, CIndx{})..., oargs...);
                co_return result;
            };
            return Fn<R(IArgs..., OArgs...)>(std::move(function));
        }

        template <class ...Args>
        [[nodiscard]] auto currying(Args ...args) const
        {
            using IArgs = details::CarryingIArgs<typetraits::List<TArgs...>, Args...>;
            using OArgs = details::CarryingOArgs<typetraits::List<TArgs...>, Args...>;
            using CIndx = details::CarryingCIndx<typetraits::List<TArgs...>, Args...>;

            if constexpr (std::is_same_v<typetraits::Concat<IArgs, OArgs>, typetraits::List<>>) {
                return ifunction->run(args...);
            } else {
                return currying(typetraits::tag_t<IArgs>, typetraits::tag_t<OArgs>, typetraits::tag_t<CIndx>, args...);
            }
        }

    public:
        template <class ...IArgs>
        auto operator()(IArgs ...args) const
        {
            static_assert(sizeof...(IArgs) <= sizeof...(TArgs));
            return currying(wrap(args)...);
        }

        Fn<R(TArgs...)> operator()() const noexcept
        {
            return *this;
        }
    };

    unittest {
        SimpleWorker worker;

        Fn<int(int, int)> sum = [] (int lhs, int rhs) noexcept {
            return lhs + rhs;
        };
        check(sum(1, 2)(worker) == 3);
        check(sum(3)(2)(worker) == 5);
    }

    unittest {
        SimpleWorker worker;

        const auto sum_coro = [](Fn<int> lhs, Fn<int> rhs) -> Fn<int> {
            const auto l = co_await lhs;
            const auto r = co_await rhs;
            co_return l + r;
        };

        Fn<int(int, int)> sum = sum_coro;
        check(sum(1, 2)(worker) == 3);
        check(sum(3)(2)(worker) == 5);
    }

    unittest {
        SimpleWorker worker;

        Fn<std::string(std::string_view, const char*)> concat = [](std::string_view lhs, const char* rhs) -> std::string {
            std::string result(lhs.begin(), lhs.end());
            result.append(rhs);
            return result;
        };
        check(concat(std::string_view("hello"), "world")(worker) == "helloworld");
        check(concat(std::string_view("hello"))("world")(worker) == "helloworld");
    }
}
