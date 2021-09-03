#pragma once
#include <type_traits>

#if __cplusplus < 201703L
namespace std
{
    /// \see https://en.cppreference.com/w/cpp/types/void_t
    template <class...>
    using void_t = void;

    /// \see https://en.cppreference.com/w/cpp/types/integral_constant
    template <bool _Bp>
    using bool_constant = std::integral_constant<bool, _Bp>;

    /// \see https://en.cppreference.com/w/cpp/types/conjunction
    template <class...> struct conjunction : std::true_type {};
    template <class _Tp> struct conjunction<_Tp> : _Tp {};
    template <class _First, class... _Rest>
    struct conjunction<_First, _Rest...>
    : conditional_t<bool(_First::value), conjunction<_Rest...>, _First> {};

    /// \see https://en.cppreference.com/w/cpp/types/disjunction
    template <class...> struct disjunction : std::false_type {};
    template <class _Tp> struct disjunction<_Tp> : _Tp {};
    template <class _First, class... _Rest>
    struct disjunction<_First, _Rest...> 
    : std::conditional_t<bool(_First::value), _First, disjunction<_Rest...>>  {};

    /// \see https://en.cppreference.com/w/cpp/types/negation
    template <class _Tp>
    struct negation : bool_constant<!bool(_Tp::value)> {};

    /// \see https://en.cppreference.com/w/cpp/types/is_swappable
    template <class, class = void>
    struct is_swappable : std::false_type {};

    template <class _Tp>
    struct is_swappable<_Tp,
        void_t<decltype(swap(std::declval<_Tp&>(), std::declval<_Tp&>()))>>
    : std::true_type {};

    template <class _Tp>
    struct is_nothrow_swappable {
    private:
        static bool_constant<
            noexcept(swap(std::declval<_Tp&>(), std::declval<_Tp&>()))>
        __test(int);

        static std::false_type __test(...);
    public:
        static constexpr bool value = decltype(__test(0))::value;
    };

} // namespace std
#endif // __cplusplus < 201703L

