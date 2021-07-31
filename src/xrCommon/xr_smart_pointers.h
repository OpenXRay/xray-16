#pragma once

#include "../xrCore/xrMemory.h"
#include <memory>
#include <functional>

template <typename T>
struct xr_custom_deleter
{
    void operator()(T* ptr) const noexcept
    {
        xr_delete(ptr);
    }
};

template <typename T>
using xr_unique_ptr = std::unique_ptr<T, xr_custom_deleter<T>>;

template <typename T>
using xr_shared_ptr = std::shared_ptr<T>;

template <class T, class... Args>
inline xr_unique_ptr<T> xr_make_unique(Args&&... args)
{
    return xr_unique_ptr<T>(xr_new<T>(std::forward<Args>(args)...));
}

template <class T, class... Args>
inline xr_shared_ptr<T> xr_make_shared(Args&&... args)
{
    return xr_shared_ptr<T>(xr_new<T>(std::forward<Args>(args)...), xr_custom_deleter<T>());
}
