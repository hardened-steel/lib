#pragma once
#include <lib/platform.hpp>
#include <lib/typetraits/var.hpp>
#include <lib/array.hpp>
#include <source_location>


namespace lib {

    class Test
    {
    public:
        class Check;
        using CaseFunction = void (*)(Check& check);
        using Case = std::pair<typetraits::Location, CaseFunction>;

        static bool Run();
        static void Register(typetraits::Location location, CaseFunction function);

        struct CaseRegister
        {
            template <std::size_t N>
            CaseRegister(const std::array<Case, N>& cases)
            {
                for (const auto& [location, test]: cases) {
                    Register(location, test);
                }
            }
        };
    };

    class Test::Check
    {
    public:
        void operator () (bool conditional, std::source_location slocation = std::source_location::current());
    };

    constexpr static inline Test::Case empty_test_case {typetraits::next(), [](Test::Check&){}};
    constexpr auto tests_init = typetraits::set<"lib-test-cases", std::array<Test::Case, 1>{empty_test_case}, void>;

    template <class Tag, typetraits::Location Loc>
    struct TestTag
    {
        static void test(Test::Check& check)
        {
            const Tag* tag = nullptr;
            test_case(TestTag{}, tag, check);
        }
        constexpr static inline auto& old_tests = typetraits::get<"lib-test-cases", TestTag>;
        constexpr static inline auto& new_tests = typetraits::set<
            "lib-test-cases",
            concat(
                old_tests,
                std::array<Test::Case, 1> {Test::Case(Loc, &test)}
            ),
            TestTag
        >;
    };
}

#define LIB_TEST_CASE_NAME_IMPL(NAME, NUMBER) NAME##NUMBER
#define LIB_TEST_CASE_NAME(NAME, NUMBER) LIB_TEST_CASE_NAME_IMPL(NAME, NUMBER)

#define LIB_GENERATE_TEST_CASE(INDEX) inline void test_case(                                     \
    ::lib::TestTag<struct LIB_TEST_CASE_NAME(test_case_tag_, INDEX), ::lib::typetraits::next()>, \
    const struct LIB_TEST_CASE_NAME(test_case_tag_, INDEX)*,                                     \
    [[maybe_unused]] ::lib::Test::Check& check                                                   \
)

#define unittest LIB_GENERATE_TEST_CASE(__COUNTER__) // NOLINT
