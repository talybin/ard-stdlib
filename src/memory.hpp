#pragma once
#include <memory>

#if __cplusplus < 201703L
namespace std
{
    /// \see https://en.cppreference.com/w/cpp/memory/destroy_at
    template <class _Tp>
    inline void destroy_at(_Tp* p) {
        p->~_Tp();
    }

} // namespace std
#endif // __cplusplus < 201703L

