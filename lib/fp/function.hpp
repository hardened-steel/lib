#pragma once
#include <lib/test.hpp>
#include <lib/event.hpp>
#include <lib/index.sequence.hpp>

#include <memory>
#include <optional>
#include <coroutine>


namespace lib::fp {

    template <class T>
    class Fn
    {
        class IResult
        {
        public:
            //[[nodiscard]] virtual IEvent& event() const noexcept = 0;
            [[nodiscard]] virtual bool ready() const noexcept = 0;
            [[nodiscard]] virtual T get() const = 0;
            virtual void run() = 0;
        };

        std::shared_ptr<IResult> result;

    public:
        Fn(const Fn& other) noexcept = default;
        Fn(Fn&& other) noexcept = default;
        Fn& operator=(const Fn& other) noexcept = default;
        Fn& operator=(Fn&& other) noexcept = default;

        template <class Function>
        requires (!std::is_same_v<std::remove_cvref_t<Function>, Fn>)
        Fn(Function&& function)
        {
            class Impl: public IResult
            {
                Function function;
                std::optional<T> value;

            public:
                Impl(Function&& function)
                : function(std::forward<Function>(function))
                {}

                [[nodiscard]] bool ready() const noexcept final
                {
                    return value.has_value();
                }

                [[nodiscard]] T get() const final
                {
                    return *value;
                }

                void run() final
                {
                    value.emplace(function());
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

                [[nodiscard]] T get() const final
                {
                    return value;
                }

                void run() final
                {
                }
            };

            result = std::make_shared<Impl>(std::move(value));
        }

        T operator()() const
        {
            if (!result->ready()) {
                result->run();
            }
            return result->get();
        }
    };

    template <class R>
    class Fn<R()>: public Fn<R>
    {
    public:
        using Fn<R>::Fn;
    };

    unittest {
        Fn<int> function = [] { return 42; };
        check(function() == 42);
        function = 13;
        check(function() == 13);
    };

    template <class T>
    T unwrap(T value)
    {
        return value;
    }

    template <class T>
    T unwrap(Fn<T> value)
    {
        return value();
    }

    unittest {
        const lib::fp::Fn<int> function = 42;
        check(unwrap(function) == 42);
        check(unwrap(42) == 42);
    }

    template <class R, class ...TArgs>
    class Fn<R(TArgs...)>
    {
        class IFunction
        {
        public:
            [[nodiscard]] virtual R run(TArgs...) const = 0;
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

                [[nodiscard]] R run(TArgs... args) const final
                {
                    return function(unwrap(args)...);
                }
            };

            ifunction = std::make_shared<Impl>(std::forward<Function>(function));
        }

    private:
        template <std::size_t ...I, class ...IArgs>
        [[nodiscard]] auto currying(std::index_sequence<I...>, IArgs ...cargs) const
        {
            Fn<R(std::tuple_element_t<I, std::tuple<TArgs...>>...)> function = [=, ifunction = this->ifunction]
            (std::tuple_element_t<I, std::tuple<TArgs...>>... iargs) {
                return ifunction->run(cargs..., iargs...);
            };
            return function;
        }

    public:
        template <class ...IArgs>
        auto operator()(IArgs ...args) const
        {
            constexpr auto fparams = sizeof...(TArgs);
            constexpr auto iparams = sizeof...(IArgs);
            static_assert(iparams <= fparams);

            if constexpr (iparams == fparams) {
                return currying(std::index_sequence<>{}, args...);
            } else {
                return currying(indexes::range<fparams - 1U, fparams - iparams>(), args...);
            }
        }
    };

    unittest {
        Fn<int(int, int)> sum = [] (int lhs, int rhs) noexcept {
            return lhs + rhs;
        };
        check(sum(1, 2)() == 3);
        check(sum(3)(2)() == 5);
    }

    unittest {
        Fn<std::string(std::string_view, const char*)> concat = [](std::string_view lhs, const char* rhs) -> std::string {
            std::string result(lhs.begin(), lhs.end());
            result.append(rhs);
            return result;
        };
        check(concat(std::string_view("hello"), "world")() == "helloworld");
        check(concat(std::string_view("hello"))("world")() == "helloworld");
    }

}
