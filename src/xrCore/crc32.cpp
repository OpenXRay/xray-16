#include "stdafx.h"
#pragma hdrstop

#include <array>

// Reflects CRC bits in the lookup table
constexpr u32 reflect(u32 ref, char ch) noexcept
{
    // Used only by Init_CRC32_Table().

    u32 value(0);

    // Swap bit 0 for bit 7
    // bit 1 for bit 6, etc.
    for (int i = 1; i < (ch + 1); i++)
    {
        if (ref & 1)
            value |= 1 << (ch - i);
        ref >>= 1;
    }
    return value;
}

constexpr std::array<u32, 256> generate_crc32_lookup_table() noexcept
{
    std::array<u32, 256> crc32_table{};

    // This is the official polynomial used by CRC-32
    // in PKZip, WinZip and Ethernet.
    u32 ulPolynomial = 0x04c11db7;

    // 256 values representing ASCII character codes.
    for (int i = 0; i <= 0xFF; i++)
    {
        crc32_table[i] = reflect(i, 8) << 24;
        for (int j = 0; j < 8; j++)
            crc32_table[i] = (crc32_table[i] << 1) ^ (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
        crc32_table[i] = reflect(crc32_table[i], 32);
    }

    return crc32_table;
}

// Lookup table array
static constexpr auto crc32_table = generate_crc32_lookup_table();

u32 crc32(const void* P, u32 len)
{
    // Pass a text string to this function and it will return the CRC.

    // Once the lookup table has been filled in by the two functions above,
    // this function creates all CRCs using only the lookup table.

    // Be sure to use unsigned variables,
    // because negative values introduce high bits
    // where zero bits are required.

    // Start out with all bits set high.
    u32 ulCRC = 0xffffffff;
    u8* buffer = (u8*)P;

    // Perform the algorithm on each character
    // in the string, using the lookup table values.
    while (len--)
        ulCRC = (ulCRC >> 8) ^ crc32_table[(ulCRC & 0xFF) ^ *buffer++];

    // Exclusive OR the result with the beginning value.
    return ulCRC ^ 0xffffffff;
}

u32 crc32(const void* P, u32 len, u32 starting_crc)
{
    u32 ulCRC = 0xffffffff ^ starting_crc;
    u8* buffer = (u8*)P;

    while (len--)
        ulCRC = (ulCRC >> 8) ^ crc32_table[(ulCRC & 0xFF) ^ *buffer++];

    return ulCRC ^ 0xffffffff;
}

u32 path_crc32(const char* path, u32 len)
{
    u32 ulCRC = 0xffffffff;
    u8* buffer = (u8*)path;

    while (len--)
    {
        const u8 c = *buffer;
        if (c != '/' && c != _DELIMITER)
        {
            ulCRC = (ulCRC >> 8) ^ crc32_table[(ulCRC & 0xFF) ^ *buffer];
        }

        ++buffer;
    }

    return ulCRC ^ 0xffffffff;
}
