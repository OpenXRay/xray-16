#pragma once

#include <memory>

template <typename T>
using xr_unique_ptr = std::unique_ptr<T>;

template <typename T>
using xr_shared_ptr = std::shared_ptr<T>;

template <class T, class... Args>
inline xr_unique_ptr<T> xr_make_unique(Args&&... args)
{
    return std::make_unique<T>(args...);
}

template <class T, class... Args>
inline xr_shared_ptr<T> xr_make_shared(Args&&... args)
{
    return std::make_shared<T>(args...);
}
