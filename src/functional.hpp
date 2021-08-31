#pragma once
#include <functional>

#if __cplusplus < 201703L

namespace std
{
    namespace detail
    {
        // Concepts
        template <class T, class = std::enable_if_t<std::is_function<T>::value>>
        using function_t = T;

        template <class C, class T1,
            class = std::enable_if_t<std::is_base_of<C, std::decay_t<T1>>::value>>
        using base_t = T1;

    } // namespace detail

    // std::invoke
    // https://en.cppreference.com/w/cpp/utility/functional/invoke

    template <class F, class... Args>
    constexpr decltype(auto)
    invoke(F&& f, Args&&... args) {
        return std::forward<F>(f)(std::forward<Args>(args)...);
    }

    template <class C, class Pointed, class T1, class... Args>
    constexpr decltype(auto)
    invoke(detail::function_t<Pointed> C::* f, detail::base_t<C, T1>&& t1, Args&&... args) {
        return (std::forward<T1>(t1).*f)(std::forward<Args>(args)...);
    }

    // std::invoke_result
    template <class F, class... Args>
    using invoke_result_t = decltype(invoke(std::declval<F>(), std::declval<Args>()...));

} // namespace std

#endif // __cplusplus < 201703L

