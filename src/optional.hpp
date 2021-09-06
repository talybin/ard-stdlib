// <optional> -*- C++ -*-
  
// Copyright (C) 2013-2020 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

//
// Adapted to Arduino devices (running C++14 with RTTI and exceptions disabled).
// Vladimir Talybin (2021)
//
// File version: 1.0.0
//

#pragma once

#if __cplusplus >= 201703L
#include <optional>
#else

#include "type_traits.hpp"
#include "utility.hpp"

#include <bits/enable_special_members.h>
#include <cstdlib>

namespace std
{
    template<typename _Tp>
    class optional;

    /// Tag type to disengage optional objects.
    struct nullopt_t { };

    // This class template manages construction/destruction of
    // the contained value for a std::optional.
    template <typename _Tp>
    struct _Optional_payload_base
    {
        using _Stored_type = remove_const_t<_Tp>;

        _Optional_payload_base() = default;
        ~_Optional_payload_base() = default;

        template <typename... _Args>
        constexpr
        _Optional_payload_base(in_place_t __tag, _Args&&... __args)
        : _M_payload(__tag, std::forward<_Args>(__args)...)
        , _M_engaged(true)
        { }

        template <typename _Up, typename... _Args>
        constexpr
        _Optional_payload_base(std::initializer_list<_Up> __il, _Args&&... __args)
        : _M_payload(__il, std::forward<_Args>(__args)...)
        , _M_engaged(true)
        { }

        // Constructor used by _Optional_base copy constructor when the
        // contained value is not trivially copy constructible.
        constexpr
        _Optional_payload_base(
            bool __engaged, const _Optional_payload_base& __other)
        {
            if (__other._M_engaged)
                this->_M_construct(__other._M_get());
        }

        // Constructor used by _Optional_base move constructor when the
        // contained value is not trivially move constructible.
        constexpr
        _Optional_payload_base(bool __engaged, _Optional_payload_base&& __other) {
            if (__other._M_engaged)
                this->_M_construct(std::move(__other._M_get()));
        }

        // Copy constructor is only used to when the contained value is
        // trivially copy constructible.
        _Optional_payload_base(const _Optional_payload_base&) = default;

        // Move constructor is only used to when the contained value is
        // trivially copy constructible.
        _Optional_payload_base(_Optional_payload_base&&) = default;

        _Optional_payload_base&
        operator=(const _Optional_payload_base&) = default;

        _Optional_payload_base&
        operator=(_Optional_payload_base&&) = default;

        // used to perform non-trivial copy assignment.
        constexpr void
        _M_copy_assign(const _Optional_payload_base& __other)
        {
            if (this->_M_engaged && __other._M_engaged)
                this->_M_get() = __other._M_get();
            else {
                if (__other._M_engaged)
                    this->_M_construct(__other._M_get());
                else
                    this->_M_reset();
            }
        }

        // used to perform non-trivial move assignment.
        constexpr void
        _M_move_assign(_Optional_payload_base&& __other)
        noexcept(__and_<is_nothrow_move_constructible<_Tp>,
            is_nothrow_move_assignable<_Tp>>::value)
        {
            if (this->_M_engaged && __other._M_engaged)
                this->_M_get() = std::move(__other._M_get());
            else {
                if (__other._M_engaged)
                    this->_M_construct(std::move(__other._M_get()));
                else
                    this->_M_reset();
            }
        }

        struct _Empty_byte { };

        template <typename _Up, bool = is_trivially_destructible<_Up>::value>
        union _Storage
        {
            constexpr _Storage() noexcept : _M_empty() { }

            template <typename... _Args>
            constexpr
            _Storage(in_place_t, _Args&&... __args)
            : _M_value(std::forward<_Args>(__args)...)
            { }

            template <typename _Vp, typename... _Args>
            constexpr
            _Storage(std::initializer_list<_Vp> __il, _Args&&... __args)
            : _M_value(__il, std::forward<_Args>(__args)...)
            { }

            _Empty_byte _M_empty;
            _Up _M_value;
        };

        template<typename _Up>
        union _Storage<_Up, false>
        {
            constexpr _Storage() noexcept : _M_empty() { }

            template <typename... _Args>
            constexpr
            _Storage(in_place_t, _Args&&... __args)
            : _M_value(std::forward<_Args>(__args)...)
            { }

            template<typename _Vp, typename... _Args>
            constexpr
            _Storage(std::initializer_list<_Vp> __il, _Args&&... __args)
            : _M_value(__il, std::forward<_Args>(__args)...)
            { }

            // User-provided destructor is needed when _Up has non-trivial dtor.
            ~_Storage() { }

            _Empty_byte _M_empty;
            _Up _M_value;
        };

        _Storage<_Stored_type> _M_payload;
        bool _M_engaged = false;

        template<typename... _Args>
        void
        _M_construct(_Args&&... __args)
        noexcept(is_nothrow_constructible<_Stored_type, _Args...>::value)
        {
            ::new ((void *) std::__addressof(this->_M_payload))
                _Stored_type(std::forward<_Args>(__args)...);
            this->_M_engaged = true;
        }

        constexpr void
        _M_destroy() noexcept
        {
            _M_engaged = false;
            _M_payload._M_value.~_Stored_type();
        }

        // The _M_get() operations have _M_engaged as a precondition.
        // They exist to access the contained value with the appropriate
        // const-qualification, because _M_payload has had the const removed.

        constexpr _Tp&
        _M_get() noexcept
        { return this->_M_payload._M_value; }

        constexpr const _Tp&
        _M_get() const noexcept
        { return this->_M_payload._M_value; }

        // _M_reset is a 'safe' operation with no precondition.
        constexpr void
        _M_reset() noexcept
        {
            if (this->_M_engaged)
                _M_destroy();
        }
    };

    // Class template that manages the payload for optionals.
    template <typename _Tp,
        bool /*_HasTrivialDestructor*/ =
            is_trivially_destructible<_Tp>::value,
        bool /*_HasTrivialCopy */ =
            is_trivially_copy_assignable<_Tp>::value
            && is_trivially_copy_constructible<_Tp>::value,
        bool /*_HasTrivialMove */ =
            is_trivially_move_assignable<_Tp>::value
            && is_trivially_move_constructible<_Tp>::value>
    struct _Optional_payload;

    // Payload for potentially-constexpr optionals (trivial copy/move/destroy).
    template <typename _Tp>
    struct _Optional_payload<_Tp, true, true, true>
    : _Optional_payload_base<_Tp>
    {
        using _Optional_payload_base<_Tp>::_Optional_payload_base;

        _Optional_payload() = default;
    };

    // Payload for optionals with non-trivial copy construction/assignment.
    template <typename _Tp>
    struct _Optional_payload<_Tp, true, false, true>
    : _Optional_payload_base<_Tp>
    {
        using _Optional_payload_base<_Tp>::_Optional_payload_base;

        _Optional_payload() = default;
        ~_Optional_payload() = default;
        _Optional_payload(const _Optional_payload&) = default;
        _Optional_payload(_Optional_payload&&) = default;
        _Optional_payload& operator=(_Optional_payload&&) = default;

        // Non-trivial copy assignment.
        constexpr
        _Optional_payload&
        operator=(const _Optional_payload& __other)
        {
            this->_M_copy_assign(__other);
            return *this;
        }
    };

    // Payload for optionals with non-trivial move construction/assignment.
    template <typename _Tp>
    struct _Optional_payload<_Tp, true, true, false>
    : _Optional_payload_base<_Tp>
    {
        using _Optional_payload_base<_Tp>::_Optional_payload_base;

        _Optional_payload() = default;
        ~_Optional_payload() = default;
        _Optional_payload(const _Optional_payload&) = default;
        _Optional_payload(_Optional_payload&&) = default;
        _Optional_payload& operator=(const _Optional_payload&) = default;

        // Non-trivial move assignment.
        constexpr
        _Optional_payload&
        operator=(_Optional_payload&& __other)
        noexcept(__and_<is_nothrow_move_constructible<_Tp>,
            is_nothrow_move_assignable<_Tp>>::value)
        {
            this->_M_move_assign(std::move(__other));
            return *this;
        }
    };

    // Payload for optionals with non-trivial copy and move assignment.
    template <typename _Tp>
    struct _Optional_payload<_Tp, true, false, false>
    : _Optional_payload_base<_Tp>
    {
        using _Optional_payload_base<_Tp>::_Optional_payload_base;

        _Optional_payload() = default;
        ~_Optional_payload() = default;
        _Optional_payload(const _Optional_payload&) = default;
        _Optional_payload(_Optional_payload&&) = default;

        // Non-trivial copy assignment.
        constexpr
        _Optional_payload&
        operator=(const _Optional_payload& __other)
        {
            this->_M_copy_assign(__other);
            return *this;
        }

        // Non-trivial move assignment.
        constexpr
        _Optional_payload&
        operator=(_Optional_payload&& __other)
        noexcept(__and_<is_nothrow_move_constructible<_Tp>,
            is_nothrow_move_assignable<_Tp>>::value)
        {
            this->_M_move_assign(std::move(__other));
            return *this;
        }
    };

    // Payload for optionals with non-trivial destructors.
    template <typename _Tp, bool _Copy, bool _Move>
    struct _Optional_payload<_Tp, false, _Copy, _Move>
    : _Optional_payload<_Tp, true, false, false>
    {
        // Base class implements all the constructors and assignment operators:
        using _Optional_payload<_Tp, true, false, false>::_Optional_payload;
        _Optional_payload() = default;
        _Optional_payload(const _Optional_payload&) = default;
        _Optional_payload(_Optional_payload&&) = default;
        _Optional_payload& operator=(const _Optional_payload&) = default;
        _Optional_payload& operator=(_Optional_payload&&) = default;

        // Destructor needs to destroy the contained value:
        ~_Optional_payload() { this->_M_reset(); }
    };

    // Common base class for _Optional_base<T> to avoid repeating these
    // member functions in each specialization.
    template <typename _Tp, typename _Dp>
    class _Optional_base_impl
    {
    protected:
        using _Stored_type = remove_const_t<_Tp>;

        // The _M_construct operation has !_M_engaged as a precondition
        // while _M_destruct has _M_engaged as a precondition.
        template <typename... _Args>
        void
        _M_construct(_Args&&... __args)
        noexcept(is_nothrow_constructible<_Stored_type, _Args...>::value)
        {
            ::new
                (std::__addressof(static_cast<_Dp*>(this)->_M_payload._M_payload))
                _Stored_type(std::forward<_Args>(__args)...);
            static_cast<_Dp*>(this)->_M_payload._M_engaged = true;
        }

        void
        _M_destruct() noexcept
        { static_cast<_Dp*>(this)->_M_payload._M_destroy(); }

        // _M_reset is a 'safe' operation with no precondition.
        constexpr void
        _M_reset() noexcept
        { static_cast<_Dp*>(this)->_M_payload._M_reset(); }

        constexpr bool _M_is_engaged() const noexcept
        { return static_cast<const _Dp*>(this)->_M_payload._M_engaged; }

        // The _M_get operations have _M_engaged as a precondition.
        constexpr _Tp&
        _M_get() noexcept
        {
            __glibcxx_assert(this->_M_is_engaged());
            return static_cast<_Dp*>(this)->_M_payload._M_get();
        }

        constexpr const _Tp&
        _M_get() const noexcept
        {
            __glibcxx_assert(this->_M_is_engaged());
            return static_cast<const _Dp*>(this)->_M_payload._M_get();
        }
    };

    /**
    * @brief Class template that provides copy/move constructors of optional.
    *
    * Such a separate base class template is necessary in order to
    * conditionally make copy/move constructors trivial.
    *
    * When the contained value is trivially copy/move constructible,
    * the copy/move constructors of _Optional_base will invoke the
    * trivial copy/move constructor of _Optional_payload. Otherwise,
    * they will invoke _Optional_payload(bool, const _Optional_payload&)
    * or _Optional_payload(bool, _Optional_payload&&) to initialize
    * the contained value, if copying/moving an engaged optional.
    *
    * Whether the other special members are trivial is determined by the
    * _Optional_payload<_Tp> specialization used for the _M_payload member.
    *
    * @see optional, _Enable_special_members
    */
    template <typename _Tp,
        bool = is_trivially_copy_constructible<_Tp>::value,
        bool = is_trivially_move_constructible<_Tp>::value>
    struct _Optional_base
    : _Optional_base_impl<_Tp, _Optional_base<_Tp>>
    {
        // Constructors for disengaged optionals.
        constexpr _Optional_base() = default;

        // Constructors for engaged optionals.
        template <typename... _Args,
            enable_if_t<is_constructible<_Tp, _Args&&...>::value, bool> = false>
        constexpr explicit _Optional_base(in_place_t, _Args&&... __args)
        : _M_payload(in_place_t{}
        , std::forward<_Args>(__args)...) { }

        template <typename _Up, typename... _Args,
            enable_if_t<is_constructible<
                _Tp, initializer_list<_Up>&, _Args&&...>::value,
                bool> = false>
        constexpr explicit _Optional_base(in_place_t,
                                          initializer_list<_Up> __il,
                                          _Args&&... __args)
        : _M_payload(in_place_t{}, __il, std::forward<_Args>(__args)...)
        { }

        // Copy and move constructors.
        constexpr _Optional_base(const _Optional_base& __other)
        : _M_payload(__other._M_payload._M_engaged, __other._M_payload)
        { }

        constexpr _Optional_base(_Optional_base&& __other)
        noexcept(is_nothrow_move_constructible<_Tp>::value)
        : _M_payload(__other._M_payload._M_engaged, std::move(__other._M_payload))
        { }

        // Assignment operators.
        _Optional_base& operator=(const _Optional_base&) = default;
        _Optional_base& operator=(_Optional_base&&) = default;

        _Optional_payload<_Tp> _M_payload;
    };

    template <typename _Tp>
    struct _Optional_base<_Tp, false, true>
    : _Optional_base_impl<_Tp, _Optional_base<_Tp>>
    {
        // Constructors for disengaged optionals.
        constexpr _Optional_base() = default;

        // Constructors for engaged optionals.
        template <typename... _Args,
            enable_if_t<is_constructible<_Tp, _Args&&...>::value, bool> = false>
        constexpr explicit _Optional_base(in_place_t, _Args&&... __args)
        : _M_payload(in_place_t{}, std::forward<_Args>(__args)...)
        { }

        template <typename _Up, typename... _Args,
            enable_if_t<is_constructible<
                _Tp, initializer_list<_Up>&, _Args&&...>::value,
                bool> = false>
        constexpr explicit _Optional_base(in_place_t,
                                          initializer_list<_Up> __il,
                                          _Args&&... __args)
        : _M_payload(in_place_t{}, __il, std::forward<_Args>(__args)...)
        { }

        // Copy and move constructors.
        constexpr _Optional_base(const _Optional_base& __other)
        : _M_payload(__other._M_payload._M_engaged, __other._M_payload)
        { }

        constexpr _Optional_base(_Optional_base&& __other) = default;

        // Assignment operators.
        _Optional_base& operator=(const _Optional_base&) = default;
        _Optional_base& operator=(_Optional_base&&) = default;

        _Optional_payload<_Tp> _M_payload;
    };

    template <typename _Tp>
    struct _Optional_base<_Tp, true, false>
    : _Optional_base_impl<_Tp, _Optional_base<_Tp>>
    {
        // Constructors for disengaged optionals.
        constexpr _Optional_base() = default;

        // Constructors for engaged optionals.
        template <typename... _Args,
            enable_if_t<is_constructible<_Tp, _Args&&...>::value, bool> = false>
        constexpr explicit _Optional_base(in_place_t, _Args&&... __args)
        : _M_payload(in_place_t{}, std::forward<_Args>(__args)...)
        { }

        template <typename _Up, typename... _Args,
            enable_if_t<is_constructible<
                _Tp, initializer_list<_Up>&, _Args&&...>::value,
                bool> = false>
        constexpr explicit _Optional_base(in_place_t,
                                          initializer_list<_Up> __il,
                                          _Args&&... __args)
        : _M_payload(in_place_t{}, __il, std::forward<_Args>(__args)...)
        { }

        // Copy and move constructors.
        constexpr _Optional_base(const _Optional_base& __other) = default;

        constexpr _Optional_base(_Optional_base&& __other)
        noexcept(is_nothrow_move_constructible<_Tp>::value)
        : _M_payload(__other._M_payload._M_engaged, std::move(__other._M_payload))
        { }

        // Assignment operators.
        _Optional_base& operator=(const _Optional_base&) = default;
        _Optional_base& operator=(_Optional_base&&) = default;

        _Optional_payload<_Tp> _M_payload;
    };

    template <typename _Tp>
    struct _Optional_base<_Tp, true, true>
    : _Optional_base_impl<_Tp, _Optional_base<_Tp>>
    {
        // Constructors for disengaged optionals.
        constexpr _Optional_base() = default;

        // Constructors for engaged optionals.
        template <typename... _Args,
            enable_if_t<is_constructible<_Tp, _Args&&...>::value, bool> = false>
        constexpr explicit _Optional_base(in_place_t, _Args&&... __args)
        : _M_payload(in_place_t{}, std::forward<_Args>(__args)...)
        { }

        template <typename _Up, typename... _Args,
            enable_if_t<is_constructible<
                _Tp, initializer_list<_Up>&, _Args&&...>::value,
                bool> = false>
        constexpr explicit _Optional_base(in_place_t,
                                          initializer_list<_Up> __il,
                                          _Args&&... __args)
        : _M_payload(in_place_t{}, __il, std::forward<_Args>(__args)...)
        { }

        // Copy and move constructors.
        constexpr _Optional_base(const _Optional_base& __other) = default;
        constexpr _Optional_base(_Optional_base&& __other) = default;

        // Assignment operators.
        _Optional_base& operator=(const _Optional_base&) = default;
        _Optional_base& operator=(_Optional_base&&) = default;

        _Optional_payload<_Tp> _M_payload;
    };

  template <typename _Tp>
  class optional;

  template <typename _Tp, typename _Up>
    using __converts_from_optional =
      __or_<is_constructible<_Tp, const optional<_Up>&>,
        is_constructible<_Tp, optional<_Up>&>,
        is_constructible<_Tp, const optional<_Up>&&>,
        is_constructible<_Tp, optional<_Up>&&>,
        is_convertible<const optional<_Up>&, _Tp>,
        is_convertible<optional<_Up>&, _Tp>,
        is_convertible<const optional<_Up>&&, _Tp>,
        is_convertible<optional<_Up>&&, _Tp>>;

    template <typename _Tp, typename _Up>
    using __assigns_from_optional =
      __or_<is_assignable<_Tp&, const optional<_Up>&>,
        is_assignable<_Tp&, optional<_Up>&>,
        is_assignable<_Tp&, const optional<_Up>&&>,
        is_assignable<_Tp&, optional<_Up>&&>>;

    /**
      * @brief Class template for optional values.
      */
    template <typename _Tp>
    class optional
    : private _Optional_base<_Tp>,
      private _Enable_copy_move<
        // Copy constructor.
        is_copy_constructible<_Tp>::value,
        // Copy assignment.
        __and_<is_copy_constructible<_Tp>, is_copy_assignable<_Tp>>::value,
        // Move constructor.
        is_move_constructible<_Tp>::value,
        // Move assignment.
        __and_<is_move_constructible<_Tp>, is_move_assignable<_Tp>>::value,
        // Unique tag type.
        optional<_Tp>>
    {
        static_assert(!is_same<remove_cv_t<_Tp>, nullopt_t>::value);
        static_assert(!is_same<remove_cv_t<_Tp>, in_place_t>::value);
        static_assert(!is_reference<_Tp>::value);

    private:
        using _Base = _Optional_base<_Tp>;

        // SFINAE helpers
        template <typename _Up>
        using __not_self = __not_<is_same<optional, __remove_cvref_t<_Up>>>;
        template <typename _Up>
        using __not_tag = __not_<is_same<in_place_t, __remove_cvref_t<_Up>>>;
        template <typename... _Cond>
        using _Requires = enable_if_t<__and_<_Cond...>::value, bool>;

    public:
        using value_type = _Tp;

        constexpr optional() = default;

        constexpr optional(nullopt_t) noexcept { }

        // Converting constructors for engaged optionals.
        template <typename _Up = _Tp,
            _Requires<__not_self<_Up>, __not_tag<_Up>,
                is_constructible<_Tp, _Up&&>,
                is_convertible<_Up&&, _Tp>> = true>
        constexpr
        optional(_Up&& __t)
        : _Base(std::in_place_t{}, std::forward<_Up>(__t)) { }

        template <typename _Up = _Tp,
            _Requires<__not_self<_Up>, __not_tag<_Up>,
                is_constructible<_Tp, _Up&&>,
                __not_<is_convertible<_Up&&, _Tp>>> = false>
        explicit constexpr
        optional(_Up&& __t)
        : _Base(std::in_place_t{}, std::forward<_Up>(__t)) { }

        template <typename _Up,
            _Requires<__not_<is_same<_Tp, _Up>>,
                is_constructible<_Tp, const _Up&>,
                is_convertible<const _Up&, _Tp>,
                __not_<__converts_from_optional<_Tp, _Up>>> = true>
        constexpr
        optional(const optional<_Up>& __t)
        {
            if (__t)
                emplace(*__t);
        }

        template <typename _Up,
            _Requires<__not_<is_same<_Tp, _Up>>,
                is_constructible<_Tp, const _Up&>,
                __not_<is_convertible<const _Up&, _Tp>>,
                __not_<__converts_from_optional<_Tp, _Up>>> = false>
        explicit constexpr
        optional(const optional<_Up>& __t)
        {
            if (__t)
                emplace(*__t);
        }

        template <typename _Up,
            _Requires<__not_<is_same<_Tp, _Up>>,
                is_constructible<_Tp, _Up&&>,
                is_convertible<_Up&&, _Tp>,
                __not_<__converts_from_optional<_Tp, _Up>>> = true>
        constexpr
        optional(optional<_Up>&& __t)
        {
            if (__t)
                emplace(std::move(*__t));
        }

        template <typename _Up,
            _Requires<__not_<is_same<_Tp, _Up>>,
                is_constructible<_Tp, _Up&&>,
                __not_<is_convertible<_Up&&, _Tp>>,
                __not_<__converts_from_optional<_Tp, _Up>>> = false>
        explicit constexpr
        optional(optional<_Up>&& __t)
        {
            if (__t)
                emplace(std::move(*__t));
        }

        template <typename... _Args,
            _Requires<is_constructible<_Tp, _Args&&...>> = false>
        explicit constexpr
        optional(in_place_t, _Args&&... __args)
        : _Base(std::in_place_t{}, std::forward<_Args>(__args)...) { }

        template <typename _Up, typename... _Args,
            _Requires<is_constructible<_Tp,
                      initializer_list<_Up>&,
                      _Args&&...>> = false>
        explicit constexpr
        optional(in_place_t, initializer_list<_Up> __il, _Args&&... __args)
        : _Base(std::in_place_t{}, __il, std::forward<_Args>(__args)...) { }

        // Assignment operators.
        optional&
        operator=(nullopt_t) noexcept
        {
            this->_M_reset();
            return *this;
        }

        template <typename _Up = _Tp>
        enable_if_t<__and_<
            __not_self<_Up>, __not_<__and_<is_scalar<_Tp>, is_same<_Tp, decay_t<_Up>>>>,
            is_constructible<_Tp, _Up>,
            is_assignable<_Tp&, _Up>>::value,
            optional&>
        operator=(_Up&& __u)
        {
            if (this->_M_is_engaged())
                this->_M_get() = std::forward<_Up>(__u);
            else
                this->_M_construct(std::forward<_Up>(__u));

            return *this;
        }

        template <typename _Up>
        enable_if_t<__and_<__not_<is_same<_Tp, _Up>>,
            is_constructible<_Tp, const _Up&>,
            is_assignable<_Tp&, _Up>,
            __not_<__converts_from_optional<_Tp, _Up>>,
            __not_<__assigns_from_optional<_Tp, _Up>>>::value,
            optional&>
        operator=(const optional<_Up>& __u)
        {
            if (__u) {
                if (this->_M_is_engaged())
                    this->_M_get() = *__u;
                else
                    this->_M_construct(*__u);
            }
            else
                this->_M_reset();
            return *this;
        }

        template <typename _Up>
        enable_if_t<__and_<__not_<is_same<_Tp, _Up>>,
            is_constructible<_Tp, _Up>,
            is_assignable<_Tp&, _Up>,
            __not_<__converts_from_optional<_Tp, _Up>>,
            __not_<__assigns_from_optional<_Tp, _Up>>>::value,
            optional&>
        operator=(optional<_Up>&& __u)
        {
            if (__u) {
                if (this->_M_is_engaged())
                    this->_M_get() = std::move(*__u);
                else
                    this->_M_construct(std::move(*__u));
            }
            else
                this->_M_reset();
            return *this;
        }

        template <typename... _Args>
        enable_if_t<is_constructible<_Tp, _Args&&...>::value, _Tp&>
        emplace(_Args&&... __args)
        {
            this->_M_reset();
            this->_M_construct(std::forward<_Args>(__args)...);
            return this->_M_get();
        }

        template <typename _Up, typename... _Args>
        enable_if_t<is_constructible<_Tp, initializer_list<_Up>&,
                       _Args&&...>::value, _Tp&>
        emplace(initializer_list<_Up> __il, _Args&&... __args)
        {
            this->_M_reset();
            this->_M_construct(__il, std::forward<_Args>(__args)...);
            return this->_M_get();
        }

        // Destructor is implicit, implemented in _Optional_base.

        // Swap.
        void
        swap(optional& __other)
        noexcept(is_nothrow_move_constructible<_Tp>::value
            && is_nothrow_swappable<_Tp>::value)
        {
            using std::swap;

            if (this->_M_is_engaged() && __other._M_is_engaged())
                swap(this->_M_get(), __other._M_get());
            else if (this->_M_is_engaged())
            {
                __other._M_construct(std::move(this->_M_get()));
                this->_M_destruct();
            }
            else if (__other._M_is_engaged())
            {
                this->_M_construct(std::move(__other._M_get()));
                __other._M_destruct();
            }
        }

        // Observers.
        constexpr const _Tp*
        operator->() const
        { return std::__addressof(this->_M_get()); }

        constexpr _Tp*
        operator->()
        { return std::__addressof(this->_M_get()); }

        constexpr const _Tp&
        operator*() const&
        { return this->_M_get(); }

        constexpr _Tp&
        operator*()&
        { return this->_M_get(); }

        constexpr _Tp&&
        operator*()&&
        { return std::move(this->_M_get()); }

        constexpr const _Tp&&
        operator*() const&&
        { return std::move(this->_M_get()); }

        constexpr explicit operator bool() const noexcept
        { return this->_M_is_engaged(); }

        constexpr bool has_value() const noexcept
        { return this->_M_is_engaged(); }

        constexpr const _Tp&
        value() const&
        {
            if (this->_M_is_engaged())
                return this->_M_get();
            std::abort();
        }

        constexpr _Tp&
        value()&
        {
            if (this->_M_is_engaged())
                return this->_M_get();
            std::abort();
        }

        constexpr _Tp&&
        value()&&
        {
            if (this->_M_is_engaged())
                return std::move(this->_M_get());
            std::abort();
        }

        constexpr const _Tp&&
        value() const&&
        {
            if (this->_M_is_engaged())
                return std::move(this->_M_get());
            std::abort();
        }

        template <typename _Up>
        constexpr _Tp
        value_or(_Up&& __u) const&
        {
            static_assert(is_copy_constructible<_Tp>::value);
            static_assert(is_convertible<_Up&&, _Tp>::value);

            return this->_M_is_engaged()
                ? this->_M_get() : static_cast<_Tp>(std::forward<_Up>(__u));
        }

        template <typename _Up>
        constexpr _Tp
        value_or(_Up&& __u) &&
        {
            static_assert(is_move_constructible<_Tp>::value);
            static_assert(is_convertible<_Up&&, _Tp>::value);

            return this->_M_is_engaged()
                ? std::move(this->_M_get())
                : static_cast<_Tp>(std::forward<_Up>(__u));
        }

        void reset() noexcept { this->_M_reset(); }
    };

    template <typename _Tp>
    using __optional_relop_t =
        enable_if_t<is_convertible<_Tp, bool>::value, bool>;

    template <typename _Tp, typename _Up>
    using __optional_eq_t = __optional_relop_t<
        decltype(std::declval<const _Tp&>() == std::declval<const _Up&>())>;

    template <typename _Tp, typename _Up>
    using __optional_ne_t = __optional_relop_t<
        decltype(std::declval<const _Tp&>() != std::declval<const _Up&>())>;

    template <typename _Tp, typename _Up>
    using __optional_lt_t = __optional_relop_t<
        decltype(std::declval<const _Tp&>() < std::declval<const _Up&>())>;

    template <typename _Tp, typename _Up>
    using __optional_gt_t = __optional_relop_t<
        decltype(std::declval<const _Tp&>() > std::declval<const _Up&>())>;

    template <typename _Tp, typename _Up>
    using __optional_le_t = __optional_relop_t<
        decltype(std::declval<const _Tp&>() <= std::declval<const _Up&>())>;

    template<typename _Tp, typename _Up>
    using __optional_ge_t = __optional_relop_t<
        decltype(std::declval<const _Tp&>() >= std::declval<const _Up&>())>;

    // Comparisons between optional values.
    template <typename _Tp, typename _Up>
    constexpr auto
    operator==(const optional<_Tp>& __lhs, const optional<_Up>& __rhs)
        -> __optional_eq_t<_Tp, _Up>
    {
        return static_cast<bool>(__lhs) == static_cast<bool>(__rhs)
            && (!__lhs || *__lhs == *__rhs);
    }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator!=(const optional<_Tp>& __lhs, const optional<_Up>& __rhs)
        -> __optional_ne_t<_Tp, _Up>
    {
        return static_cast<bool>(__lhs) != static_cast<bool>(__rhs)
            || (static_cast<bool>(__lhs) && *__lhs != *__rhs);
    }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator<(const optional<_Tp>& __lhs, const optional<_Up>& __rhs)
        -> __optional_lt_t<_Tp, _Up>
    {
        return static_cast<bool>(__rhs) && (!__lhs || *__lhs < *__rhs);
    }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator>(const optional<_Tp>& __lhs, const optional<_Up>& __rhs)
        -> __optional_gt_t<_Tp, _Up>
    {
        return static_cast<bool>(__lhs) && (!__rhs || *__lhs > *__rhs);
    }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator<=(const optional<_Tp>& __lhs, const optional<_Up>& __rhs)
        -> __optional_le_t<_Tp, _Up>
    {
        return !__lhs || (static_cast<bool>(__rhs) && *__lhs <= *__rhs);
    }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator>=(const optional<_Tp>& __lhs, const optional<_Up>& __rhs)
        -> __optional_ge_t<_Tp, _Up>
    {
        return !__rhs || (static_cast<bool>(__lhs) && *__lhs >= *__rhs);
    }

    // Comparisons with nullopt.
    template <typename _Tp>
    constexpr bool
    operator==(const optional<_Tp>& __lhs, nullopt_t) noexcept
    { return !__lhs; }

    template <typename _Tp>
    constexpr bool
    operator==(nullopt_t, const optional<_Tp>& __rhs) noexcept
    { return !__rhs; }

    template <typename _Tp>
    constexpr bool
    operator!=(const optional<_Tp>& __lhs, nullopt_t) noexcept
    { return static_cast<bool>(__lhs); }

    template <typename _Tp>
    constexpr bool
    operator!=(nullopt_t, const optional<_Tp>& __rhs) noexcept
    { return static_cast<bool>(__rhs); }

    template <typename _Tp>
    constexpr bool
    operator<(const optional<_Tp>& /* __lhs */, nullopt_t) noexcept
    { return false; }

    template <typename _Tp>
    constexpr bool
    operator<(nullopt_t, const optional<_Tp>& __rhs) noexcept
    { return static_cast<bool>(__rhs); }

    template <typename _Tp>
    constexpr bool
    operator>(const optional<_Tp>& __lhs, nullopt_t) noexcept
    { return static_cast<bool>(__lhs); }

    template <typename _Tp>
    constexpr bool
    operator>(nullopt_t, const optional<_Tp>& /* __rhs */) noexcept
    { return false; }

    template <typename _Tp>
    constexpr bool
    operator<=(const optional<_Tp>& __lhs, nullopt_t) noexcept
    { return !__lhs; }

    template <typename _Tp>
    constexpr bool
    operator<=(nullopt_t, const optional<_Tp>& /* __rhs */) noexcept
    { return true; }

    template <typename _Tp>
    constexpr bool
    operator>=(const optional<_Tp>& /* __lhs */, nullopt_t) noexcept
    { return true; }

    template<typename _Tp>
    constexpr bool
    operator>=(nullopt_t, const optional<_Tp>& __rhs) noexcept
    { return !__rhs; }

    // Comparisons with value type.
    template <typename _Tp, typename _Up>
    constexpr auto
    operator==(const optional<_Tp>& __lhs, const _Up& __rhs)
        -> __optional_eq_t<_Tp, _Up>
    { return __lhs && *__lhs == __rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator==(const _Up& __lhs, const optional<_Tp>& __rhs)
        -> __optional_eq_t<_Up, _Tp>
    { return __rhs && __lhs == *__rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator!=(const optional<_Tp>& __lhs, const _Up& __rhs)
        -> __optional_ne_t<_Tp, _Up>
    { return !__lhs || *__lhs != __rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator!=(const _Up& __lhs, const optional<_Tp>& __rhs)
        -> __optional_ne_t<_Up, _Tp>
    { return !__rhs || __lhs != *__rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator<(const optional<_Tp>& __lhs, const _Up& __rhs)
        -> __optional_lt_t<_Tp, _Up>
    { return !__lhs || *__lhs < __rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator<(const _Up& __lhs, const optional<_Tp>& __rhs)
        -> __optional_lt_t<_Up, _Tp>
    { return __rhs && __lhs < *__rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator>(const optional<_Tp>& __lhs, const _Up& __rhs)
        -> __optional_gt_t<_Tp, _Up>
    { return __lhs && *__lhs > __rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator>(const _Up& __lhs, const optional<_Tp>& __rhs)
        -> __optional_gt_t<_Up, _Tp>
    { return !__rhs || __lhs > *__rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator<=(const optional<_Tp>& __lhs, const _Up& __rhs)
        -> __optional_le_t<_Tp, _Up>
    { return !__lhs || *__lhs <= __rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator<=(const _Up& __lhs, const optional<_Tp>& __rhs)
        -> __optional_le_t<_Up, _Tp>
    { return __rhs && __lhs <= *__rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator>=(const optional<_Tp>& __lhs, const _Up& __rhs)
        -> __optional_ge_t<_Tp, _Up>
    { return __lhs && *__lhs >= __rhs; }

    template <typename _Tp, typename _Up>
    constexpr auto
    operator>=(const _Up& __lhs, const optional<_Tp>& __rhs)
        -> __optional_ge_t<_Up, _Tp>
    { return !__rhs || __lhs >= *__rhs; }

    // Swap and creation functions.

    // _GLIBCXX_RESOLVE_LIB_DEFECTS
    // 2748. swappable traits for optionals
    template <typename _Tp>
    inline enable_if_t<is_move_constructible<_Tp>::value && is_swappable<_Tp>::value>
    swap(optional<_Tp>& __lhs, optional<_Tp>& __rhs)
    noexcept(noexcept(__lhs.swap(__rhs)))
    { __lhs.swap(__rhs); }

    template <typename _Tp>
    enable_if_t<!(is_move_constructible<_Tp>::value && is_swappable<_Tp>::value)>
    swap(optional<_Tp>&, optional<_Tp>&) = delete;

    template <typename _Tp>
    constexpr optional<decay_t<_Tp>>
    make_optional(_Tp&& __t)
    { return optional<decay_t<_Tp>> { std::forward<_Tp>(__t) }; }

    template <typename _Tp, typename ..._Args>
    constexpr optional<_Tp>
    make_optional(_Args&&... __args)
    { return optional<_Tp> { in_place_t{}, std::forward<_Args>(__args)... }; }

    template <typename _Tp, typename _Up, typename ..._Args>
    constexpr optional<_Tp>
    make_optional(initializer_list<_Up> __il, _Args&&... __args)
    { return optional<_Tp> { in_place_t{}, __il, std::forward<_Args>(__args)... }; }

} // namespace std

#endif // __cplusplus < 201703L

