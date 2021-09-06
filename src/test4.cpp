#include "string_view.hpp"
#include <iostream>

int main()
{
    using namespace std::literals;

    std::string_view s1 = "test";

    auto s2 = "test"sv;

    std::cout << s2 << '\n';
    std::cout << std::hash<std::string_view>{}(s2) << '\n';
}

