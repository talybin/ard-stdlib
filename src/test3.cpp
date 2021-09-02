#include "variant2.hpp"

#include <iostream>
#include <vector>
#include <functional>
#include <cassert>

int main()
{
    using Ref = std::reference_wrapper<std::vector<int>>;
    using test_variant = std::variant<int, Ref, std::string>;

    test_variant v1;
    test_variant v2 = 42;

    int i1 = 0;

    test_variant v3 = i1;

    test_variant v4 = "test";
    assert(v4.index() == 2);

    std::vector<int> vec { 42 };
    test_variant v5 = vec;
    assert(v5.index() == 1);
    // TODO try to get or visit and use [0]

//    v5 = std::vector<std::string>{};

    test_variant v6 = 3.14f;
    assert(v6.index() == 0);

    std::cout << "v6: " << std::get<0>(v6) << '\n';
}

