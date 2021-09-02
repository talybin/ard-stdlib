/*
 * Implementation of std::variant for Arduino
 *
 * Features:
 *   - Works with C++14
 *   - Does not instantiate objects before assigning
 */

#pragma once

#if __cplusplus >= 201703L
#include <variant>
#else

#include <tuple>
#include <algorithm>

#include "type_traits.hpp"
#include "utility.hpp"

namespace std
{
    template <class...>
    struct variant;

    // variant_npos
    enum : size_t { variant_npos = size_t(-1) };

    // variant_size
    template <class>
    struct variant_size;

    template <class... _Types>
    struct variant_size<variant<_Types...>>
    : std::integral_constant<size_t, sizeof...(_Types)> {};

    // variant_alternative
    template <size_t _Np, class _Variant>
    struct variant_alternative;

    template <size_t _Np, class... _Types>
    struct variant_alternative<_Np, variant<_Types...>>
    : std::tuple_element<_Np, std::tuple<_Types...>> {};

    template <size_t _Np, class _Variant>
    using variant_alternative_t = typename variant_alternative<_Np, _Variant>::type;

    // detail
    namespace __detail {
        namespace __variant
        {
            // Returns the first appearence of _Tp in _Types.
            // Returns sizeof...(_Types) if _Tp is not in _Types.
            template <class _Tp, class... _Types>
            struct __index_of : std::integral_constant<size_t, 0> {};

            template <class _Tp, class _First, class... _Rest>
            struct __index_of<_Tp, _First, _Rest...> : std::integral_constant<
                size_t,
                std::is_same<_Tp, _First>::value ? 0 : __index_of<_Tp, _Rest...>::value + 1> {};

            // Takes _Types and create an indexed overloaded _fun for each type.
            // If a type appears more than once in _Types, create only one overload.
            template <class... _Types>
            struct __overload_set {
                static void _fun();
            };

            template <class _First, class... _Rest>
            struct __overload_set<_First, _Rest...> : __overload_set<_Rest...> {
                using __overload_set<_Rest...>::_fun;
                static std::integral_constant<size_t, sizeof...(_Rest)> _fun(_First);
            };

            // accepted_index
            template <class _Tp, class _Variant, class = void>
            struct __accepted_index {
                static constexpr size_t value = variant_npos;
            };

            // Find index to overload that _Tp is assignable to.
            template <class _Tp, class... _Types>
            struct __accepted_index<_Tp, variant<_Types...>,
                std::void_t<decltype(__overload_set<_Types...>::_fun(std::declval<_Tp>()))> >
            {
                static constexpr size_t value = sizeof...(_Types) - 1 -
                    decltype(__overload_set<_Types...>::_fun(std::declval<_Tp>()))::value;
            };

            // Chack _Tp is in_place_type_t or in_place_index_t
            template <class>
            struct __is_in_place_tag : std::false_type {};

            template <class _Tp>
            struct __is_in_place_tag<std::in_place_type_t<_Tp>> : std::true_type {};

            template <size_t _Np>
            struct __is_in_place_tag<std::in_place_index_t<_Np>> : std::true_type {};

            // External getter
            template <size_t _Np, class _Variant>
            constexpr decltype(auto) __get_value(_Variant&& __variant) {
                if (__variant.index() != _Np)
                    std::abort();
                return std::forward<_Variant>(__variant).template get<_Np>();
            }

            // visit invoker
            template <size_t _Np, class _Visitor, class _Variant>
            constexpr decltype(auto)
            __visit_impl(_Visitor&& __visitor, _Variant&& __variant) {
                return std::forward<_Visitor>(__visitor)(
                    __get_value<_Np>(std::forward<_Variant>(__variant)));
            }

            // Visitor implementation
            template <class _Visitor, class _Variant, size_t... _Ind>
            constexpr decltype(auto)
            __visit_impl(
                _Visitor&& __visitor, _Variant&& __variant, std::index_sequence<_Ind...>)
            {
                if (__variant.valueless_by_exception())
                    std::abort();
                    //throw_bad_variant_access("visit: variant is valueless");

                using _Ret = decltype(__visitor(__get_value<0>(std::declval<_Variant>())));
                constexpr _Ret (*__vtable[])(_Visitor&&, _Variant&&) = {
                    &__visit_impl<_Ind, _Visitor, _Variant>...
                };
                return __vtable[__variant.index()](
                    std::forward<_Visitor>(__visitor), std::forward<_Variant>(__variant));
            }

        } // namespace __variant
    } // namespace __detail

    template <class... _Types>
    struct variant {
    private:
        static_assert(sizeof...(_Types) > 0,
            "variant must have at least one alternative");
        static_assert(!std::disjunction<std::is_reference<_Types>...>::value,
            "variant must have no reference alternative");
        static_assert(!std::disjunction<std::is_void<_Types>...>::value,
            "variant must have no void alternative");

        // Alias
        template <class _Tp>
        static constexpr bool __not_self =
            !std::is_same<std::decay_t<_Tp>, variant>::value;

        template <class _Tp>
        static constexpr size_t __accepted_index =
            __detail::__variant::__accepted_index<_Tp, variant>::value;

        template <size_t _Np, class = std::enable_if_t<(_Np < sizeof...(_Types))>>
        using __to_type = variant_alternative_t<_Np, variant>;

        template <class _Tp, class = std::enable_if_t<__not_self<_Tp>>>
        using __accepted_type = __to_type<__accepted_index<_Tp>>;

        template <class _Tp>
        static constexpr size_t __index_of =
            __detail::__variant::__index_of<_Tp, _Types...>::value;

        template <class _Tp>
        static constexpr bool __not_in_place_tag =
           !__detail::__variant::__is_in_place_tag<std::decay_t<_Tp>>::value;

        // Construct value by index
        template <size_t _Np, class _Tp = __to_type<_Np>, class... _Args>
        constexpr void
        _construct(_Args&&... __args)
        noexcept(std::is_nothrow_constructible<_Tp, _Args...>::value)
        {
            ::new (_storage) _Tp(std::forward<_Args>(__args)...);
            _index = _Np;
        }

        // Raw getters
        template <size_t _Np, class _Tp = __to_type<_Np>>
        const _Tp& get() const& noexcept { return *(_Tp*)_storage; }

        template <size_t _Np, class _Tp = __to_type<_Np>>
        _Tp& get() & noexcept { return *(_Tp*)_storage; }

        template <size_t _Np, class _Tp = __to_type<_Np>>
        const _Tp&& get() const&& noexcept { return std::move(*(_Tp*)_storage); }

        template <size_t _Np, class _Tp = __to_type<_Np>>
        _Tp&& get() && noexcept { return std::move(*(_Tp*)_storage); }

        // External getter
        template <size_t, class _Variant>
        friend constexpr decltype(auto) __detail::__variant::__get_value(_Variant&&);

        // Value holder (union)
        unsigned char _storage[std::max({ sizeof(_Types)... })];
        size_t _index = variant_npos;

    public:
        // Constructors
        // 1
        constexpr variant() noexcept = default;
        // 2
        constexpr variant(const variant& __other);
        // 3
        constexpr variant(variant&& __other);

        // 4
        template <class _Tp,
            class = std::enable_if_t<__not_in_place_tag<_Tp>>,
            class _Tj = __accepted_type<_Tp&&>,
            class = std::enable_if_t<std::is_constructible<_Tj, _Tp>::value>
        >
        constexpr
        variant(_Tp&& __t)
        noexcept(std::is_nothrow_constructible<_Tj, _Tp>::value)
        : variant(std::in_place_index_t<__index_of<_Tj>>{}, std::forward<_Tp>(__t))
        { }

        // 5
        template <class _Tp, class... _Args,
            class = std::enable_if_t<std::is_constructible<_Tp, _Args...>::value>
        >
        constexpr explicit
        variant(std::in_place_type_t<_Tp>, _Args&&... __args)
        : variant(std::in_place_index_t<__index_of<_Tp>>{}, std::forward<_Args>(__args)...)
        { }

        // 6
        template <class _Tp, class _Up, class... _Args,
            class = std::enable_if_t<
                std::is_constructible<_Tp, std::initializer_list<_Up>&, _Args...>::value>
        >
        constexpr explicit
        variant(std::in_place_type_t<_Tp>,
            std::initializer_list<_Up> __il, _Args&&... __args)
        : variant(
            std::in_place_index_t<__index_of<_Tp>>{}, __il, std::forward<_Args>(__args)...)
        { }

        // 7
        template <size_t _Np, class... _Args,
            class _Tp = __to_type<_Np>,
            class = std::enable_if_t<std::is_constructible<_Tp, _Args...>::value>
        >
        constexpr explicit
        variant(std::in_place_index_t<_Np>, _Args&&... __args)
        { _construct<_Np, _Tp>(std::forward<_Args>(__args)...); }

        // 8
        template <size_t _Np, class _Up, class... _Args,
            class _Tp = __to_type<_Np>,
            class = std::enable_if_t<
                std::is_constructible<_Tp, std::initializer_list<_Up>&, _Args...>::value>
        >
        constexpr explicit
        variant(std::in_place_index_t<_Np>,
            std::initializer_list<_Up> __il, _Args&&... __args)
        { _construct<_Np, _Tp>(__il, std::forward<_Args>(__args)...); }

        // Returns the zero-based index of the alternative held by the variant
        constexpr size_t index() const noexcept
        { return _index; }

        // Returns false if and only if the variant holds a value
        constexpr bool valueless_by_exception() const noexcept
        { return _index == variant_npos; }
    };

    // get
    // 1
    template <size_t _Np, class... _Types>
    constexpr variant_alternative_t<_Np, variant<_Types...>>&
    get(variant<_Types...>& __v)
    { return __detail::__variant::__get_value<_Np>(__v); }

    template <size_t _Np, class... _Types>
    constexpr variant_alternative_t<_Np, variant<_Types...>>&&
    get(variant<_Types...>&& __v)
    { return __detail::__variant::__get_value<_Np>(std::move(__v)); }

    template <size_t _Np, class... _Types>
    constexpr variant_alternative_t<_Np, variant<_Types...>> const&
    get(const variant<_Types...>& __v)
    { return __detail::__variant::__get_value<_Np>(__v); }

    template <size_t _Np, class... _Types>
    constexpr variant_alternative_t<_Np, variant<_Types...>> const&&
    get(const variant<_Types...>&& __v)
    { return __detail::__variant::__get_value<_Np>(std::move(__v)); }

    // get
    // 2
    template <class _Tp, class... _Types>
    constexpr _Tp&
    get(std::variant<_Types...>& __v)
    { return get<__detail::__variant::__index_of<_Tp, _Types...>::value>(__v); }

    template <class _Tp, class... _Types>
    constexpr _Tp&&
    get(std::variant<_Types...>&& __v)
    { return get<__detail::__variant::__index_of<_Tp, _Types...>::value>(std::move(__v)); }

    template <class _Tp, class... _Types>
    constexpr const _Tp&
    get(const std::variant<_Types...>& __v)
    { return get<__detail::__variant::__index_of<_Tp, _Types...>::value>(__v); }

    template <class _Tp, class... _Types>
    constexpr const _Tp&&
    get(const std::variant<_Types...>&& __v)
    { return get<__detail::__variant::__index_of<_Tp, _Types...>::value>(std::move(__v)); }

    // visit
    // Support only one variant for now
    template <class _Visitor, class _Variant>
    constexpr decltype(auto)
    visit(_Visitor&& __visitor, _Variant&& __variant)
    {
        using _Vp = std::remove_reference_t<_Variant>;
        return __detail::__variant::__visit_impl(
            std::forward<_Visitor>(__visitor),
            std::forward<_Variant>(__variant),
            std::make_index_sequence<variant_size<_Vp>::value>{});
    }

    // Constructors
    // 2
    template <class... _Types>
    constexpr
    variant<_Types...>::variant(const variant<_Types...>& __other) 
    {
        // TODO use __raw_idx_visit instead to get exact type and for noexcept(...)
        if (!__other.valueless_by_exception()) {
            visit([this](const auto& arg) {
                using _Tp = decltype(arg);
                _construct<__index_of<_Tp>, _Tp>(arg);
            }, __other);
        }
    }

    // 3
    template <class... _Types>
    constexpr
    variant<_Types...>::variant(variant<_Types...>&& __other)
    {
        if (!__other.valueless_by_exception()) {
            visit([this](auto&& arg) {
                using _Tp = std::remove_reference_t<decltype(arg)>;
                _construct<__index_of<_Tp>, _Tp>(std::move(arg));
            }, std::move(__other));
        }
    }

} // namespace std

#endif // __cplusplus < 201703L

