#pragma once
#include <string>
#include "xr_allocator.h"

// string(char)
using xr_string = std::basic_string<char, std::char_traits<char>, xr_allocator<char>>;

inline void xr_strlwr(xr_string& src)
{
    for (auto& it : src)
        it = xr_string::value_type(tolower(it));
}
