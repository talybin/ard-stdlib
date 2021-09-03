/*
 * Implementation of std::variant for Arduino.
 *
 * Note! Since Arduino has exceptions disabled the implementation
 * become much simpler. All noexcept flags removed and no actions
 * (in case exception throws) for invalid state applied.
 * In case of invalid operation (like getting value of valueless
 * variant) the program will abort (call std::abort()).
 *
 * Features:
 *   - Works with C++14.
 *   - Does not instantiate objects before assigning (stdlib do).
 */

#pragma once

#if __cplusplus >= 201703L
#include <variant>
#else

#include <tuple>
#include <algorithm>

#include "type_traits.hpp"
#include "utility.hpp"
#include "memory.hpp"

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

    // monostate
    struct monostate {};

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

            // How many times does T appear in type sequence?
            template <class _Tp, class... _Types>
            struct __type_count : std::integral_constant<size_t, 0> {};

            template <class _Tp, class _First, class... _Rest>
            struct __type_count<_Tp, _First, _Rest...> : std::integral_constant<
                size_t,
                __type_count<_Tp, _Rest...>::value + std::is_same<_Tp, _First>::value> {};

            // __exactly_once
            template <class _Tp, class... _Types>
            struct __exactly_once
            : std::bool_constant<__type_count<_Tp, _Types...>::value == 1> {};

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
                std::void_t<decltype(__overload_set<_Types...>::_fun(std::declval<_Tp>()))>>
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
            constexpr decltype(auto)
            __raw_get(_Variant&& __variant)
            { return std::forward<_Variant>(__variant).template _get<_Np>(); }

            template <size_t _Np, class _Variant>
            constexpr decltype(auto)
            __get(_Variant&& __variant) {
                if (__variant.index() != _Np)
                    std::abort();
                return __raw_get<_Np>(std::forward<_Variant>(__variant));
            }

            // Raw index visitor
            template <size_t _Np, class _Visitor>
            constexpr decltype(auto)
            __raw_idx_visit(_Visitor&& __visitor) {
                return std::forward<_Visitor>(__visitor)(std::integral_constant<size_t, _Np>{});
            }

            template <class _Visitor, size_t... _Ind,
                class _Ret = std::common_type_t<
                    decltype(std::declval<_Visitor>()(std::integral_constant<size_t, _Ind>{}))...>
            >
            constexpr decltype(auto)
            __raw_idx_visit(size_t __index, _Visitor&& __visitor, std::index_sequence<_Ind...>)
            {
                if (__index < sizeof...(_Ind)) {
                    constexpr _Ret (*__vtable[])(_Visitor&&) = {
                        &__raw_idx_visit<_Ind, _Visitor>...
                    };
                    return __vtable[__index](std::forward<_Visitor>(__visitor));
                }
                std::abort();
            }

            template <class _Visitor, class... _Types>
            constexpr decltype(auto)
            __raw_idx_visit(_Visitor&& __visitor, const variant<_Types...>& __variant) {
                return __raw_idx_visit(
                    __variant.index(),
                    std::forward<_Visitor>(__visitor),
                    std::make_index_sequence<sizeof...(_Types)>{});
            }

            // Destroy non-trivially destructible type
            template <class _Tp, bool = std::is_trivially_destructible<_Tp>::value>
            struct __destroy
            { constexpr void operator()(_Tp*) const {} };

            template <class _Tp>
            struct __destroy<_Tp, false> {
                constexpr void operator()(_Tp* __object_ptr) const
                { std::destroy_at(__object_ptr); }
            };

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
        static constexpr bool __exactly_once =
            __detail::__variant::__exactly_once<_Tp, _Types...>::value;

        template <class _Tp>
        static constexpr bool __not_in_place_tag =
           !__detail::__variant::__is_in_place_tag<std::decay_t<_Tp>>::value;

        // Construct value by index
        template <size_t _Np, class _Tp = __to_type<_Np>, class... _Args>
        constexpr _Tp&
        _construct(_Args&&... __args) {
            _Tp* __ret = ::new (_storage) _Tp(std::forward<_Args>(__args)...);
            _index = _Np;
            return *__ret;
        }

        // Destruct if no valueless
        constexpr void
        _destruct() {
            if (_index != variant_npos) {
                __detail::__variant::__raw_idx_visit([this](auto _Np) {
                    using _Tp = __to_type<_Np>;
                    __detail::__variant::__destroy<_Tp>{}((_Tp*)_storage);
                }, *this);
                _index = variant_npos;
            }
        }

        // Raw getters
        template <size_t _Np, class _Tp = __to_type<_Np>>
        const _Tp& _get() const& { return *(_Tp*)_storage; }

        template <size_t _Np, class _Tp = __to_type<_Np>>
        _Tp& _get() & { return *(_Tp*)_storage; }

        template <size_t _Np, class _Tp = __to_type<_Np>>
        const _Tp&& _get() const&&  { return std::move(*(_Tp*)_storage); }

        template <size_t _Np, class _Tp = __to_type<_Np>>
        _Tp&& _get() && { return std::move(*(_Tp*)_storage); }

        // External getter
        template <size_t, class _Variant>
        friend constexpr decltype(auto) __detail::__variant::__raw_get(_Variant&&);

        // Value holder (union)
        unsigned char _storage[std::max({ sizeof(_Types)... })];
        size_t _index = variant_npos;

    public:
        // Constructors
        // 1
        constexpr variant() = default;
        // 2
        constexpr variant(const variant& __other);
        // 3
        constexpr variant(variant&& __other);

        // 4
        template <class _Tp,
            class = std::enable_if_t<__not_in_place_tag<_Tp>>,
            class _Tj = __accepted_type<_Tp&&>,
            class = std::enable_if_t<__exactly_once<_Tj> &&
                std::is_constructible<_Tj, _Tp>::value>
        >
        constexpr
        variant(_Tp&& __t)
        : variant(std::in_place_index_t<__index_of<_Tj>>{}, std::forward<_Tp>(__t))
        { }

        // 5
        template <class _Tp, class... _Args,
            class = std::enable_if_t<__exactly_once<_Tp> &&
                std::is_constructible<_Tp, _Args...>::value>
        >
        constexpr explicit
        variant(std::in_place_type_t<_Tp>, _Args&&... __args)
        : variant(std::in_place_index_t<__index_of<_Tp>>{}, std::forward<_Args>(__args)...)
        { }

        // 6
        template <class _Tp, class _Up, class... _Args,
            class = std::enable_if_t<__exactly_once<_Tp> &&
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

        // Destructor
        ~variant()
        { _destruct(); }

        // Assignments
        // 1
        constexpr variant& operator=(const variant& __rhs);
        // 2
        constexpr variant& operator=(variant&& __rhs);

        // 3
        template <class _Tp,
            size_t _Np = __accepted_index<_Tp&&>,
            class _Tj = __to_type<_Np>,
            class = std::enable_if_t<__exactly_once<_Tj> &&
                std::is_constructible<_Tj, _Tp>::value &&
                std::is_assignable<_Tj&, _Tp>::value>
        >
        constexpr variant&
        operator=(_Tp&& __rhs) {
            if (_index == _Np)
                _get<_Np, _Tj>() = std::forward<_Tp>(__rhs);
            else {
                _destruct();
                _construct<_Np, _Tj>(std::forward<_Tp>(__rhs));
            }
            return *this;
        }

        // Emplace
        // 1
        template <class _Tp, class... _Args,
            class = std::enable_if_t<__exactly_once<_Tp> &&
                std::is_constructible<_Tp, _Args...>::value>
        >
        constexpr _Tp&
        emplace(_Args&&... __args)
        { return this->emplace<__index_of<_Tp>>(std::forward<_Args>(__args)...); }

        // 2
        template <class _Tp, class _Up, class... _Args,
            class = std::enable_if_t<__exactly_once<_Tp> &&
                std::is_constructible<_Tp, std::initializer_list<_Up>&, _Args...>::value>
        >
        constexpr _Tp&
        emplace(std::initializer_list<_Up> __il, _Args&&... __args)
        { return this->emplace<__index_of<_Tp>>(__il, std::forward<_Args>(__args)...); }

        // 3
        template <size_t _Np, class... _Args,
            class = std::enable_if_t<
                std::is_constructible<variant_alternative_t<_Np, variant>, _Args...>::value>
        >
        constexpr variant_alternative_t<_Np, variant>&
        emplace(_Args&&... __args) {
            _destruct();
            return _construct<_Np>(std::forward<_Args>(__args)...);
        }

        // 4
        template <size_t _Np, class _Up, class... _Args,
            class = std::enable_if_t<
                std::is_constructible<variant_alternative_t<_Np, variant>,
                    std::initializer_list<_Up>&, _Args...>::value>
        >
        variant_alternative_t<_Np, variant>&
        emplace(std::initializer_list<_Up> __il, _Args&&... __args) {
            _destruct();
            return _construct<_Np>(__il, std::forward<_Args>(__args)...);
        }

        // Swap
        constexpr void
        swap(variant& __rhs);

        // Returns the zero-based index of the alternative held by the variant
        constexpr size_t index() const
        { return _index; }

        // Returns false if and only if the variant holds a value
        constexpr bool valueless_by_exception() const
        { return _index == variant_npos; }
    };

    // get
    // 1
    template <size_t _Np, class... _Types>
    constexpr variant_alternative_t<_Np, variant<_Types...>>&
    get(variant<_Types...>& __v)
    { return __detail::__variant::__get<_Np>(__v); }

    template <size_t _Np, class... _Types>
    constexpr variant_alternative_t<_Np, variant<_Types...>>&&
    get(variant<_Types...>&& __v)
    { return __detail::__variant::__get<_Np>(std::move(__v)); }

    template <size_t _Np, class... _Types>
    constexpr variant_alternative_t<_Np, variant<_Types...>> const&
    get(const variant<_Types...>& __v)
    { return __detail::__variant::__get<_Np>(__v); }

    template <size_t _Np, class... _Types>
    constexpr variant_alternative_t<_Np, variant<_Types...>> const&&
    get(const variant<_Types...>&& __v)
    { return __detail::__variant::__get<_Np>(std::move(__v)); }

    // get
    // 2
    template <class _Tp, class... _Types>
    constexpr _Tp&
    get(std::variant<_Types...>& __v) {
        static_assert(__detail::__variant::__exactly_once<_Tp, _Types...>::value,
            "_Tp must occur exactly once in alternatives");
        return get<__detail::__variant::__index_of<_Tp, _Types...>::value>(__v);
    }

    template <class _Tp, class... _Types>
    constexpr _Tp&&
    get(std::variant<_Types...>&& __v) {
        static_assert(__detail::__variant::__exactly_once<_Tp, _Types...>::value,
            "_Tp must occur exactly once in alternatives");
        return get<__detail::__variant::__index_of<_Tp, _Types...>::value>(std::move(__v));
    }

    template <class _Tp, class... _Types>
    constexpr const _Tp&
    get(const std::variant<_Types...>& __v) {
        static_assert(__detail::__variant::__exactly_once<_Tp, _Types...>::value,
            "_Tp must occur exactly once in alternatives");
        return get<__detail::__variant::__index_of<_Tp, _Types...>::value>(__v);
    }

    template <class _Tp, class... _Types>
    constexpr const _Tp&&
    get(const std::variant<_Types...>&& __v) {
        static_assert(__detail::__variant::__exactly_once<_Tp, _Types...>::value,
            "_Tp must occur exactly once in alternatives");
        return get<__detail::__variant::__index_of<_Tp, _Types...>::value>(std::move(__v));
    }

    // get_if
    // 1
    template <size_t _Np, class... _Types>
    constexpr std::add_pointer_t<variant_alternative_t<_Np, variant<_Types...>>>
    get_if(variant<_Types...>* __ptr) {
        if (__ptr && __ptr->index() == _Np)
            return std::addressof(__detail::__variant::__raw_get<_Np>(*__ptr));
        return nullptr;
    }

    template <size_t _Np, class... _Types>
    constexpr std::add_pointer_t<const variant_alternative_t<_Np, variant<_Types...>>>
    get_if(const variant<_Types...>* __ptr) {
        if (__ptr && __ptr->index() == _Np)
            return std::addressof(__detail::__variant::__raw_get<_Np>(*__ptr));
        return nullptr;
    }

    // 2
    template <class _Tp, class... _Types>
    constexpr add_pointer_t<_Tp>
    get_if(variant<_Types...>* __ptr) {
        static_assert(__detail::__variant::__exactly_once<_Tp, _Types...>::value,
            "_Tp must occur exactly once in alternatives");
        return get_if<__detail::__variant::__index_of<_Tp, _Types...>::value>(__ptr);
    }

    template <class _Tp, class... _Types>
    constexpr add_pointer_t<const _Tp>
    get_if(const variant<_Types...>* __ptr) {
        static_assert(__detail::__variant::__exactly_once<_Tp, _Types...>::value,
            "_Tp must occur exactly once in alternatives");
        return get_if<__detail::__variant::__index_of<_Tp, _Types...>::value>(__ptr);
    }

    // visit
    // Support only one variant for now
    template <class _Visitor, class _Variant>
    constexpr decltype(auto)
    visit(_Visitor&& __visitor, _Variant&& __variant)
    {
        if (__variant.valueless_by_exception())
            std::abort();

        return __detail::__variant::__raw_idx_visit(
            [&](auto _Np) {
                return std::forward<_Visitor>(__visitor)(
                    __detail::__variant::__raw_get<_Np>(std::forward<_Variant>(__variant)));
            },
            std::forward<_Variant>(__variant));
    }

    // Constructors
    // 2
    template <class... _Types>
    constexpr
    variant<_Types...>::variant(const variant<_Types...>& __other) 
    {
        if (!__other.valueless_by_exception()) {
            __detail::__variant::__raw_idx_visit(
                [this, &__other](auto _Np) {
                    _construct<_Np>(__other.template _get<_Np>());
                },
                __other);
        }
    }

    // 3
    template <class... _Types>
    constexpr
    variant<_Types...>::variant(variant<_Types...>&& __other)
    {
        if (!__other.valueless_by_exception()) {
            __detail::__variant::__raw_idx_visit(
                [this, &__other](auto _Np) {
                    _construct<_Np>(std::move(__other).template _get<_Np>());
                },
                __other);
        }
    }

    // Assignments
    // 1
    template <class... _Types>
    constexpr variant<_Types...>&
    variant<_Types...>::operator=(const variant<_Types...>& __rhs)
    {
        // Note, _destruct will destroy value only if not valueless
        if (__rhs.valueless_by_exception()) {
            // If both *this and rhs are valueless by exception, do nothing.
            // Otherwise, if rhs is valueless, but *this is not, destroy
            // the value contained in *this and makes it valueless.
            _destruct();
        }
        else { // rhs contains a value
            __detail::__variant::__raw_idx_visit(
                [this, &__rhs](auto _Np) {
                    // If rhs holds the same alternative as *this, assign the
                    // value contained in rhs to the value contained in *this
                    if (__rhs._index == _index)
                        _get<_Np>() = __rhs.template _get<_Np>();
                    else {
                        // rhs and *this has different index
                        _destruct();
                        _construct<_Np>(__rhs.template _get<_Np>());
                    }
                },
                __rhs);
        }
        return *this;
    }

    // 2
    template <class... _Types>
    constexpr variant<_Types...>&
    variant<_Types...>::operator=(variant<_Types...>&& __rhs)
    {
        // Same as copy assignment but moves in value
        if (__rhs.valueless_by_exception())
            _destruct();
        else { // rhs contains a value
            __detail::__variant::__raw_idx_visit(
                [this, &__rhs](auto _Np) {
                    if (__rhs._index == _index)
                        _get<_Np>() = std::move(__rhs).template _get<_Np>();
                    else {
                        // rhs and *this has different index
                        _destruct();
                        _construct<_Np>(std::move(__rhs).template _get<_Np>());
                    }
                },
                __rhs);
        }
        return *this;
    }

    // Swap
    template <class... _Types>
    constexpr void
    variant<_Types...>::swap(variant<_Types...>& __rhs)
    {
        // Note! Just swapping _storage and _index do not always work (ex.
        // std::string, seems to hold internal pointer to it self).
        if (valueless_by_exception() && __rhs.valueless_by_exception())
            return;

        __detail::__variant::__raw_idx_visit([this, &__rhs](auto __rhs_index) {
        },
        __rhs);

#if 0
        __detail::__variant::__raw_idx_visit([this, &__rhs](auto __thisI) {
            __detail::__variant::__raw_idx_visit([this, &__rhs](auto __rhsI)
            {
                
                if (__rhs._index == _index) {
                    
                }
            },
            __rhs);
        },
        *this);        

        __detail::__variant::__raw_idx_visit(
            [this, &__rhs](auto _Np) {
                if (
            },
            __rhs);

        // If both *this and rhs are valueless by exception, do nothing.
        // Otherwise, if both *this and rhs hold the same alternative, call
        // swap(std::get<i>(*this), std::get<i>(rhs)) where i is index().
        if (__rhs._index == _index) {
            if (!valueless_by_exception())
               std::swap(_storage, __rhs._storage);
        }
        // Otherwise, exchange values of rhs and *this.
        else {
            std::swap(_storage, __rhs._storage);
            std::swap(_index, __rhs._index);
        }
#endif
    }

} // namespace std

#endif // __cplusplus < 201703L

