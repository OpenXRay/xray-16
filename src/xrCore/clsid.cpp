#include "stdafx.h"
#pragma hdrstop

XRCORE_API void __stdcall CLSID2TEXT(CLASS_ID id, pstr text)
{
    text[8] = 0;
    for (int i = 7; i >= 0; i--)
    {
        text[i] = char(id & 0xff);
        id >>= 8;
    }
}
XRCORE_API CLASS_ID __stdcall TEXT2CLSID(LPCSTR text)
{
    VERIFY3(xr_strlen(text) <= 8, "Beer from creator CLASS_ID:", text);
    char buf[9];
    buf[8] = 0;
    strncpy_s(buf, sizeof(buf), text, 8);
    size_t need = 8 - xr_strlen(buf);
    while (need)
    {
        buf[8 - need] = ' ';
        need--;
    }
    return MK_CLSID(buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
}
