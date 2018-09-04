#pragma once
#ifndef NET_AUTH_CHECK_INCLUDED
#define NET_AUTH_CHECK_INCLUDED

#include "NET_Shared.h"

using xr_auth_strings_t = xr_vector<shared_str>;
void XRNETSERVER_API fill_auth_check_params(xr_auth_strings_t& ignore, xr_auth_strings_t& check);
bool XRNETSERVER_API allow_to_include_path(xr_auth_strings_t const& ignore, pcstr path);

#endif //#ifndef NET_AUTH_CHECK_INCLUDED
