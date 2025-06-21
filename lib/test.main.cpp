#include <lib/test.hpp>

#include <cstdlib>


int main()
{
    if (lib::Test::Run()) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
