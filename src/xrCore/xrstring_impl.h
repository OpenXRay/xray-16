#pragma once

#include "xr_types.h"

#pragma pack(push, 4)
#pragma warning(push)
#pragma warning(disable : 4200)
struct str_value
{
    u32 dwReference;
    u32 dwLength;
    u32 dwCRC;
    str_value* next;
    char value[];
};
#pragma warning(pop)
#pragma pack(pop)

