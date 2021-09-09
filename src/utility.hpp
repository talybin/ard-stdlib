#pragma once
#include <utility>

#if __cplusplus < 201703L
namespace std
{
    /// \see https://en.cppreference.com/w/cpp/utility/in_place
    struct in_place_t {
        explicit in_place_t() = default;
    };
    template <class>
    struct in_place_type_t {
        explicit in_place_type_t() = default;
    };
    template <std::size_t>
    struct in_place_index_t {
        explicit in_place_index_t() = default;
    };

} // namespace std
#endif // __cplusplus < 201703L

