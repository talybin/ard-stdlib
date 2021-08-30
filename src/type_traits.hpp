#pragma once
#include <type_traits>

#if __cplusplus < 201703L
namespace std
{
    /// \see https://en.cppreference.com/w/cpp/types/void_t
    template <class...>
    using void_t = void;

    /// \see https://en.cppreference.com/w/cpp/types/integral_constant
    template <bool B>
    using bool_constant = std::integral_constant<bool, B>;

    /// \see https://en.cppreference.com/w/cpp/types/conjunction
    template <class...> struct conjunction : std::true_type { };
    template <class T1> struct conjunction<T1> : T1 { };
    template <class T1, class... Tn>
    struct conjunction<T1, Tn...>
    : conditional_t<bool(T1::value), conjunction<Tn...>, T1> { };

    /// \see https://en.cppreference.com/w/cpp/types/disjunction
    template <class...> struct disjunction : std::false_type { };
    template <class T1> struct disjunction<T1> : T1 { };
    template <class T1, class... Tn>
    struct disjunction<T1, Tn...> 
    : std::conditional_t<bool(T1::value), T1, disjunction<Tn...>>  { };

    /// \see https://en.cppreference.com/w/cpp/types/negation
    template <class T>
    struct negation : bool_constant<!bool(T::value)> { };

} // namespace std
#endif // __cplusplus < 201703L

