#pragma once
#include <string>
#include <cstdint>

namespace xray::text
{
// Windows-1251 names used by the original Russian game UI; native paths are UTF-8.
inline constexpr uint16_t cp1251[] = {
    0x402, 0x403, 0x201a, 0x453, 0x201e, 0x2026, 0x2020, 0x2021,
    0x20ac, 0x2030, 0x409, 0x2039, 0x40a, 0x40c, 0x40b, 0x40f,
    0x452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0xfffd, 0x2122, 0x459, 0x203a, 0x45a, 0x45c, 0x45b, 0x45f,
    0xa0, 0x40e, 0x45e, 0x408, 0xa4, 0x490, 0xa6, 0xa7,
    0x401, 0xa9, 0x404, 0xab, 0xac, 0xad, 0xae, 0x407,
    0xb0, 0xb1, 0x406, 0x456, 0x491, 0xb5, 0xb6, 0xb7,
    0x451, 0x2116, 0x454, 0xbb, 0x458, 0x405, 0x455, 0x457,
    0x410, 0x411, 0x412, 0x413, 0x414, 0x415, 0x416, 0x417,
    0x418, 0x419, 0x41a, 0x41b, 0x41c, 0x41d, 0x41e, 0x41f,
    0x420, 0x421, 0x422, 0x423, 0x424, 0x425, 0x426, 0x427,
    0x428, 0x429, 0x42a, 0x42b, 0x42c, 0x42d, 0x42e, 0x42f,
    0x430, 0x431, 0x432, 0x433, 0x434, 0x435, 0x436, 0x437,
    0x438, 0x439, 0x43a, 0x43b, 0x43c, 0x43d, 0x43e, 0x43f,
    0x440, 0x441, 0x442, 0x443, 0x444, 0x445, 0x446, 0x447,
    0x448, 0x449, 0x44a, 0x44b, 0x44c, 0x44d, 0x44e, 0x44f,
};

inline bool next_utf8(const std::string& value, size_t& at, uint32_t& code)
{
    const auto first = static_cast<unsigned char>(value[at++]);
    if (first < 128) { code = first; return true; }
    const unsigned count = first >= 0xc2 && first <= 0xdf ? 1 :
        first >= 0xe0 && first <= 0xef ? 2 : first >= 0xf0 && first <= 0xf4 ? 3 : 0;
    if (!count || at + count > value.size()) return false;
    code = first & ((1u << (6 - count)) - 1);
    for (unsigned i = 0; i < count; ++i)
    {
        const auto byte = static_cast<unsigned char>(value[at++]);
        if ((byte & 0xc0) != 0x80) return false;
        code = (code << 6) | (byte & 0x3f);
    }
    return code >= (count == 1 ? 0x80u : count == 2 ? 0x800u : 0x10000u) &&
        code <= 0x10ffff && !(code >= 0xd800 && code <= 0xdfff);
}

inline bool valid_utf8(const std::string& value)
{
    size_t at = 0;
    uint32_t code;
    while (at < value.size()) if (!next_utf8(value, at, code)) return false;
    return true;
}

inline std::string filename_to_utf8(const std::string& value)
{
    if (valid_utf8(value)) return value;
    std::string result;
    for (unsigned char byte : value)
    {
        const uint32_t code = byte < 128 ? byte : cp1251[byte - 128];
        if (code < 128) result += static_cast<char>(code);
        else if (code < 0x800)
        {
            result += static_cast<char>(0xc0 | (code >> 6));
            result += static_cast<char>(0x80 | (code & 63));
        }
        else
        {
            result += static_cast<char>(0xe0 | (code >> 12));
            result += static_cast<char>(0x80 | ((code >> 6) & 63));
            result += static_cast<char>(0x80 | (code & 63));
        }
    }
    return result;
}

inline std::string filename_from_utf8(const std::string& value)
{
    std::string result;
    size_t at = 0;
    uint32_t code;
    while (at < value.size())
    {
        if (!next_utf8(value, at, code)) return value;
        if (code < 128) result += static_cast<char>(code);
        else
        {
            unsigned index = 0;
            while (index < 128 && cp1251[index] != code) ++index;
            if (index == 128 || code == 0xfffd) return value;
            result += static_cast<char>(index + 128);
        }
    }
    return result;
}
}
