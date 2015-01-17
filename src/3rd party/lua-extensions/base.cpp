#include "../../build_config_defines.h"

#ifdef USE_LUAJIT_ONE
#pragma comment(lib, "LuaJIT-1.1.8.lib")
#else
#pragma comment(lib, "lua51.lib")
#endif //-USE_LUAJIT_ONE

#include "lua.hpp"

//#pragma comment(lib, "xrCore.lib")
//#pragma comment(lib, "user32.lib")

extern "C"{
    #include "lfs.h"
    #include "lmarshal.h"
}

//#include "Libs.h"
#include "script_additional_libs.h"

extern "C" __declspec(dllexport) int luaopen_lua_extensions(lua_State *L){
    //luaopen_debug(L);

    open_additional_libs(L);

    luaopen_lfs(L);
    //open_string(L);
    //open_math(L);
    //open_table(L);
    luaopen_marshal(L);
    //open_kb(L);
    //open_log(L); 

    return 0;
}