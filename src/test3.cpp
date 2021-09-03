#include "variant2.hpp"

#include <iostream>
#include <vector>
#include <functional>
#include <cassert>

using Ref = std::reference_wrapper<std::vector<int>>;
std::ostream& operator<<(std::ostream& os, const Ref& value)
{
    os << "ref";
    return os;
}

int main()
{
    using test_variant = std::variant<int, Ref, std::string>;

    test_variant v1;
    test_variant v2 = 42;

    int i1 = 0;

    test_variant v3 = i1;
    test_variant v3_copy(v3);
    test_variant v3_move(std::move(v3));

    // Test assignment
    v2 = v3_copy;
    v2 = v1;
    v2 = std::move(v3_copy);

    test_variant v4 = "test";
    assert(v4.index() == 2);

    std::visit([](auto v) {
        std::cout << "---> visited copy: " << v << '\n';
    }, v4);

    std::visit([](auto&& v) {
        std::cout << "---> visited move: " << v << '\n';
    }, std::move(v4));

    std::vector<int> vec { 42 };
    test_variant v5 = vec;
    assert(v5.index() == 1);
    // TODO try to get or visit and use [0]

//    v5 = std::vector<std::string>{};

    test_variant v6 = 3.14f;
    assert(v6.index() == 0);

    std::cout << "v6: " << std::get<0>(v6) << '\n';
}

