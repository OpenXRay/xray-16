#pragma once

#include "xr_types.h"
#include "xrstring.h"

enum class ParseIncludeResult
{
    Success,   /// There is a valid #include and 'out_include_name' contains the filename
    Error,     /// There is a #include but there is some problem
    NoInclude, /// There is no #include on this line
};

// Given a string of the form: '#include "filename"' we try to parse filename into 'out_include_name'
// Note that the file name is parsed inplace to avoid copying the string
ParseIncludeResult ParseInclude(pstr string, pcstr& out_include_name);

// Starting from the beginning of the string skips all characters for which 'std::isspace' is 'true'.
// Returns the first position where 'std::isspace' is 'false'.
pcstr ParseAllSpaces(pcstr string);

pstr ParseAllSpaces(pstr string);

// Starting from the begging of the string skips all characters until 'character' is found
// or until the end of the string is reached.
// Returns the first position where 'character' is found or the end of string if 'character' is not found
pcstr ParseUntil(pcstr string, const char character);

pstr ParseUntil(pstr string, const char character);

// Copies 'size' characters from 'src' to 'destination' and converts it to lowercase
void StringCopyLowercase(pstr destination, pcstr src, std::size_t size);

void StringCopyLowercase(pstr destination, shared_str src, std::size_t size);
