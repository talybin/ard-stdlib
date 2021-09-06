#include "variant.hpp"
#include <iostream>


int main()
{
    std::variant<int, float, double, std::string> v = "hej";

    static_assert(std::is_same<int&, decltype(std::get<0>(v))>::value);
    static_assert(std::is_same<float&, decltype(std::get<1>(v))>::value);
    static_assert(std::is_same<double&, decltype(std::get<2>(v))>::value);
    static_assert(std::is_same<std::string&, decltype(std::get<3>(v))>::value);

    std::cout << "size (std::string): " << sizeof(std::string) << '\n';
    std::cout << "size (variant): " << sizeof(v) << '\n';

    std::cout << "cur index before: " << v.index() << '\n';
    std::cout << "value before: " << std::get<3>(v) << '\n';

    v = 3.14f;
    //std::get<9>(v);
    //std::get<1>(v);
    //std::get<0>(v);

    // https://github.com/TartanLlama/expected/blob/master/include/tl/expected.hpp
    // https://github.com/martinmoene/expected-lite/blob/master/include/nonstd/expected.hpp
    #if 0
    ard::expected<int, std::exception> r = ard::get<0>(v);
    if (r)
        std::cout << r.value() << '\n';
    else
        r.error().what()
    #endif

    std::cout << "cur index after: " << v.index() << '\n';
    std::cout << "value after: " << std::get<1>(v) << '\n';

    const char* ret = std::visit([](auto x) {
        std::cout << "visiting: " << x << '\n';
        return "test";
    }, v);
    std::cout << "ret: " << ret << '\n';

    //v.reset();

    std::variant<long, int> x = 2;
    std::cout << "x index: " << x.index() << '\n';

    int i = 2;
    std::variant<int> y = i;
    std::cout << "y index: " << y.index() << '\n';

    // Test valueless
    std::variant<int, float, double, std::string> v2;
    std::cout << "valueless: " << v2.valueless_by_exception() << '\n';

    v2 = 42;
    std::cout << v2.index() << '\n';

    int val = 0;
    using testv1 = std::variant<int>;

    auto f1 = [&val](int) -> int { return val; };
    auto f2 = [&val](int) -> int& { return val; };
    auto f3 = [&val](int) -> const int& { return val; };
    auto f4 = [&val](int) -> const int&& { return std::move(val); };
    auto f5 = [&val](int) -> int&& { return std::move(val); };
 

    static_assert(std::is_same<
        decltype(f1(std::get<0>(std::declval<testv1>()))), int>::value);

    static_assert(std::is_same<
        decltype(std::visit(f1, std::variant<int>{0})), int>::value);


    static_assert(std::is_same<
        decltype(f2(std::get<0>(std::declval<testv1>()))), int&>::value);

    static_assert(std::is_same<
        decltype(std::visit(f2, std::variant<int>{0})), int&>::value);


    static_assert(std::is_same<
        decltype(f3(std::get<0>(std::declval<testv1>()))), const int&>::value);

    static_assert(std::is_same<
        decltype(std::visit(f3, std::variant<int>{0})), const int&>::value);


    static_assert(std::is_same<
        decltype(f4(std::get<0>(std::declval<testv1>()))), const int&&>::value);

    static_assert(std::is_same<
        decltype(std::visit(f4, std::variant<int>{0})), const int&&>::value);


    static_assert(std::is_same<
        decltype(f5(std::get<0>(std::declval<testv1>()))), int&&>::value);

    static_assert(std::is_same<
        decltype(std::visit(f5, std::variant<int>{0})), int&&>::value);
}

