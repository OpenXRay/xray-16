#include "xrCore/xrstring.h"
#include "xrCore/xrMemory.h"

void xr_strlwr(shared_str& src)
{
    if (*src)
    {
        char* lp = xr_strdup(*src);
        xr_strlwr(lp);
        src = lp;
        xr_free(lp);
    }
}

shared_str& __cdecl shared_str::printf(const char* format, ...)
{
    string4096 buf;
    va_list p;
    va_start(p, format);
    int vs_sz = vsnprintf(buf, sizeof(buf)-1, format, p);
    buf[sizeof(buf)-1] = 0;
    va_end(p);
    if (vs_sz)
        _set(buf);
    return (shared_str&)*this;
}
