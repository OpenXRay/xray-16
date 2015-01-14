#pragma once

#include "../../build_config_defines.h"
#ifdef USE_LUAJIT_ONE
#include "../LuaJIT-1.1.8/etc/lua.hpp"
#else
#include "../luajit-2.0/src/lua.hpp"
#endif
#include <cstdlib>
#include <cctype>

typedef unsigned long DWORD;
typedef unsigned char BYTE;

void open_additional_libs(lua_State*);