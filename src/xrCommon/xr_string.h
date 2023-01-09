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

inline xr_string xr_substrreplace(const xr_string& src, const xr_string& src_substr, const xr_string& dst_substr)
{
    xr_string res(src);
    while (true)
    {
        size_t pos = res.find(src_substr);
        if (pos != xr_string::npos)
        {
            res.replace(pos, src_substr.size(), dst_substr);
        }
        else
        {
            break;
        }
    }
    return res;
}

#if !defined(XR_PLATFORM_WINDOWS)
template<>
struct std::hash<xr_string>
{
    [[nodiscard]] size_t operator()(const xr_string& str) const noexcept
    {
        return std::hash<std::string_view>{}(str.c_str());
    }
};
#endif
