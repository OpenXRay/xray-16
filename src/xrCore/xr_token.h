#pragma once

struct alignas(alignof(pcstr)) xr_token
{
    xr_token(): name(nullptr), id(-1) {}
    xr_token(const pcstr _name, const int _id) : name(_name), id(_id) {}

    pcstr name;
    int id;
};
static_assert(sizeof(xr_token) == sizeof(pcstr) * 2, "xr_token should be aligned, otherwise it may have problems on RISC (e.g. ARM) architectures, which require aligned pointers.");

XRCORE_API pcstr get_token_name(const xr_token* tokens, int key);
XRCORE_API int get_token_id(const xr_token* tokens, pcstr key);
