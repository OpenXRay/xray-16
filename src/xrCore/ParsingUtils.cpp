#include "stdafx.h"

#include "ParsingUtils.hpp"

ParseIncludeResult ParseInclude(pstr string, pcstr& out_include_name)
{
    VERIFY(string);

    // Skip any whitespace characters
    string = ParseAllSpaces(string);

    // Check for #include
    static constexpr pcstr IncludeTag = "#include";
    if (std::strncmp(string, IncludeTag, 8) != 0)
        return ParseIncludeResult::NoInclude;

    string += 8;

    // Skip any whitespace characters
    string = ParseAllSpaces(string);

    // Check that after the tag there is a quote
    if (*string != '\"')
        return ParseIncludeResult::Error;

    // Mark the start of the include name
    ++string;
    out_include_name = string;

    string = ParseUntil(string, '\"');

    // Check for unterminated or empty include name
    if (*string == '\0' || out_include_name == string)
        return ParseIncludeResult::Error;

    // Check for unreasonably long include names
    const size_t size = string - out_include_name;
    if (size > 1024)
        return ParseIncludeResult::Error;

    // NOTE(Andre): Yes this might look scary but it's perfectly fine. Since the include name is already in the string
    // we are parsing and its not used afterwards we simply replace the closing quote with a null byte and we have a
    // valid c-string pointed to by 'out_include_name' and safe ourselves the need to copy the string.
    *string = '\0';

    return ParseIncludeResult::Success;
}

pcstr ParseAllSpaces(pcstr string)
{
    VERIFY(string);

    while (*string != '\0' && std::isspace(*string))
        ++string;

    return string;
}

pstr ParseAllSpaces(pstr string) { return const_cast<pstr>(ParseAllSpaces(reinterpret_cast<pcstr>(string))); }

pcstr ParseUntil(pcstr string, const char character)
{
    VERIFY(string);

    while (*string != '\0' && *string != character)
        ++string;

    return string;
}

pstr ParseUntil(pstr string, const char character)
{
    return const_cast<pstr>(ParseUntil(reinterpret_cast<pcstr>(string), character));
}

void StringCopyLowercase(pstr destination, pcstr src, std::size_t size)
{
    VERIFY(destination);
    VERIFY(src);

    for (std::size_t i = 0; *src != '\0' && i < size; ++i)
    {
        *destination = std::tolower(*src);
        ++src;
        ++destination;
    }

    // Ensure the string is null-terminated
    *destination = '\0';
}

void StringCopyLowercase(pstr destination, shared_str src, std::size_t size)
{
    StringCopyLowercase(destination, *src, size);
}
