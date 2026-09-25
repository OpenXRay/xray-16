#include <cstdio>
#include <cstring>
#include <lua.hpp>

static bool unwound = false;

static int raiseError(lua_State* state)
{
    struct Scope
    {
        ~Scope()
        {
            unwound = true;
        }
    } scope;
    return luaL_error(state, "unwind fixture");
}

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    lua_State* state = luaL_newstate();
    if (!state)
        return 1;
    luaL_openlibs(state);
    lua_pushcfunction(state, raiseError);
    lua_setglobal(state, "raise_error");
    int result = luaL_dofile(state, argv[1]);
    if (result)
        std::fprintf(stderr, "%s\n", lua_tostring(state, -1));
    lua_settop(state, 0);
    if (result || !unwound)
    {
        lua_close(state);
        return 1;
    }
    lj_allow_escape_sequences(0);
    result = luaL_dostring(state, "return '\\q'");
    bool permissive = !result && std::strcmp(lua_tostring(state, -1), "q") == 0;
    lua_settop(state, 0);
    lj_allow_escape_sequences(1);
    bool strict = luaL_loadstring(state, "return '\\q'") != 0;
    lua_settop(state, 0);
    int timed = lua_gc(state, LUA_GCTIMEOUT, 1000);
    lua_gc(state, LUA_GCRESTART, 0);
    lua_gc(state, LUA_GCCOLLECT, 0);
    lua_close(state);
    return permissive && strict && (timed == 0 || timed == 1) ? 0 : 1;
}
