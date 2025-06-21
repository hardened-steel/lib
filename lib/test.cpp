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
        Test::Check check;
        for (const auto& [location, test]: GetTestCaseList()) {
            test(check);
        }
        return true;
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
        }
    }
}
