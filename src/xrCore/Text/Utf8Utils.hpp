#pragma once

#include "xrCore/xrCore.h"
#include "xrCommon/xr_string.h"

// Basic UTF-8 utilities. Supports only valid UTF-8 sequences (BMP is enough for our needs).
// All functions assume null-terminated strings unless noted otherwise.

namespace XRay
{
namespace Utf8
{
// Returns true if the byte is a valid UTF-8 continuation byte (10xxxxxx).
inline bool IsContinuationByte(u8 byte) { return (byte & 0xC0) == 0x80; }

// Returns true if the byte is a valid UTF-8 leading byte.
inline bool IsLeadingByte(u8 byte)
{
    return (byte & 0x80) == 0x00 || // 0xxxxxxx (ASCII)
        (byte & 0xE0) == 0xC0 || // 110xxxxx
        (byte & 0xF0) == 0xE0 || // 1110xxxx
        (byte & 0xF8) == 0xF0; // 11110xxx
}

// Returns the number of bytes in the UTF-8 sequence starting with the given byte.
// Returns 0 for continuation bytes and invalid leading bytes.
XRCORE_API size_t SequenceLength(u8 leadingByte);

// Returns pointer to the start of the next UTF-8 codepoint.
// If the current byte is not a leading byte, advances one byte forward (safe fallback).
XRCORE_API pcstr Next(pcstr pos);

// Returns pointer to the start of the previous UTF-8 codepoint.
// start must be the beginning of the string. Returns start if pos == start.
XRCORE_API pcstr Prev(pcstr start, pcstr pos);

// Decodes one UTF-8 codepoint starting at pos.
// Returns the codepoint value and writes the number of bytes into outLength.
// Returns REPLACEMENT_CHARACTER (U+FFFD) and outLength=1 on invalid sequence.
XRCORE_API u32 Decode(pcstr pos, size_t& outLength);

// Counts the number of UTF-8 codepoints in the string.
XRCORE_API size_t LengthCodepoints(pcstr str);

// Advances the pointer by the given number of codepoints.
// Stops at the end of the string. Returns pos if count is 0 or pos is null/empty.
XRCORE_API pcstr Advance(pcstr pos, size_t codepoints);

// Counts the number of UTF-8 codepoints between start and end (end must be >= start).
XRCORE_API size_t DistanceCodepoints(pcstr start, pcstr end);

// Returns true if the entire string is valid UTF-8.
XRCORE_API bool IsValid(pcstr str);

// Strips UTF-8 BOM (EF BB BF) from the beginning of the string if present.
XRCORE_API pcstr StripBom(pcstr str);

// ASCII-only lowercase. Does not touch non-ASCII bytes, so it is safe for UTF-8.
XRCORE_API void ToLowerAscii(pstr str);

// ASCII-only uppercase. Does not touch non-ASCII bytes, so it is safe for UTF-8.
XRCORE_API void ToUpperAscii(pstr str);

// Converts UTF-8 string to UTF-16 (std::wstring). Useful for WinAPI interop.
XRCORE_API std::wstring ToWide(pcstr str);

// Converts UTF-16 string (wchar_t*) to UTF-8.
XRCORE_API xr_string FromWide(const wchar_t* str);

// Converts a string from the system default ANSI code page to UTF-8.
XRCORE_API xr_string FromACP(pcstr str);

// Converts a string from Windows-1251 (legacy game scripts/text) to UTF-8.
XRCORE_API xr_string FromCP1251(pcstr str);

// Converts a UTF-8 string to Windows-1251.
XRCORE_API xr_string ToCP1251(pcstr str);

// Maps a single CP1251 byte (0x00..0xFF) to its Unicode codepoint.
// Used by the font atlas builder to re-key glyphs of legacy Windows-1251 TTFs.
XRCORE_API u32 CP1251ByteToCodepoint(u8 byte);

// Replaces invalid UTF-8 bytes with U+FFFD replacement character.
XRCORE_API xr_string Sanitize(pcstr str);

// Windows: SDL_TEXTINPUT may return characters encoded in the current keyboard layout's
// ANSI code page instead of UTF-8. This helper detects invalid UTF-8 and converts from
// the layout's code page. On non-Windows it returns the input unchanged.
XRCORE_API xr_string FixTextInputEncoding(pcstr text);

constexpr u32 REPLACEMENT_CHARACTER = 0xFFFD;
constexpr u32 MAX_BMP_CODEPOINT = 0xFFFF;
} // namespace Utf8
} // namespace XRay
