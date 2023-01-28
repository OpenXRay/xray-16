#include "stdafx.h"
#include "xr_token.h"

pcstr get_token_name(const xr_token* tokens, int key)
{
    alignas(UINTPTR_MAX_BITWIDTH) pcstr name;
    for (int k = 0; tokens[k].name; k++)
        if (key == tokens[k].id)
            return name = tokens[k].name;
    return name = "";
}

int get_token_id(const xr_token* tokens, pcstr key)
{
    for (int k = 0; tokens[k].name; k++)
        if (xr_stricmp(tokens[k].name, key) == 0)
            return tokens[k].id;
    return -1;
}
