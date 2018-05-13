#pragma once
#include <array>

template<typename T, size_t Size>
using xr_array = std::array<T, Size>;

// compatibility with old svector
template<typename T, size_t Size>
class xr_array_s : public std::array<T, Size>
{
    // svector used to contain the 'count' variable
    // which used 4 bytes of memory
    u32 dummy = 0;
};
