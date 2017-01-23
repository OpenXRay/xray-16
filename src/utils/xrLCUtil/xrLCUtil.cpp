#include "pch.hpp"
#include "xrLCUtil.hpp"

std::string make_time(u32 sec)
{
    char buf[64];
    xr_sprintf(buf, "%2.0d:%2.0d:%2.0d", sec/3600, (sec%3600)/60, sec%60);
    int len = int(xr_strlen(buf));
    for (int i = 0; i<len; i++)
    {
        if (buf[i]==' ')
            buf[i] = '0';
    }
    return std::string(buf);
}
