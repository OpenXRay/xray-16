#pragma once

#include "../LuaJIT-1.1.8/etc/lua.hpp"
#include <cstdlib>
#include <cctype>

typedef unsigned long DWORD;
typedef unsigned char BYTE;

void open_additional_libs(lua_State*);