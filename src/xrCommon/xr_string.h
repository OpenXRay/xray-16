#pragma once
#include <string>
#include "xalloc.h"

// string(char)
using xr_string = std::basic_string<char, std::char_traits<char>, xalloc<char>>;

inline void xr_strlwr(xr_string& src)
{
    for (xr_string::iterator it = src.begin(); it!=src.end(); it++)
        *it = xr_string::value_type(tolower(*it));
}
