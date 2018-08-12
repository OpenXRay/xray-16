#pragma once
#include "xrCore/xrCore.h"

// XXX: use c++11 functions

#define MAX_MB_CHARS 4096

XRCORE_API u16 mbhMulti2Wide(wchar_t* WideStr, wchar_t* WidePos, u16 WideStrSize, const char* MultiStr);

IC bool IsNeedSpaceCharacter(wchar_t wc)
{
    return ((wc == 0x0020) || (wc == 0x3000) || (wc == 0xFF01) || (wc == 0xFF0C) ||
        // ( wc == 0xFF0D ) ||
        (wc == 0xFF0E) || (wc == 0xFF1A) || (wc == 0xFF1B) || (wc == 0xFF1F) || (wc == 0x2026) || (wc == 0x3002) ||
        (wc == 0x3001));
}

IC bool IsBadStartCharacter(wchar_t wc)
{
    return (IsNeedSpaceCharacter(wc) || (wc == 0x0021) || (wc == 0x002C) ||
        // ( wc == 0x002D ) ||
        (wc == 0x002E) || (wc == 0x003A) || (wc == 0x003B) || (wc == 0x003F) || (wc == 0x0029) || (wc == 0xFF09));
}

IC bool IsBadEndCharacter(wchar_t wc) { return ((wc == 0x0028) || (wc == 0xFF08) || (wc == 0x4E00)); }
IC bool IsAlphaCharacter(wchar_t wc)
{
    return (((wc >= 0x0030) && (wc <= 0x0039)) || ((wc >= 0x0041) && (wc <= 0x005A)) ||
        ((wc >= 0x0061) && (wc <= 0x007A)) || ((wc >= 0xFF10) && (wc <= 0xFF19)) ||
        ((wc >= 0xFF21) && (wc <= 0xFF3A)) || ((wc >= 0xFF41) && (wc <= 0xFF5A)));
}

XRCORE_API xr_string StringFromUTF8(const char* string, const std::locale& locale = std::locale(""));
XRCORE_API xr_string StringToUTF8(const char* string, const std::locale& locale = std::locale(""));
