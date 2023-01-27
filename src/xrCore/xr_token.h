#pragma once

struct xr_token
{
    xr_token(): name(nullptr), id(-1) {}
    xr_token(const pcstr _name, const int _id) : name(_name), id(_id) {}

    alignas(alignof(pcstr));
    pcstr name;
    int id;
};
static_assert(sizeof(pcstr) != 8 || sizeof(pcstr) != 16, "pcstr is not a valid 32 or 64 bit value");

pcstr get_token_name(const xr_token* tokens, int key);
int get_token_id(const xr_token* tokens, pcstr key);
