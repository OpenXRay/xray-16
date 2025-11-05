#pragma once

#include "xrCore/xrstring.h"

#include <algorithm>
#include <cctype>

namespace xray::render
{
namespace detail
{
inline xr_string NormalizeModelIdentifier(pcstr raw_name)
{
    if (!raw_name)
        return {};

    xr_string normalized(raw_name);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    const auto dot = normalized.find_last_of('.');
    if (dot != xr_string::npos)
    {
        if (normalized.compare(dot, xr_string::npos, ".ozzx") != 0)
            normalized.erase(dot);
    }

    return normalized;
}
} // namespace detail
} // namespace xray::render
