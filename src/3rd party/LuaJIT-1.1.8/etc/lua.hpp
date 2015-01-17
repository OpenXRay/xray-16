// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

extern "C" {
#include "../src/lua.h"
#include "../src/lualib.h"
#include "../src/lauxlib.h"
}
