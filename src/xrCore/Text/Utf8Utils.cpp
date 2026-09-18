#include "stdafx.h"
#include "Utf8Utils.hpp"

#if defined(XR_PLATFORM_WINDOWS)
#include <Windows.h>
#endif

#include <cwchar>
#include <string>

namespace XRay
{
namespace Utf8
{
size_t SequenceLength(u8 leadingByte)
{
    if ((leadingByte & 0x80) == 0x00)
        return 1;
    if ((leadingByte & 0xE0) == 0xC0)
        return 2;
    if ((leadingByte & 0xF0) == 0xE0)
        return 3;
    if ((leadingByte & 0xF8) == 0xF0)
        return 4;
    return 0; // Invalid leading byte
}

pcstr Next(pcstr pos)
{
    if (!pos || !*pos)
        return pos;

    const size_t len = SequenceLength(static_cast<u8>(*pos));
    if (len == 0)
        return pos + 1; // Invalid byte, skip one byte to avoid infinite loops

    for (size_t i = 1; i < len; ++i)
    {
        if (!pos[i] || !IsContinuationByte(static_cast<u8>(pos[i])))
            return pos + 1; // Truncated or invalid sequence, skip one byte
    }

    return pos + len;
}

pcstr Prev(pcstr start, pcstr pos)
{
    if (!start || !pos || pos <= start)
        return start;

    pcstr prev = pos - 1;
    // Skip continuation bytes
    while (prev > start && IsContinuationByte(static_cast<u8>(*prev)))
        --prev;

    // Validate that we landed on a valid leading byte
    const size_t len = SequenceLength(static_cast<u8>(*prev));
    if (len == 0 || prev + len > pos)
    {
        // Invalid sequence. Keep moving back over continuation bytes so we never
        // return a continuation byte as a "codepoint start" (callers that only
        // step once would otherwise land inside a multi-byte sequence).
        pcstr fallback = pos - 1;
        while (fallback > start && IsContinuationByte(static_cast<u8>(*fallback)))
            --fallback;
        return fallback;
    }

    return prev;
}

u32 Decode(pcstr pos, size_t& outLength)
{
    outLength = 0;
    if (!pos || !*pos)
        return 0;

    const u8 b1 = static_cast<u8>(*pos);
    const size_t len = SequenceLength(b1);

    if (len == 1)
    {
        outLength = 1;
        return b1;
    }

    if (len == 2)
    {
        const u8 b2 = static_cast<u8>(pos[1]);
        if (b2 && IsContinuationByte(b2))
        {
            const u32 cp = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
            // Reject overlong sequences (e.g. 0xC0 0x80 encoding U+0000).
            if (cp < 0x80)
            {
                outLength = 1;
                return REPLACEMENT_CHARACTER;
            }
            outLength = 2;
            return cp;
        }
    }
    else if (len == 3)
    {
        const u8 b2 = static_cast<u8>(pos[1]);
        if (b2 && IsContinuationByte(b2))
        {
            const u8 b3 = static_cast<u8>(pos[2]);
            if (b3 && IsContinuationByte(b3))
            {
                const u32 cp = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
                // Reject overlong sequences and surrogates
                if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF))
                {
                    outLength = 1;
                    return REPLACEMENT_CHARACTER;
                }
                outLength = 3;
                return cp;
            }
        }
    }
    else if (len == 4)
    {
        const u8 b2 = static_cast<u8>(pos[1]);
        if (b2 && IsContinuationByte(b2))
        {
            const u8 b3 = static_cast<u8>(pos[2]);
            if (b3 && IsContinuationByte(b3))
            {
                const u8 b4 = static_cast<u8>(pos[3]);
                if (b4 && IsContinuationByte(b4))
                {
                    const u32 cp = ((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12) | ((b3 & 0x3F) << 6) | (b4 & 0x3F);
                    if (cp < 0x10000 || cp > 0x10FFFF)
                    {
                        outLength = 1;
                        return REPLACEMENT_CHARACTER;
                    }
                    outLength = 4;
                    return cp;
                }
            }
        }
    }

    outLength = 1;
    return REPLACEMENT_CHARACTER;
}

size_t LengthCodepoints(pcstr str)
{
    size_t count = 0;
    while (str && *str)
    {
        str = Next(str);
        ++count;
    }
    return count;
}

pcstr Advance(pcstr pos, size_t codepoints)
{
    if (!pos || !*pos || codepoints == 0)
        return pos;

    while (codepoints > 0 && *pos)
    {
        pos = Next(pos);
        --codepoints;
    }

    return pos;
}

size_t DistanceCodepoints(pcstr start, pcstr end)
{
    if (!start || !end || start >= end)
        return 0;

    size_t count = 0;
    while (*start && start < end)
    {
        start = Next(start);
        ++count;
    }

    return count;
}

bool IsValid(pcstr str)
{
    if (!str)
        return true; // Empty/null is valid

    while (*str)
    {
        size_t len = 0;
        const u32 cp = Decode(str, len);
        if (cp == REPLACEMENT_CHARACTER && len == 1 && SequenceLength(static_cast<u8>(*str)) != 1)
            return false;
        str += len;
    }
    return true;
}

pcstr StripBom(pcstr str)
{
    if (!str)
        return nullptr;
    if (static_cast<u8>(str[0]) == 0xEF && static_cast<u8>(str[1]) == 0xBB && static_cast<u8>(str[2]) == 0xBF)
        return str + 3;
    return str;
}

void ToLowerAscii(pstr str)
{
    if (!str)
        return;
    while (*str)
    {
        if (*str >= 'A' && *str <= 'Z')
            *str += 'a' - 'A';
        ++str;
    }
}

void ToUpperAscii(pstr str)
{
    if (!str)
        return;
    while (*str)
    {
        if (*str >= 'a' && *str <= 'z')
            *str -= 'a' - 'A';
        ++str;
    }
}

std::wstring ToWide(pcstr str)
{
    std::wstring result;
    if (!str)
        return result;

    result.reserve(xr_strlen(str)); // Worst case: all ASCII

    while (*str)
    {
        size_t len = 0;
        const u32 cp = Decode(str, len);
        str += len;

        if (cp <= 0xFFFF)
        {
            result.push_back(static_cast<wchar_t>(cp));
        }
        else
        {
            // Surrogate pair for non-BMP (should not happen for us, but handle correctly)
            const u32 code = cp - 0x10000;
            result.push_back(static_cast<wchar_t>(0xD800 + (code >> 10)));
            result.push_back(static_cast<wchar_t>(0xDC00 + (code & 0x3FF)));
        }
    }

    return result;
}

xr_string FromWide(const wchar_t* str)
{
    xr_string result;
    if (!str)
        return result;

    const size_t len = std::wcslen(str);
    result.reserve(len * 3); // Worst case: 3 bytes per wchar_t for BMP

    for (size_t i = 0; i < len; ++i)
    {
        const wchar_t wc = str[i];
        if (wc <= 0x7F)
        {
            result.push_back(static_cast<char>(wc));
        }
        else if (wc <= 0x7FF)
        {
            result.push_back(static_cast<char>(0xC0 | (wc >> 6)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
        else if (wc >= 0xD800 && wc <= 0xDBFF && i + 1 < len)
        {
            // Surrogate pair
            const wchar_t low = str[++i];
            if (low >= 0xDC00 && low <= 0xDFFF)
            {
                const u32 cp = 0x10000 + ((wc - 0xD800) << 10) + (low - 0xDC00);
                result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else
            {
                result.push_back(static_cast<char>(0xEF));
                result.push_back(static_cast<char>(0xBF));
                result.push_back(static_cast<char>(0xBD));
            }
        }
        else if (wc >= 0xDC00 && wc <= 0xDFFF)
        {
            // Lone low surrogate
            result.push_back(static_cast<char>(0xEF));
            result.push_back(static_cast<char>(0xBF));
            result.push_back(static_cast<char>(0xBD));
        }
        else
        {
            result.push_back(static_cast<char>(0xE0 | (wc >> 12)));
            result.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
    }

    return result;
}

namespace
{
// CP1251 to Unicode mapping for bytes 0x80..0xBF.
static const wchar_t CP1251ToUnicodeTable[64] =
{
    0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
    0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
    0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0xFFFD, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
    0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
    0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
    0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
    0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
};

static wchar_t CP1251ByteToUnicode(u8 byte)
{
    if (byte < 0x80)
        return static_cast<wchar_t>(byte);

    if (byte < 0xC0)
        return CP1251ToUnicodeTable[byte - 0x80];

    // 0xC0..0xFF map directly to U+0410..U+044F
    return static_cast<wchar_t>(byte - 0xC0 + 0x0410);
}

static u8 UnicodeToCP1251Byte(wchar_t wc)
{
    if (wc < 0x80)
        return static_cast<u8>(wc);

    if (wc >= 0x0410 && wc <= 0x044F)
        return static_cast<u8>(wc - 0x0410 + 0xC0);

    for (int i = 0; i < 64; ++i)
    {
        if (CP1251ToUnicodeTable[i] == wc)
            return static_cast<u8>(0x80 + i);
    }

    return static_cast<u8>('?');
}

static xr_string CP1251ToUtf8(pcstr str)
{
    const size_t len = xr_strlen(str);
    std::wstring wide;
    wide.reserve(len);

    for (size_t i = 0; i < len; ++i)
        wide.push_back(CP1251ByteToUnicode(static_cast<u8>(str[i])));

    return FromWide(wide.c_str());
}

static xr_string Utf8ToCP1251(pcstr str)
{
    xr_string result;
    result.reserve(xr_strlen(str));

    pcstr p = str;
    while (*p)
    {
        size_t len = 0;
        const u32 cp = Decode(p, len);
        if (len == 0)
        {
            ++p;
            result.push_back('?');
            continue;
        }

        result.push_back(static_cast<char>(UnicodeToCP1251Byte(static_cast<wchar_t>(cp))));
        p += len;
    }

    return result;
}
} // namespace

// Public single-byte CP1251 -> Unicode codepoint mapping (uses the internal
// table above). Exposed so the font atlas builder can re-key legacy CP1251
// glyphs without duplicating the translation table.
u32 CP1251ByteToCodepoint(u8 byte) { return static_cast<u32>(CP1251ByteToUnicode(byte)); }

xr_string FromACP(pcstr str)
{
    if (!str || !str[0])
        return xr_string();

#if defined(XR_PLATFORM_WINDOWS)
    // ACP on Windows may already be UTF-8 (system locale option) or a legacy code page.
    // If the input is valid UTF-8, keep it unchanged.
    if (IsValid(str))
        return xr_string(str);

    const int wideLen = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    if (wideLen <= 0)
        return xr_string();

    std::wstring wide;
    wide.resize(wideLen);
    MultiByteToWideChar(CP_ACP, 0, str, -1, wide.data(), wideLen);
    return FromWide(wide.c_str());
#else
    // POSIX has no ACP. Assume UTF-8 first, then fall back to CP1251 for legacy content.
    if (IsValid(str))
        return xr_string(str);

    return CP1251ToUtf8(str);
#endif
}

xr_string FromCP1251(pcstr str)
{
    if (!str || !str[0])
        return xr_string();

#if defined(XR_PLATFORM_WINDOWS)
    constexpr UINT CP_1251 = 1251;
    const int wideLen = MultiByteToWideChar(CP_1251, 0, str, -1, nullptr, 0);
    if (wideLen <= 0)
        return xr_string();

    std::wstring wide;
    wide.resize(wideLen);
    MultiByteToWideChar(CP_1251, 0, str, -1, wide.data(), wideLen);
    return FromWide(wide.c_str());
#else
    return CP1251ToUtf8(str);
#endif
}

xr_string ToCP1251(pcstr str)
{
    if (!str || !str[0])
        return xr_string();

#if defined(XR_PLATFORM_WINDOWS)
    constexpr UINT CP_1251 = 1251;
    const std::wstring wide = ToWide(str);
    const int cpLen = WideCharToMultiByte(CP_1251, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (cpLen <= 0)
        return xr_string();

    xr_string result;
    result.resize(cpLen);
    WideCharToMultiByte(CP_1251, 0, wide.c_str(), -1, result.data(), cpLen, nullptr, nullptr);
    return result;
#else
    return Utf8ToCP1251(str);
#endif
}

xr_string Sanitize(pcstr str)
{
    xr_string result;
    if (!str)
        return result;

    while (*str)
    {
        size_t len = 0;
        const u32 cp = Decode(str, len);
        if (cp == REPLACEMENT_CHARACTER && len == 1 && SequenceLength(static_cast<u8>(*str)) != 1)
        {
            // Invalid sequence, insert replacement character
            result.push_back(static_cast<char>(0xEF));
            result.push_back(static_cast<char>(0xBF));
            result.push_back(static_cast<char>(0xBD));
            ++str;
        }
        else
        {
            result.append(str, len);
            str += len;
        }
    }

    return result;
}

xr_string FixTextInputEncoding(pcstr text)
{
    if (!text || !text[0])
        return xr_string();

    // If SDL already gives us valid UTF-8, nothing to fix.
    if (IsValid(text))
        return xr_string(text);

#if defined(XR_PLATFORM_WINDOWS)
    // SDL on Windows may deliver characters encoded in the current keyboard
    // layout's ANSI code page instead of UTF-8. Convert from that code page.
    const HKL hkl = GetKeyboardLayout(0);
    const LANGID langId = LOWORD(hkl);
    const LCID lcid = MAKELCID(langId, SORT_DEFAULT);

    char cpBuf[16] = {};
    if (GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, cpBuf, sizeof(cpBuf)))
    {
        const UINT cp = static_cast<UINT>(atoi(cpBuf));
        if (cp != 0)
        {
            const int wideLen = MultiByteToWideChar(cp, 0, text, -1, nullptr, 0);
            if (wideLen > 0)
            {
                std::wstring wide;
                wide.resize(wideLen);
                if (MultiByteToWideChar(cp, 0, text, -1, wide.data(), wideLen) > 0)
                    // FromWide() stops at the null terminator written by MultiByteToWideChar,
                    // so there is no need to resize it away (matches FromACP/FromCP1251).
                    return FromWide(wide.c_str());
            }
        }
    }
#else
    // Non-Windows: SDL text input is already UTF-8; return it unchanged.
    return xr_string(text);
#endif

    // Fallback: return sanitized UTF-8.
    return Sanitize(text);
}
} // namespace Utf8
} // namespace XRay
