#pragma once

struct XRCORE_API xr_token
{
    xr_token(): name(nullptr), id(-1) {}
    xr_token(const pcstr _name, const int _id) : name(_name), id(_id) {}

    pcstr name;
    int id;
};

XRCORE_API pcstr get_token_name(const xr_token* tokens, int key);
XRCORE_API int get_token_id(const xr_token* tokens, pcstr key);
