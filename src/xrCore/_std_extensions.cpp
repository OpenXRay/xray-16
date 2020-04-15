#include "stdafx.h"
#pragma hdrstop

#include <time.h>

#ifdef BREAK_AT_STRCMP
int xr_strcmp(const char* S1, const char* S2)
{
    int res = (int)strcmp(S1, S2);
    return res;
}
#endif

char* timestamp(string64& dest)
{
    time_t     now = time(nullptr);
    struct tm  tstruct;

#if defined(XR_PLATFORM_WINDOWS)
    localtime_s(&tstruct, &now);// thread-safe for windows
#else
    localtime_r(&now, &tstruct);// thread-safe for posix systems
#endif

    strftime(dest, sizeof(dest), "%m-%d-%y_%H-%M-%S", &tstruct);

    return dest;
}
