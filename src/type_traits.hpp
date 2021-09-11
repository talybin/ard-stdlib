#pragma once
#include <type_traits>

#if __cplusplus < 201703L
// C++17 type traits
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

    #ifndef __cpp_lib_is_swappable
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
        static bool_constant<noexcept(swap(std::declval<_Tp&>(), std::declval<_Tp&>()))>
        __test(int);
        static false_type __test(...);
    public:
        static constexpr bool value = decltype(__test(0))::value;
    };

    template <class, class, class = void>
    struct is_swappable_with : std::false_type {};

    template <class _Tp, class _Up>
    struct is_swappable_with<_Tp, _Up, std::void_t<
        decltype(swap(std::declval<_Tp>(), std::declval<_Up>())),
        decltype(swap(std::declval<_Up>(), std::declval<_Tp>()))>>
    : std::true_type {};

    template <class _Tp, class _Up>
    struct is_nothrow_swappable_with {
    private:
        static bool_constant<
            noexcept(swap(std::declval<_Tp>(), std::declval<_Up>())) &&
            noexcept(swap(std::declval<_Up>(), std::declval<_Tp>()))>
        __test(int);
        static false_type __test(...);
    public:
        static constexpr bool value = decltype(__test(0))::value;
    };
    #endif

} // namespace std
#endif // C++17

#if __cplusplus <= 201703L
// C++20 type traits
namespace std
{
    /// \see https://en.cppreference.com/w/cpp/types/type_identity
    template <class _Tp>
    struct type_identity
    { using type = _Tp; };

    template <class _Tp>
    using type_identity_t = typename type_identity<_Tp>::type;

    /// \see https://en.cppreference.com/w/cpp/types/remove_cvref
    template <class _Tp>
    struct remove_cvref {
        using type = std::remove_cv_t<std::remove_reference_t<_Tp>>;
    };

    template <class _Tp>
    using remove_cvref_t = typename remove_cvref<_Tp>::type;

} // namespace std
#endif // C++20

// std::experimental
namespace std
{
    /// \see https://en.cppreference.com/w/cpp/experimental/nonesuch
    struct nonesuch {
        ~nonesuch() = delete;
        nonesuch(nonesuch const&) = delete;
        void operator=(nonesuch const&) = delete;
    };

    namespace __detail
    {
        template <class _Default, class _AlwaysVoid,
            template <class...> class _Op, class... _Args>
        struct __detector {
            using value_t = std::false_type;
            using type = _Default;
        };
 
        template <class _Default, template <class...> class _Op, class... _Args>
        struct __detector<_Default, std::void_t<_Op<_Args...>>, _Op, _Args...> {
            using value_t = std::true_type;
            using type = _Op<_Args...>;
        };
 
    } // namespace __detail
 
    /// \see https://en.cppreference.com/w/cpp/experimental/is_detected
    template <template <class...> class _Op, class... _Args>
    using is_detected = typename __detail::__detector<nonesuch, void, _Op, _Args...>::value_t;
 
    template <template <class...> class _Op, class... _Args>
    using detected_t = typename __detail::__detector<nonesuch, void, _Op, _Args...>::type;
 
    template <class _Default, template <class...> class _Op, class... _Args>
    using detected_or = __detail::__detector<_Default, void, _Op, _Args...>;

    template <class _Default, template <class...> class _Op, class... _Args>
    using detected_or_t = typename detected_or<_Default, _Op, _Args...>::type;

    template <class _Expected, template <class...> class _Op, class... _Args>
    using is_detected_exact = std::is_same<_Expected, detected_t<_Op, _Args...>>;

    template <class _To, template <class...> class _Op, class... _Args>
    using is_detected_convertible = std::is_convertible<detected_t<_Op, _Args...>, _To>;

} // namespace std

