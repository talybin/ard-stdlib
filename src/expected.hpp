/*
 * Implementation of std::expected for Arduino.
 * Vladimir Talybin
 * 2021
 *
 * std::expected is not actually in standard, it's a proposal with
 * following paper
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0323r4.html
 * But it may be usefull as alternative to exceptions.
 *
 * Removed:
 *   - bad_expected_access
 * 
 * Changes:
 *   - std::unexpected renamed to std::not_expected because depricated
 *     function with the same name is not removed until C++17.
 */

#pragma once
#include "variant.hpp"

namespace std
{
    // TODO fix me
    template <class _Ep>
    struct not_expected
    {
        not_expected() = delete;
        constexpr explicit not_expected(const _Ep&) {}
        constexpr explicit not_expected(_Ep&&) {}
        constexpr const _Ep& value() const & {}
        constexpr _Ep& value() & { return _M_value; }
        constexpr _Ep&& value() && { return _M_value; }
        constexpr _Ep const&& value() const && { return _M_value; }

    private:
        _Ep _M_value;
    };

    // unexpect tag
    struct unexpect_t {
        explicit unexpect_t() = default;
    };

    // Expected for object types
    template <class _Value, class _Error>
    struct expected {
    private:
        // TODO implement void
        static_assert(!std::is_same<_Value, void>::value, "void type not implemented yet");

        // Alias
        template <class _Tp>
        static constexpr bool __not_self =
            !std::is_same<std::decay_t<_Tp>, expected>::value;

        // Value holder
        std::variant<_Value, not_expected<_Error>> _M_storage;

    public:
        // Constructors
        constexpr expected()
        : _M_storage(std::in_place_index_t<0>{})
        { }

        constexpr expected(const expected&) = default;
        constexpr expected(expected&&) = default;

        template <class _Tp, class _Ep,
            class = std::enable_if_t<
                std::is_constructible<_Value, const _Tp&>::value &&
                std::is_constructible<_Error, const _Ep&>::value>
        >
        constexpr explicit
        expected(const expected<_Tp, _Ep>& __rhs) {
            if (__rhs.has_value())
                _M_storage.template emplace<0>(std::get<0>(__rhs._M_storage));
            else
                _M_storage.template emplace<1>(std::get<1>(__rhs._M_storage));
        }

        template <class _Tp, class _Ep,
            class = std::enable_if_t<
                std::is_constructible<_Value, _Tp&&>::value &&
                std::is_constructible<_Error, _Ep&&>::value>
        >
        constexpr explicit
        expected(expected<_Tp, _Ep>&& __rhs) {
            if (__rhs.has_value())
                _M_storage.template emplace<0>(std::get<0>(std::move(__rhs)._M_storage));
            else
                _M_storage.template emplace<1>(std::get<1>(std::move(__rhs)._M_storage));
        }

        template <class _Tp = _Value,
            class = std::enable_if_t<
                __not_self<_Tp> && std::is_constructible<_Value, _Tp>::value>
        >
        constexpr explicit
        expected(_Tp&& __v)
        : _M_storage(std::in_place_index_t<0>{}, std::forward<_Tp>(__v))
        { }

        template <class... _Args,
            class = std::enable_if_t<std::is_constructible<_Value, _Args...>::value>
        >
        constexpr explicit
        expected(std::in_place_t, _Args&&... __args)
        : _M_storage(std::in_place_index_t<0>{}, std::forward<_Args>(__args)...)
        { }

        template <class _Up, class... _Args,
            class = std::enable_if_t<
                std::is_constructible<_Value, std::initializer_list<_Up>&, _Args...>::value>
        >
        constexpr explicit
        expected(std::in_place_t, std::initializer_list<_Up> __il, _Args&&... __args)
        : _M_storage(std::in_place_index_t<0>{}, __il, std::forward<_Args>(__args)...)
        { }

        template <class _Ep = _Error>
        constexpr
        expected(const not_expected<_Ep>& __v)
        : _M_storage(std::in_place_index_t<1>{}, __v)
        { }

        template <class _Ep = _Error>
        constexpr
        expected(not_expected<_Ep>&& __v)
        : _M_storage(std::in_place_index_t<1>{}, std::move(__v))
        { }

        template <class... _Args,
            class = std::enable_if_t<std::is_constructible<
                not_expected<_Error>, _Args...>::value>
        >
        constexpr explicit
        expected(unexpect_t, _Args&&... __args)
        : _M_storage(std::in_place_index_t<1>{}, std::forward<_Args>(__args)...)
        { }

        template <class _Up, class... _Args,
            class = std::enable_if_t<std::is_constructible<
                not_expected<_Error>, std::initializer_list<_Up>&, _Args...>::value>
        >
        constexpr explicit
        expected(unexpect_t, std::initializer_list<_Up> __il, _Args&&... __args)
        : _M_storage(std::in_place_index_t<1>{}, __il, std::forward<_Args>(__args)...)
        { }

        // Destructor
        ~expected() = default;

        // Assignments
        expected& operator=(const expected&) = default;
        expected& operator=(expected&&) = default;

        template <class _Tp = _Value,
            class = std::enable_if_t<__not_self<_Tp>>
        >
        expected& operator=(_Tp&& __v)
        { return (_M_storage.template emplace<0>(std::forward<_Tp>(__v)), *this); }

        template <class _Ep = _Error>
        expected& operator=(const not_expected<_Ep>& __v)
        { return (_M_storage.template emplace<1>(__v), *this); }

        template <class _Ep = _Error>
        expected& operator=(not_expected<_Ep>&& __v)
        { return (_M_storage.template emplace<1>(std::move(__v)), *this); }

        // Emplace
        template <class... _Args>
        constexpr _Value&
        emplace(_Args&&... __args)
        { return _M_storage.template emplace<0>(std::forward<_Args>(__args)...); }

        template <class _Up, class... _Args>
        constexpr _Value&
        emplace(std::initializer_list<_Up> __il, _Args&&... __args)
        { return _M_storage.template emplace<0>(__il, std::forward<_Args>(__args)...); }

        // Swap
        void swap(expected& __rhs)
        { _M_storage.swap(__rhs._M_storage); }

        // Observers
        constexpr const _Value*
        operator->() const
        { return std::get_if<0>(_M_storage); }

        constexpr _Value*
        operator->()
        { return std::get_if<0>(_M_storage); }

        constexpr const _Value&
        operator*() const &
        { return std::get<0>(_M_storage); }

        constexpr _Value&
        operator*() &
        { return std::get<0>(_M_storage); }

        constexpr const _Value&&
        operator*() const &&
        { return std::get<0>(std::move(_M_storage)); }

        constexpr _Value&&
        operator*() &&
        { return std::get<0>(std::move(_M_storage)); }

        constexpr explicit
        operator bool() const
        { return has_value(); }

        constexpr bool
        has_value() const
        { return _M_storage.index() == 0; }

        constexpr const _Value&
        value() const &
        { return std::get<0>(_M_storage); }

        constexpr _Value&
        value() &
        { return std::get<0>(_M_storage); }

        constexpr const _Value&&
        value() const &&
        { return std::get<0>(std::move(_M_storage)); }

        constexpr _Value&& value() &&
        { return std::get<0>(std::move(_M_storage)); }

        constexpr const _Error&
        error() const &
        { return std::get<1>(_M_storage).value(); }

        constexpr _Error&
        error() &
        { return std::get<1>(_M_storage).value(); }

        constexpr const _Error&&
        error() const &&
        { return std::get<1>(std::move(_M_storage)).value(); }

        constexpr _Error&&
        error() &&
        { return std::get<1>(std::move(_M_storage)).value(); }

        template <class _Tp,
            class = std::enable_if_t<std::is_constructible<_Value, _Tp>::value>
        >
        constexpr _Value
        value_or(_Tp&& __v) const &
        { return has_value() ? value() : std::forward<_Tp>(__v); }

        template <class _Tp,
            class = std::enable_if_t<std::is_constructible<_Value, _Tp>::value>
        >
        constexpr _Value
        value_or(_Tp&& __v) &&
        { return has_value() ? std::move(value()) : std::forward<_Tp>(__v); }
    };

} // namespace std

