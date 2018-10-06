#include "stdafx.h"
#include "StringConversion.hpp"
#include <codecvt>

// XXX: use c++11 functions and kill that DUMP_CONVERSION!!!11

#define BITS1_MASK 0x80 // 10000000b
#define BITS2_MASK 0xC0 // 11000000b
#define BITS3_MASK 0xE0 // 11100000b
#define BITS4_MASK 0xF0 // 11110000b
#define BITS1_EXP 0x00 // 00000000b
#define BITS2_EXP 0x80 // 10000000b
#define BITS3_EXP 0xC0 // 11000000b
#define BITS4_EXP 0xE0 // 11100000b

// MB_DUMB_CONVERSION - ignore non UTF-8 compliant sequences, as-is conversion
#define MB_DUMB_CONVERSION

#ifdef MB_DUMB_CONVERSION

u16 mbhMulti2WideDumb(wchar_t* WideStr, wchar_t* WidePos, u16 WideStrSize, const char* MultiStr)
{
    u16 spos = 0, dpos = 0;
    wchar_t b1;
    wchar_t wc = 0;
    VERIFY(MultiStr);
    if (!MultiStr[0])
        return 0;
    if (WideStr || WidePos)
        VERIFY2(((WideStrSize > 0) && (WideStrSize < 0xFFFF)), make_string("'WideStrSize'=%hu", WideStrSize));
    while ((b1 = MultiStr[spos++]) != 0x00)
    {
        if (WidePos)
            WidePos[dpos] = spos;
        dpos++;
        wc = b1;
        if (WideStr)
        {
            VERIFY2((dpos < WideStrSize), make_string("S1: '%s',%hu<%hu", MultiStr, dpos, WideStrSize));
            WideStr[dpos] = wc;
        }
    }
    if (WidePos)
        WidePos[dpos] = spos;
    if (WideStr)
    {
        VERIFY2((dpos < WideStrSize), make_string("S2: '%s',%hu<%hu", MultiStr, dpos, WideStrSize));
        WideStr[dpos + 1] = 0x0000;
    }
    if (WideStr)
        WideStr[0] = dpos;
    return dpos;
}

#endif // MB_DUMB_CONVERSION

u16 mbhMulti2Wide(wchar_t* WideStr, wchar_t* WidePos, u16 WideStrSize, const char* MultiStr)
{
    u16 spos = 0;
    u16 dpos = 0;
    wchar_t b1, b2, b3;
    wchar_t wc = 0;
    VERIFY(MultiStr);
    if (!MultiStr[0])
        return 0;
    if (WideStr || WidePos)
        VERIFY2(((WideStrSize > 0) && (WideStrSize < 0xFFFF)), make_string("'WideStrSize'=%hu", WideStrSize));
    while ((b1 = MultiStr[spos]) != 0x00)
    {
        if (WidePos)
            WidePos[dpos] = spos;
        spos++;
        if ((b1 & BITS1_MASK) == BITS1_EXP)
            wc = b1;
        else if ((b1 & BITS3_MASK) == BITS3_EXP)
        {
            b2 = MultiStr[spos++];
#ifdef MB_DUMB_CONVERSION
            if (!(b2 && ((b2 & BITS2_MASK) == BITS2_EXP)))
                return mbhMulti2WideDumb(WideStr, WidePos, WideStrSize, MultiStr);
#else
            VERIFY2((b2 && ((b2 & BITS2_MASK) == BITS2_EXP)),
                make_string("B2: '%s',@%hu,[%hc][%hc]", MultiStr, spos, b1, b2));
#endif
            wc = ((b1 & ~BITS3_MASK) << 6) | (b2 & ~BITS2_MASK);
        }
        else if ((b1 & BITS4_MASK) == BITS4_EXP)
        {
            b2 = MultiStr[spos++];
#ifdef MB_DUMB_CONVERSION
            if (!(b2 && ((b2 & BITS2_MASK) == BITS2_EXP)))
                return mbhMulti2WideDumb(WideStr, WidePos, WideStrSize, MultiStr);
#else
            VERIFY2((b2 && ((b2 & BITS2_MASK) == BITS2_EXP)),
                make_string("B31: '%s',@%hu,[%hc][%hc]", MultiStr, spos, b1, b2));
#endif
            b3 = MultiStr[spos++];
#ifdef MB_DUMB_CONVERSION
            if (!(b3 && ((b3 & BITS2_MASK) == BITS2_EXP)))
                return mbhMulti2WideDumb(WideStr, WidePos, WideStrSize, MultiStr);
#else
            VERIFY2((b3 && ((b3 & BITS2_MASK) == BITS2_EXP)),
                make_string("B32: '%s',@%hu,[%hc][%hc][%hc]", MultiStr, spos, b1, b2, b3));
#endif
            wc = ((b1 & ~BITS4_MASK) << 12) | ((b2 & ~BITS2_MASK) << 6) | (b3 & ~BITS2_MASK);
        }
        else
        {
#ifdef MB_DUMB_CONVERSION
            return mbhMulti2WideDumb(WideStr, WidePos, WideStrSize, MultiStr);
#else
            VERIFY2(0, make_string("B1: '%s',@%hu,[%hc]", MultiStr, spos, b1));
#endif
        }
        dpos++;
        if (WideStr)
        {
            VERIFY2((dpos < WideStrSize), make_string("S1: '%s',%hu<%hu", MultiStr, dpos, WideStrSize));
            WideStr[dpos] = wc;
        }
    }
    if (WidePos)
        WidePos[dpos] = spos;
    if (WideStr)
    {
        VERIFY2((dpos < WideStrSize), make_string("S2: '%s',%hu<%hu", MultiStr, dpos, WideStrSize));
        WideStr[dpos + 1] = 0x0000;
    }
    if (WideStr)
        WideStr[0] = dpos;
    return dpos;
}

xr_string StringFromUTF8(const char* in, const std::locale& locale)
{
    using wcvt = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;
    auto wstr = wcvt{}.from_bytes(in);
    xr_string result(wstr.size(), '\0');
    std::use_facet<std::ctype<wchar_t>>(locale).narrow(wstr.data(), wstr.data() + wstr.size(), '?', &result[0]);
    return result;
}

xr_string StringToUTF8(const char* in, const std::locale& locale)
{
    using wcvt = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;
    std::wstring wstr(xr_strlen(in), L'\0');
    std::use_facet<std::ctype<wchar_t>>(locale).widen(in, in + xr_strlen(in), &wstr[0]);
    std::string result = wcvt{}.to_bytes(wstr.data(), wstr.data() + wstr.size());
    return result.data();
}
