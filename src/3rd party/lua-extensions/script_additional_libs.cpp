//RvP, 11.05.2014	Функционал из xrLuaFix
//#include "StdAfx.h"
#include "script_additional_libs.h"
#include <random>
#include "../../build_config_defines.h"

/******************** BIT ********************/
int ROL(int a, int n)
{
    int t1, t2;
    n = n % (sizeof(a) * 8);
    t1 = a << n;
    t2 = a >> (sizeof(a) * 8 - n);
    return t1 | t2;
}
int ROR(int a, int n)
{
    int t1, t2;
    n = n % (sizeof(a) * 8);
    t1 = a >> n;
    t2 = a << (sizeof(a) * 8 - n);
    return t1 | t2;
}

int bit_tobit(lua_State *L)
{
    LUA_INTEGER n = luaL_checkinteger(L, 1);
    BYTE len = sizeof(n) * 8;
    char *s = new char[len + 1];
    _itoa(n, s, 2);
    lua_pushfstring(L, s);
    return 1;
}

int bit_tohex(lua_State *L)
{
    LUA_INTEGER n = luaL_checkinteger(L, 1);
    BYTE len = sizeof(n) * 2;
    char *s = new char[len + 1];
    _itoa(n, s, 16);
    lua_pushfstring(L, s);
    return 1;
}

int bit_not(lua_State *L)
{
    LUA_INTEGER n = luaL_checkinteger(L, 1);
    lua_pushinteger(L, ~n);
    return 1;
}

int bit_and(lua_State *L)
{
    LUA_INTEGER a = luaL_checkinteger(L, 1);
    LUA_INTEGER b = luaL_checkinteger(L, 2);
    lua_pushinteger(L, a & b);
    return 1;
}

int bit_or(lua_State *L)
{
    LUA_INTEGER a = luaL_checkinteger(L, 1);
    LUA_INTEGER b = luaL_checkinteger(L, 2);
    lua_pushinteger(L, a | b);
    return 1;
}

int bit_xor(lua_State *L)
{
    LUA_INTEGER a = luaL_checkinteger(L, 1);
    LUA_INTEGER b = luaL_checkinteger(L, 2);
    lua_pushinteger(L, a ^ b);
    return 1;
}

int bit_rol(lua_State *L)
{
    LUA_INTEGER a = luaL_checkinteger(L, 1);
    LUA_INTEGER n = luaL_checkinteger(L, 2);
    lua_pushinteger(L, ROL(a, n));
    return 1;
}

int bit_ror(lua_State *L)
{
    LUA_INTEGER a = luaL_checkinteger(L, 1);
    LUA_INTEGER n = luaL_checkinteger(L, 2);
    lua_pushinteger(L, ROR(a, n));
    return 1;
}

int bit_lshift(lua_State *L)
{
    LUA_INTEGER a = luaL_checkinteger(L, 1);
    LUA_INTEGER n = luaL_checkinteger(L, 2);
    lua_pushinteger(L, a << n);
    return 1;
}

int bit_rshift(lua_State *L)
{
    LUA_INTEGER a = luaL_checkinteger(L, 1);
    LUA_INTEGER n = luaL_checkinteger(L, 2);
    lua_pushinteger(L, a >> n);
    return 1;
}

const struct luaL_Reg bit_funcs[] = {
    {"tobit", bit_tobit},
    {"tohex", bit_tohex},
    {"bnot", bit_not},
    {"band", bit_and},
    {"bor", bit_or},
    {"bxor", bit_xor},
    {"lshift", bit_lshift},
    {"rshift", bit_rshift},
    {"rol", bit_rol},
    {"ror", bit_ror},
    {NULL, NULL}
};

int open_bit(lua_State *L)
{
    luaL_register(L, "bit", bit_funcs);
    return 0;
}
/******************** BIT END ********************/

/******************** STRING ********************/
int str_trim(lua_State *L)
{
    const char *front;
    const char *end;
    size_t      size;
    front = luaL_checklstring(L, 1, &size);
    end = &front[size - 1];
    for (; size && isspace(*front); size--, front++)
        ;
    for (; size && isspace(*end); size--, end--)
        ;
    lua_pushlstring(L, front, (size_t) (end - front) + 1);
    return 1;
}

int str_trim_l(lua_State *L)
{
    const char *front;
    const char *end;
    size_t      size;
    front = luaL_checklstring(L, 1, &size);
    end = &front[size - 1];
    for (; size && isspace(*front); size--, front++)
        ;
    lua_pushlstring(L, front, (size_t) (end - front) + 1);
    return 1;
}

int str_trim_r(lua_State *L)
{
    const char *front;
    const char *end;
    size_t      size;
    front = luaL_checklstring(L, 1, &size);
    end = &front[size - 1];
    for (; size && isspace(*end); size--, end--)
        ;
    lua_pushlstring(L, front, (size_t) (end - front) + 1);
    return 1;
}

int str_trim_w(lua_State *L)
{
    int i = 0, d, n;
    const char *s = luaL_checkstring(L, 1);;
    while (s[i] == ' ') i++;
    n = i;
    while (s[i] != ' ' && s[i]) i++;
    d = i - n;
    lua_pushlstring(L, s + n, d);
    return 1;
}

const luaL_Reg strlib[] = {
    {"trim", str_trim},
    {"trim_l", str_trim_l},
    {"trim_r", str_trim_r},
    {"trim_w", str_trim_w},
    {NULL, NULL}
};

int open_string(lua_State *L)
{
    luaL_openlib(L, LUA_STRLIBNAME, strlib, 0);
    return 0;
}
/******************** STRING END ********************/

/******************** MATH ********************/
std::random_device ndrng;
std::mt19937 intgen;
std::uniform_real<float> float_random_01;

int gen_random_in_range(int a1, int a2)
{	//unsigned?
    std::uniform_int<> dist(a1, a2);
    return dist(intgen);
}

int math_randomseed(lua_State *L)
{
    switch (lua_gettop(L))
    {
    case 0:{
        intgen.seed(ndrng());
        break;
    }
    case 1:{
        DWORD seed_value = luaL_checkint(L, 1);
        intgen.seed(seed_value);
        break;
    }
    default: return luaL_error(L, "math_randomseed: wrong number of arguments");
    }
    return 0;
}

int math_random(lua_State *L)
{
    switch (lua_gettop(L))
    {
    case 0:{
        lua_pushnumber(L, float_random_01(intgen));
        break;
    }
    case 1:{
        int u = luaL_checkint(L, 1);
        luaL_argcheck(L, 1 <= u, 1, "interval is empty");
        lua_pushinteger(L, gen_random_in_range(1, u));
        break;
    }
    case 2:{
        int l = luaL_checkint(L, 1);
        int u = luaL_checkint(L, 2);
        luaL_argcheck(L, l <= u, 2, "interval is empty");
        lua_pushinteger(L, gen_random_in_range(l, u));
        break;
    }
    default: return luaL_error(L, "wrong number of arguments");
    }
    return 1;
}

const luaL_Reg mathlib[] = {
    {"random", math_random},
    {"randomseed", math_randomseed},
    {NULL, NULL}
};

int open_math(lua_State *L)
{
    luaL_openlib(L, LUA_MATHLIBNAME, mathlib, 0);
    return 0;
}
/******************** MATH END ********************/

/******************** TABLE ********************/
inline DWORD C_get_size(lua_State *L)
{
    int i = 0;
    lua_settop(L, 2);
    while (lua_next(L, 1))
    {
        ++i;
        lua_pop(L, 1);
    }
    return i;
}

int tab_keys(lua_State *L)
{
    int i = 1;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_newtable(L);
    lua_pushnil(L);
    while (lua_next(L, 1) != 0)
    {
        lua_pushinteger(L, i);
        ++i;
        lua_pushvalue(L, -3);
        lua_settable(L, 2);
        lua_pop(L, 1);
    }
    return 1;
}

int tab_values(lua_State *L)
{
    int i = 1;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_newtable(L);
    lua_pushnil(L);
    while (lua_next(L, 1) != 0)
    {
        lua_pushinteger(L, i);
        ++i;
        lua_pushvalue(L, -2);
        lua_settable(L, 2);
        lua_pop(L, 1);
    }
    return 1;
}

int get_size(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushinteger(L, C_get_size(L));
    return 1;
}

int get_random(lua_State *L)
{
    int i = C_get_size(L);
    int j = gen_random_in_range(1, i);
    i = 0;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 2);
    while (lua_next(L, 1))
    {
        ++i;
        if (i == j)
        {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            return 2;
        }
        lua_pop(L, 1);
    }
    return 0;
}

const luaL_Reg tab_funcs[] = {
    {"keys", tab_keys},
    {"values", tab_values},
    {"size", get_size},
    {"random", get_random},
    {NULL, NULL}
};

int open_table(lua_State *L)
{
    luaL_openlib(L, LUA_TABLIBNAME, tab_funcs, 0);
    return 0;
}
/******************** TABLE END ********************/
void open_additional_libs(lua_State *L)
{
#ifdef USE_LUAJIT_ONE
    open_bit(L);
    open_math(L);
#endif
    open_string(L);
    open_table(L);
}
