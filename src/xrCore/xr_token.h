#pragma once

#pragma pack(push, 1)
struct XRCORE_API xr_token
{
    xr_token(): name(nullptr), id(-1) {}
    xr_token(const pcstr _name, const int _id) : name(_name), id(_id) {}

    pcstr name;
    int id;
};
#pragma pack(pop)

XRCORE_API pcstr get_token_name(const xr_token* tokens, int key);
XRCORE_API int get_token_id(const xr_token* tokens, pcstr key);
