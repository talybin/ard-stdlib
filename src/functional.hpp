// <functional> -*- C++ -*-

// Copyright (C) 2001-2020 Free Software Foundation, Inc.
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

/*
 * Copyright (c) 1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

#pragma once
#include <functional>

#if __cplusplus < 201703L
#include "type_traits.hpp"

namespace std
{
    /// Invoke a callable object
    template <class _Callable, class... _Args>
    inline constexpr invoke_result_t<_Callable, _Args...>
    invoke(_Callable&& __fn, _Args&&... __args)
    noexcept(is_nothrow_invocable<_Callable, _Args...>::value)
    {
        return std::__invoke(std::forward<_Callable>(__fn),
            std::forward<_Args>(__args)...);
    }

} // namespace std
#endif // __cplusplus < 201703L

