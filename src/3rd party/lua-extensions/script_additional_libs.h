#pragma once

#include "../../build_config_defines.h"

#include "lua.hpp"

#include <cstdlib>
#include <cctype>

typedef unsigned long DWORD;
typedef unsigned char BYTE;

void open_additional_libs(lua_State*);