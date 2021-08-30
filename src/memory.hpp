#pragma once
#include <memory>

#if __cplusplus < 201703L
namespace std
{
    /// \see https://en.cppreference.com/w/cpp/memory/destroy_at
    template <class T>
    inline void destroy_at(T* p) {
        p->~T();
    }

} // namespace std
#endif // __cplusplus < 201703L

