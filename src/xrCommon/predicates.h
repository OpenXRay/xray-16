#pragma once

#include "xrCore/xrstring.h"

struct pred_str
{
    bool operator()(const char* x, const char* y) const
    { return xr_strcmp(x, y) < 0; }
};

struct pred_stri
{
    bool operator()(const char* x, const char* y) const
    { return xr_stricmp(x, y) < 0; }
};
