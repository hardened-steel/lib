#include <lib/test.hpp>

#include <iostream>
#include <string>
#include <map>


namespace lib {


    auto& GetTestCaseList()
    {
        static std::map<typetraits::Location, Test::CaseFunction> test_case_list;
        return test_case_list;
    }

    void Test::Register(typetraits::Location location, CaseFunction function)
    {
        GetTestCaseList().emplace(location, function);
    }

    bool Test::Run()
    {
        std::size_t f_tests = 0;
        std::size_t s_tests = 0;

        Test::Check check;
        for (const auto& [location, test]: GetTestCaseList()) {
            check.reset();
            try {
                test(check);
                if (check.test_failed()) {
                    f_tests++;
                } else {
                    s_tests++;
                }
            } catch (...) {
                f_tests++;
            }
        }
        std::cout << "report:\n";
        std::cout << "    " << "total tests: " << s_tests + f_tests << "\n";
        std::cout << "    " << "successfully tests: " << s_tests << "\n";
        std::cout << "    " << "failed tests: " << f_tests << "\n";
        std::cout << "    " << "total checks: " << check.f_count + check.s_count << "\n";
        std::cout << "    " << "failed checks: " << check.f_count << "\n";

        return check.f_count == 0;
    }

    void Test::Check::operator () (bool conditional, std::source_location slocation)
    {
        if (!conditional) {
            std::string message = "conditional is false at: "
                                + std::string(slocation.file_name())
                                + ":"
                                + std::to_string(slocation.line())
            ;
            std::cerr << message << std::endl;

            failed = true;
            f_count++;
        } else {
            s_count++;
        }
    }
}
