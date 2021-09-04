#include "expected.hpp"
#include <string>
#include <iostream>

int main()
{
    std::expected<int, std::string> e;

    std::cout << "value or (should be 0): " << e.value_or(3.14) << '\n';

    e = std::not_expected<std::string>("hello");
    std::cout << "value or (should be 3): " << e.value_or(3.14) << '\n';

}

