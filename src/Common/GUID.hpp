#pragma once
#include "xrCore/xrCore.h"

class xrGUID
{
public:
    u64 g[2];

    ICF bool operator== (const xrGUID &that) const
    { return g[0]==that.g[0] && g[1]==that.g[1]; }

    ICF bool operator!= (const xrGUID &that) const
    { return !(*this==that); }

    ICF void LoadLTX(CInifile &ini, const char *section, const char *name)
    {
        string128 buff;
        g[0] = ini.r_u64(section, strconcat(sizeof(buff), buff, name, "_g0"));
        g[1] = ini.r_u64(section, strconcat(sizeof(buff), buff, name, "_g1"));
    }

    ICF void SaveLTX(CInifile &ini, const char *section, const char *name)
    {
        string128 buff;
        ini.w_u64(section, strconcat(sizeof(buff), buff, name, "_g0"), g[0]);
        ini.w_u64(section, strconcat(sizeof(buff), buff, name, "_g1"), g[1]);
    }
};
