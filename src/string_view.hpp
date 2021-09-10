// Components for manipulating non-owning sequences of characters -*- C++ -*-

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
// Note! In case of invalid operation (like calling front() on empty string) the
// program will abort (call std::abort()).
//
// File version: 1.0.0
//
// Features:
//  - Includes starts_with(), ends_with() from C++20 and contains() from C++23
//

#pragma once

#if __cplusplus >= 201703L
#include <string_view>
#else

#include <bits/char_traits.h>
#include <iterator>
#include "type_traits.hpp"
#include "exception.hpp"

namespace std
{
    template <class _CharT, class _Traits = std::char_traits<_CharT>>
    struct basic_string_view
    {
        static_assert(!std::is_array<_CharT>::value);
        static_assert(std::is_trivial<_CharT>::value && std::is_standard_layout<_CharT>::value);
        static_assert(std::is_same<_CharT, typename _Traits::char_type>::value);

        // types
        using traits_type       = _Traits;
        using value_type        = _CharT;
        using pointer           = value_type*;
        using const_pointer     = const value_type*;
        using reference         = value_type&;
        using const_reference   = const value_type&;
        using const_iterator    = const value_type*;
        using iterator          = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator  = const_reverse_iterator;
        using size_type         = size_t;
        using difference_type   = ptrdiff_t;

        static constexpr size_type npos = size_type(-1);

        // [string.view.cons], construction and assignment

        constexpr
        basic_string_view() noexcept
        : _M_len{0}, _M_str{nullptr}
        { }

        constexpr basic_string_view(const basic_string_view&) noexcept = default;

        __attribute__((__nonnull__)) constexpr
        basic_string_view(const _CharT* __str) noexcept
        : _M_len{traits_type::length(__str)},
        _M_str{__str}
        { }

        constexpr
        basic_string_view(const _CharT* __str, size_type __len) noexcept
        : _M_len{__len}, _M_str{__str}
        { }

        constexpr basic_string_view&
        operator=(const basic_string_view&) noexcept = default;

        // [string.view.iterators], iterator support

        constexpr const_iterator
        begin() const noexcept
        { return this->_M_str; }

        constexpr const_iterator
        end() const noexcept
        { return this->_M_str + this->_M_len; }

        constexpr const_iterator
        cbegin() const noexcept
        { return this->_M_str; }

        constexpr const_iterator
        cend() const noexcept
        { return this->_M_str + this->_M_len; }

        constexpr const_reverse_iterator
        rbegin() const noexcept
        { return const_reverse_iterator(this->end()); }

        constexpr const_reverse_iterator
        rend() const noexcept
        { return const_reverse_iterator(this->begin()); }

        constexpr const_reverse_iterator
        crbegin() const noexcept
        { return const_reverse_iterator(this->end()); }

        constexpr const_reverse_iterator
        crend() const noexcept
        { return const_reverse_iterator(this->begin()); }

        // [string.view.capacity], capacity

        constexpr size_type
        size() const noexcept
        { return this->_M_len; }

        constexpr size_type
        length() const noexcept
        { return _M_len; }

        constexpr size_type
        max_size() const noexcept {
            return (npos - sizeof(size_type) - sizeof(void*)) /
                sizeof(value_type) / 4;
        }

        constexpr bool
        empty() const noexcept
        { return this->_M_len == 0; }

        // [string.view.access], element access

        constexpr const_reference
        operator[](size_type __pos) const {
            if (__pos < _M_len)
                return *(this->_M_str + __pos);
            ard::throw_exception(ard::error() <<
                "basic_string_view::operator[]: __pos "
                "(which is " << __pos << ") >= size() "
                "(which is " << this->size() << ')'
            );
        }

        constexpr const_reference
        at(size_type __pos) const {
            if (__pos < _M_len)
                return *(this->_M_str + __pos);
            ard::throw_exception(ard::error() <<
                "basic_string_view::at: __pos "
                "(which is " << __pos << ") >= size() "
                "(which is " << this->size() << ')'
            );
        }

        constexpr const_reference
        front() const {
            if (this->_M_len > 0)
                return *this->_M_str;
            ard::throw_exception(
                ard::error("basic_string_view::front: string is empty"));
        }

        constexpr const_reference
        back() const {
            if (this->_M_len > 0)
                return *(this->_M_str + this->_M_len - 1);
            ard::throw_exception(
                ard::error("basic_string_view::back: string is empty"));
        }

        constexpr const_pointer
        data() const noexcept
        { return this->_M_str; }

        // [string.view.modifiers], modifiers:

        // vlta: _M_len >= __n makes the string empty
        constexpr void
        remove_prefix(size_type __n) noexcept {
            __n = std::min(__n, _M_len);
            this->_M_str += __n;
            this->_M_len -= __n;
        }

        constexpr void
        remove_suffix(size_type __n) noexcept
        { this->_M_len -= __n; }

        constexpr void
        swap(basic_string_view& __sv) noexcept {
            auto __tmp = *this;
            *this = __sv;
            __sv = __tmp;
        }

        // [string.view.ops], string operations:

        // vlta: silently do nothing if __pos out of range, return 0 in this case
        constexpr size_type
        copy(_CharT* __str, size_type __n, size_type __pos = 0) const noexcept
        {
            if (__pos < _M_len) {
                const size_type __rlen = std::min(__n, _M_len - __pos);
                // _GLIBCXX_RESOLVE_LIB_DEFECTS
                // 2777. basic_string_view::copy should use char_traits::copy
                traits_type::copy(__str, data() + __pos, __rlen);
                return __rlen;
            }
            return 0;
        }

        // vlta: if __pos out of range return empty string
        constexpr basic_string_view
        substr(size_type __pos = 0, size_type __n = npos) const noexcept
        {
            if (__pos < _M_len) {
                const size_type __rlen = std::min(__n, _M_len - __pos);
                return basic_string_view{_M_str + __pos, __rlen};
            }
            return {};
        }

        constexpr int
        compare(basic_string_view __str) const noexcept
        {
            const size_type __rlen = std::min(this->_M_len, __str._M_len);
            int __ret = traits_type::compare(this->_M_str, __str._M_str, __rlen);
            if (__ret == 0)
                __ret = _S_compare(this->_M_len, __str._M_len);
            return __ret;
        }

        constexpr int
        compare(size_type __pos1, size_type __n1, basic_string_view __str) const
        { return this->substr(__pos1, __n1).compare(__str); }

        constexpr int
        compare(size_type __pos1, size_type __n1,
            basic_string_view __str, size_type __pos2, size_type __n2) const
        {
            return this->substr(__pos1, __n1).compare(__str.substr(__pos2, __n2));
        }

        __attribute__((__nonnull__)) constexpr int
        compare(const _CharT* __str) const noexcept
        { return this->compare(basic_string_view{__str}); }

        __attribute__((__nonnull__)) constexpr int
        compare(size_type __pos1, size_type __n1, const _CharT* __str) const
        { return this->substr(__pos1, __n1).compare(basic_string_view{__str}); }

        constexpr int
        compare(size_type __pos1, size_type __n1,
            const _CharT* __str, size_type __n2) const noexcept
        { return this->substr(__pos1, __n1).compare(basic_string_view(__str, __n2)); }

        constexpr bool
        starts_with(basic_string_view __x) const noexcept
        { return this->substr(0, __x.size()) == __x; }

        constexpr bool
        starts_with(_CharT __x) const noexcept
        { return !this->empty() && traits_type::eq(this->front(), __x); }

        constexpr bool
        starts_with(const _CharT* __x) const noexcept
        { return this->starts_with(basic_string_view(__x)); }

        constexpr bool
        ends_with(basic_string_view __x) const noexcept
        {
            return this->size() >= __x.size() &&
                this->compare(this->size() - __x.size(), npos, __x) == 0;
        }

        constexpr bool
        ends_with(_CharT __x) const noexcept
        { return !this->empty() && traits_type::eq(this->back(), __x); }

        constexpr bool
        ends_with(const _CharT* __x) const noexcept
        { return this->ends_with(basic_string_view(__x)); }

        constexpr bool
        contains(basic_string_view __x) const noexcept
        { return this->find(__x) != npos; }

        constexpr bool
        contains(_CharT __x) const noexcept
        { return this->find(__x) != npos; }

        constexpr bool
        contains(const _CharT* __x) const noexcept
        { return this->find(__x) != npos; }

        // [string.view.find], searching

        constexpr size_type
        find(basic_string_view __str, size_type __pos = 0) const noexcept
        { return this->find(__str._M_str, __pos, __str._M_len); }

        constexpr size_type
        find(_CharT __c, size_type __pos = 0) const noexcept;

        constexpr size_type
        find(const _CharT* __str, size_type __pos, size_type __n) const noexcept;

        __attribute__((__nonnull__)) constexpr size_type
        find(const _CharT* __str, size_type __pos = 0) const noexcept
        { return this->find(__str, __pos, traits_type::length(__str)); }

        constexpr size_type
        rfind(basic_string_view __str, size_type __pos = npos) const noexcept
        { return this->rfind(__str._M_str, __pos, __str._M_len); }

        constexpr size_type
        rfind(_CharT __c, size_type __pos = npos) const noexcept;

        constexpr size_type
        rfind(const _CharT* __str, size_type __pos, size_type __n) const noexcept;

        __attribute__((__nonnull__)) constexpr size_type
        rfind(const _CharT* __str, size_type __pos = npos) const noexcept
        { return this->rfind(__str, __pos, traits_type::length(__str)); }

        constexpr size_type
        find_first_of(basic_string_view __str, size_type __pos = 0) const noexcept
        { return this->find_first_of(__str._M_str, __pos, __str._M_len); }

        constexpr size_type
        find_first_of(_CharT __c, size_type __pos = 0) const noexcept
        { return this->find(__c, __pos); }

        constexpr size_type
        find_first_of(const _CharT* __str, size_type __pos, size_type __n) const noexcept;

        __attribute__((__nonnull__)) constexpr size_type
        find_first_of(const _CharT* __str, size_type __pos = 0) const noexcept
        { return this->find_first_of(__str, __pos, traits_type::length(__str)); }

        constexpr size_type
        find_last_of(basic_string_view __str, size_type __pos = npos) const noexcept
        { return this->find_last_of(__str._M_str, __pos, __str._M_len); }

        constexpr size_type
        find_last_of(_CharT __c, size_type __pos=npos) const noexcept
        { return this->rfind(__c, __pos); }

        constexpr size_type
        find_last_of(const _CharT* __str, size_type __pos, size_type __n) const noexcept;

        __attribute__((__nonnull__)) constexpr size_type
        find_last_of(const _CharT* __str, size_type __pos = npos) const noexcept
        { return this->find_last_of(__str, __pos, traits_type::length(__str)); }

        constexpr size_type
        find_first_not_of(basic_string_view __str, size_type __pos = 0) const noexcept
        { return this->find_first_not_of(__str._M_str, __pos, __str._M_len); }

        constexpr size_type
        find_first_not_of(_CharT __c, size_type __pos = 0) const noexcept;

        constexpr size_type
        find_first_not_of(const _CharT* __str, size_type __pos, size_type __n) const noexcept;

        __attribute__((__nonnull__)) constexpr size_type
        find_first_not_of(const _CharT* __str, size_type __pos = 0) const noexcept
        { return this->find_first_not_of(__str, __pos, traits_type::length(__str)); }

        constexpr size_type
        find_last_not_of(basic_string_view __str, size_type __pos = npos) const noexcept
        { return this->find_last_not_of(__str._M_str, __pos, __str._M_len); }

        constexpr size_type
        find_last_not_of(_CharT __c, size_type __pos = npos) const noexcept;

        constexpr size_type
        find_last_not_of(const _CharT* __str, size_type __pos, size_type __n) const noexcept;

        __attribute__((__nonnull__)) constexpr size_type
        find_last_not_of(const _CharT* __str, size_type __pos = npos) const noexcept
        { return this->find_last_not_of(__str, __pos, traits_type::length(__str)); }

    private:
        static constexpr int
        _S_compare(size_type __n1, size_type __n2) noexcept
        {
            const difference_type __diff = __n1 - __n2;
            if (__diff > __gnu_cxx::__int_traits<int>::__max)
                return __gnu_cxx::__int_traits<int>::__max;
            if (__diff < __gnu_cxx::__int_traits<int>::__min)
                return __gnu_cxx::__int_traits<int>::__min;
            return static_cast<int>(__diff);
        }

        size_t _M_len;
        const _CharT* _M_str;
    };

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    find(const _CharT* __str, size_type __pos, size_type __n) const noexcept
    {
        if (__n == 0)
            return __pos <= this->_M_len ? __pos : npos;

        if (__n <= this->_M_len) {
            for (; __pos <= this->_M_len - __n; ++__pos)
                if (traits_type::eq(this->_M_str[__pos], __str[0])
                    && traits_type::compare(this->_M_str + __pos + 1,
                        __str + 1, __n - 1) == 0)
                    return __pos;
        }
        return npos;
    }

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    find(_CharT __c, size_type __pos) const noexcept
    {
        size_type __ret = npos;
        if (__pos < this->_M_len) {
            const size_type __n = this->_M_len - __pos;
            const _CharT* __p = traits_type::find(this->_M_str + __pos, __n, __c);
            if (__p)
                __ret = __p - this->_M_str;
        }
        return __ret;
    }

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    rfind(const _CharT* __str, size_type __pos, size_type __n) const noexcept
    {
        if (__n <= this->_M_len) {
            __pos = std::min(size_type(this->_M_len - __n), __pos);
            do {
                if (traits_type::compare(this->_M_str + __pos, __str, __n) == 0)
                    return __pos;
            }
            while (__pos-- > 0);
        }
        return npos;
    }

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    rfind(_CharT __c, size_type __pos) const noexcept
    {
        size_type __size = this->_M_len;
        if (__size > 0) {
            if (--__size > __pos)
                __size = __pos;
            for (++__size; __size-- > 0; )
                if (traits_type::eq(this->_M_str[__size], __c))
                    return __size;
        }
        return npos;
    }

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    find_first_of(
        const _CharT* __str, size_type __pos, size_type __n) const noexcept
    {
        for (; __n && __pos < this->_M_len; ++__pos) {
            const _CharT* __p =
                traits_type::find(__str, __n, this->_M_str[__pos]);
            if (__p)
                return __pos;
        }
        return npos;
    }

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    find_last_of(
        const _CharT* __str, size_type __pos, size_type __n) const noexcept
    {
        size_type __size = this->size();
        if (__size && __n) {
            if (--__size > __pos)
                __size = __pos;
            do {
                if (traits_type::find(__str, __n, this->_M_str[__size]))
                    return __size;
            }
            while (__size-- != 0);
        }
        return npos;
    }

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    find_first_not_of(
        const _CharT* __str, size_type __pos, size_type __n) const noexcept
    {
        for (; __pos < this->_M_len; ++__pos)
            if (!traits_type::find(__str, __n, this->_M_str[__pos]))
                return __pos;
        return npos;
    }

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    find_first_not_of(_CharT __c, size_type __pos) const noexcept
    {
        for (; __pos < this->_M_len; ++__pos)
            if (!traits_type::eq(this->_M_str[__pos], __c))
                return __pos;
        return npos;
    }

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    find_last_not_of(
        const _CharT* __str, size_type __pos, size_type __n) const noexcept
    {
        size_type __size = this->_M_len;
        if (__size) {
            if (--__size > __pos)
                __size = __pos;
            do {
                if (!traits_type::find(__str, __n, this->_M_str[__size]))
                    return __size;
            }
            while (__size--);
        }
        return npos;
    }

    template <typename _CharT, typename _Traits>
    constexpr typename basic_string_view<_CharT, _Traits>::size_type
    basic_string_view<_CharT, _Traits>::
    find_last_not_of(_CharT __c, size_type __pos) const noexcept
    {
        size_type __size = this->_M_len;
        if (__size) {
            if (--__size > __pos)
                __size = __pos;
            do {
                if (!traits_type::eq(this->_M_str[__size], __c))
                    return __size;
            }
            while (__size--);
        }
        return npos;
    }

    // [string.view.comparison], non-member basic_string_view comparison function

    // Several of these functions use type_identity_t to create a non-deduced
    // context, so that only one argument participates in template argument
    // deduction and the other argument gets implicitly converted to the deduced
    // type (see N3766).

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator==(basic_string_view<_CharT, _Traits> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.size() == __y.size() && __x.compare(__y) == 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator==(basic_string_view<_CharT, _Traits> __x,
               std::type_identity_t<basic_string_view<_CharT, _Traits>> __y)
    noexcept
    { return __x.size() == __y.size() && __x.compare(__y) == 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator==(std::type_identity_t<basic_string_view<_CharT, _Traits>> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.size() == __y.size() && __x.compare(__y) == 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator!=(basic_string_view<_CharT, _Traits> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return !(__x == __y); }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator!=(basic_string_view<_CharT, _Traits> __x,
               std::type_identity_t<basic_string_view<_CharT, _Traits>> __y)
    noexcept
    { return !(__x == __y); }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator!=(std::type_identity_t<basic_string_view<_CharT, _Traits>> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return !(__x == __y); }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator< (basic_string_view<_CharT, _Traits> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.compare(__y) < 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator< (basic_string_view<_CharT, _Traits> __x,
               std::type_identity_t<basic_string_view<_CharT, _Traits>> __y)
    noexcept
    { return __x.compare(__y) < 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator< (std::type_identity_t<basic_string_view<_CharT, _Traits>> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.compare(__y) < 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator> (basic_string_view<_CharT, _Traits> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.compare(__y) > 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator> (basic_string_view<_CharT, _Traits> __x,
               std::type_identity_t<basic_string_view<_CharT, _Traits>> __y)
    noexcept
    { return __x.compare(__y) > 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator> (std::type_identity_t<basic_string_view<_CharT, _Traits>> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.compare(__y) > 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator<=(basic_string_view<_CharT, _Traits> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.compare(__y) <= 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator<=(basic_string_view<_CharT, _Traits> __x,
               std::type_identity_t<basic_string_view<_CharT, _Traits>> __y)
    noexcept
    { return __x.compare(__y) <= 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator<=(std::type_identity_t<basic_string_view<_CharT, _Traits>> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.compare(__y) <= 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator>=(basic_string_view<_CharT, _Traits> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.compare(__y) >= 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator>=(basic_string_view<_CharT, _Traits> __x,
               std::type_identity_t<basic_string_view<_CharT, _Traits>> __y)
    noexcept
    { return __x.compare(__y) >= 0; }

    template <typename _CharT, typename _Traits>
    constexpr bool
    operator>=(std::type_identity_t<basic_string_view<_CharT, _Traits>> __x,
               basic_string_view<_CharT, _Traits> __y) noexcept
    { return __x.compare(__y) >= 0; }

    // [string.view.io], Inserters and extractors
    template <typename _CharT, typename _Traits>
    inline basic_ostream<_CharT, _Traits>&
    operator<<(basic_ostream<_CharT, _Traits>& __os,
           basic_string_view<_CharT,_Traits> __str)
    { return __ostream_insert(__os, __str.data(), __str.size()); }


    // basic_string_view typedef names

    using string_view = basic_string_view<char>;
#ifdef _GLIBCXX_USE_WCHAR_T
    using wstring_view = basic_string_view<wchar_t>;
#endif
#ifdef _GLIBCXX_USE_CHAR8_T
    using u8string_view = basic_string_view<char8_t>;
#endif
    using u16string_view = basic_string_view<char16_t>;
    using u32string_view = basic_string_view<char32_t>;

    // [string.view.hash], hash support:

    template <class _Tp>
    struct hash;

    template <>
    struct hash<string_view>
    : public __hash_base<size_t, string_view>
    {
        size_t
        operator()(const string_view& __str) const noexcept
        { return std::_Hash_impl::hash(__str.data(), __str.length()); }
    };

    template <>
    struct __is_fast_hash<hash<string_view>> : std::false_type
    { };

#ifdef _GLIBCXX_USE_WCHAR_T
    template <>
    struct hash<wstring_view>
    : public __hash_base<size_t, wstring_view>
    {
        size_t
        operator()(const wstring_view& __s) const noexcept
        { return std::_Hash_impl::hash(
            __s.data(), __s.length() * sizeof(wchar_t)); }
    };

    template <>
    struct __is_fast_hash<hash<wstring_view>> : std::false_type
    { };
#endif

#ifdef _GLIBCXX_USE_CHAR8_T
    template <>
    struct hash<u8string_view>
    : public __hash_base<size_t, u8string_view>
    {
        size_t
        operator()(const u8string_view& __str) const noexcept
        { return std::_Hash_impl::hash(__str.data(), __str.length()); }
    };

    template <>
    struct __is_fast_hash<hash<u8string_view>> : std::false_type
    { };
#endif

    template <>
    struct hash<u16string_view>
    : public __hash_base<size_t, u16string_view>
    {
        size_t
        operator()(const u16string_view& __s) const noexcept
        { return std::_Hash_impl::hash(
            __s.data(), __s.length() * sizeof(char16_t)); }
    };

    template <>
    struct __is_fast_hash<hash<u16string_view>> : std::false_type
    { };

    template <>
    struct hash<u32string_view>
    : public __hash_base<size_t, u32string_view>
    {
        size_t
        operator()(const u32string_view& __s) const noexcept
        { return std::_Hash_impl::hash(
            __s.data(), __s.length() * sizeof(char32_t)); }
    };

    template <>
    struct __is_fast_hash<hash<u32string_view>> : std::false_type
    { };

    inline namespace literals
    {
        inline namespace string_view_literals
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wliteral-suffix"
            inline constexpr basic_string_view<char>
            operator""sv(const char* __str, size_t __len) noexcept
            { return basic_string_view<char>{__str, __len}; }

#ifdef _GLIBCXX_USE_WCHAR_T
            inline constexpr basic_string_view<wchar_t>
            operator""sv(const wchar_t* __str, size_t __len) noexcept
            { return basic_string_view<wchar_t>{__str, __len}; }
#endif

#ifdef _GLIBCXX_USE_CHAR8_T
            inline constexpr basic_string_view<char8_t>
            operator""sv(const char8_t* __str, size_t __len) noexcept
            { return basic_string_view<char8_t>{__str, __len}; }
#endif

            inline constexpr basic_string_view<char16_t>
            operator""sv(const char16_t* __str, size_t __len) noexcept
            { return basic_string_view<char16_t>{__str, __len}; }

            inline constexpr basic_string_view<char32_t>
            operator""sv(const char32_t* __str, size_t __len) noexcept
            { return basic_string_view<char32_t>{__str, __len}; }

#pragma GCC diagnostic pop
        } // namespace string_literals
    } // namespace literals

} // namespace std

#endif // __cplusplus < 201703L

