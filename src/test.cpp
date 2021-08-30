#include "variant.hpp"
#include <iostream>

int main()
{
    std::variant<int, float, double, std::string> v = "hej";
    //variant<int, float, double, std::string> v;

    static_assert(std::is_same<int&, decltype(std::get<0>(v))>::value);
    static_assert(std::is_same<float&, decltype(std::get<1>(v))>::value);
    static_assert(std::is_same<double&, decltype(std::get<2>(v))>::value);
    static_assert(std::is_same<std::string&, decltype(std::get<3>(v))>::value);

    std::cout << "size (std::string): " << sizeof(std::string) << '\n';
    std::cout << "size (variant): " << sizeof(v) << '\n';

    std::cout << "cur index before: " << v.index() << '\n';
    std::cout << "value before: " << std::get<3>(v) << '\n';

    v = 3.14f;
    std::cout << "cur index after: " << v.index() << '\n';
    std::cout << "value after: " << std::get<1>(v) << '\n';

    std::visit([](auto x) { std::cout << "visiting: " << x << '\n'; }, v);

    //v.reset();

    std::variant<long, int> x = 2;
    std::cout << "x index: " << x.index() << '\n';

    int i = 2;
    std::variant<int> y = i;
    std::cout << "y index: " << y.index() << '\n';
}

