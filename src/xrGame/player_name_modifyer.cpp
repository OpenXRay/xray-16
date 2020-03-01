#include "StdAfx.h"

LPCSTR modify_player_name(LPCSTR src_name, string256& dest)
{
    xr_strcpy(dest, src_name);
    static const char* denied_symbols = DELIMITER "?%%\"";
    size_t tmp_length = xr_strlen(dest);
    size_t start_pos = 0;
    size_t char_pos;
    while ((char_pos = strcspn(dest + start_pos, denied_symbols)) < (tmp_length - start_pos))
    {
        char_pos += start_pos;
        dest[char_pos] = '_';
        ++start_pos;
    }
    return dest;
}
