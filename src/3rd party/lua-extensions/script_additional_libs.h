#pragma once

#include "../luajit-2.0/src/lua.hpp"
#include <cstdlib>
#include <cctype>

typedef unsigned long DWORD;
typedef unsigned char BYTE;

void open_additional_libs(lua_State*);