#include "pch.hpp"
#include "script_lua_helper.hpp"
#include "script_debugger.hpp"
#if defined(XR_PLATFORM_LINUX)
#include "SDL.h" // for xr_itoa
#endif

CDbgLuaHelper* CDbgLuaHelper::m_pThis = nullptr;
lua_State* CDbgLuaHelper::L = nullptr;

CDbgLuaHelper::CDbgLuaHelper(CScriptDebugger* d)
{
    m_debugger = d;
    m_pAr = nullptr;
    m_pThis = this;
}

CDbgLuaHelper::~CDbgLuaHelper() { m_pThis = nullptr; }
void CDbgLuaHelper::UnPrepareLua(lua_State* l, int idx) { lua_remove(l, idx); }
int CDbgLuaHelper::PrepareLua(lua_State* l)
{
    // call this function immediatly before calling lua_pcall.
    // returns index in stack for errorFunc
    //	return 0;
    lua_register(l, "DEBUGGER_ERRORMESSAGE", errormessageLua);
    lua_sethook(l, hookLua, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 0);
    int top = lua_gettop(l);
    lua_getglobal(l, "DEBUGGER_ERRORMESSAGE");
    lua_insert(l, top);
    return top;
}

void CDbgLuaHelper::PrepareLuaBind()
{
    luabind::set_pcall_callback([](lua_State* L) { lua_pushcfunction(L, CDbgLuaHelper::hookLuaBind); });

#if !XRAY_EXCEPTIONS
    luabind::set_error_callback(errormessageLuaBind);
#endif
}

int CDbgLuaHelper::OutputTop(lua_State* l)
{
    if (!m_pThis)
        return 0;
    m_pThis->debugger()->Write(luaL_checkstring(l, -1));
    m_pThis->debugger()->Write("\n");
    return 0;
}

#define LEVELS1 12 // size of the first part of the stack
#define LEVELS2 10 // size of the second part of the stack

void CDbgLuaHelper::errormessageLuaBind(lua_State* l)
{
    if (!m_pThis)
        return;
    L = l;
    char err_msg[8192];
    xr_sprintf(err_msg, "%s", lua_tostring(L, -1));
    m_pThis->debugger()->Write(err_msg);
    m_pThis->debugger()->Write("\n");
    m_pThis->debugger()->ErrorBreak();
    FATAL("LUABIND error");
}

int CDbgLuaHelper::errormessageLua(lua_State* l)
{
    if (!m_pThis)
        return 0;
    L = l;
    int level = 1; // skip level 0 (it's this function)
    int firstpart = 1; // still before eventual `...'
    lua_Debug ar;
    if (!lua_isstring(L, 1))
        return lua_gettop(L);
    lua_settop(L, 1);
    lua_pushliteral(L, "\n");
    lua_pushliteral(L, "stack traceback:\n");
    while (lua_getstack(L, level++, &ar))
    {
        char buff[10];
        if (level > LEVELS1 && firstpart)
        {
            // no more than `LEVELS2' more levels?
            if (!lua_getstack(L, level + LEVELS2, &ar))
                level--; // keep going
            else
            {
                lua_pushliteral(L, "       ...\n"); // too many levels
                while (lua_getstack(L, level + LEVELS2, &ar)) // find last levels
                    level++;
            }
            firstpart = 0;
            continue;
        }
        xr_sprintf(buff, "%4d-  ", level - 1);
        lua_pushstring(L, buff);
        lua_getinfo(L, "Snl", &ar);
        lua_pushfstring(L, "%s:", ar.short_src);
        if (ar.currentline > 0)
            lua_pushfstring(L, "%d:", ar.currentline);
        switch (*ar.namewhat)
        {
        case 'g': // global
        case 'l': // local
        case 'f': // field
        case 'm': // method
            lua_pushfstring(L, " in function `%s'", ar.name);
            break;
        default:
        {
            if (*ar.what == 'm') // main?
                lua_pushfstring(L, " in main chunk");
            else if (*ar.what == 'C') // C function?
                lua_pushfstring(L, "%s", ar.short_src);
            else
                lua_pushfstring(L, " in function <%s:%d>", ar.short_src, ar.linedefined);
        }
        }
        lua_pushliteral(L, "\n");
        lua_concat(L, lua_gettop(L));
    }
    lua_concat(L, lua_gettop(L));
    OutputTop(L);
    const char* szSource = nullptr;
    if (ar.source[0] == '@')
        szSource = ar.source + 1;
    m_pThis->debugger()->ErrorBreak(szSource, ar.currentline);
    FATAL("LUA error");
    return 0;
}

void CDbgLuaHelper::set_lua(lua_State* l)
{
    if (!m_pThis)
        return;
    m_pThis->L = l;
}

void CDbgLuaHelper::line_hook(lua_State* l, lua_Debug* ar)
{
    if (!m_pThis)
        return;
    lua_getinfo(L, "lnuS", ar);
    m_pThis->m_pAr = ar;
    if (ar->source[0] == '@')
        m_pThis->debugger()->LineHook(ar->source + 1, ar->currentline);
}

void CDbgLuaHelper::func_hook(lua_State* l, lua_Debug* ar)
{
    if (!m_pThis)
        return;
    lua_getinfo(L, "lnuS", ar);
    m_pThis->m_pAr = ar;
    const char* szSource = nullptr;
    if (ar->source[0] == '@')
        szSource = ar->source + 1;
    m_pThis->debugger()->FunctionHook(szSource, ar->currentline, ar->event == LUA_HOOKCALL);
}

void print_stack(lua_State* L)
{
    Msg(" ");
    for (int i = 0; lua_type(L, -i - 1); i++)
        Msg("%2d : %s", -i - 1, lua_typename(L, lua_type(L, -i - 1)));
}

int CDbgLuaHelper::hookLuaBind(lua_State* l)
{
    if (!m_pThis)
        return LUA_OK; // XXX: Is it correct to return LUA_OK?
    L = l;
    int top1 = lua_gettop(L);
    Msg("hookLuaBind start");
    print_stack(L);
    if (lua_isstring(L, -1))
        errormessageLuaBind(L);
    // Msg("Tope string %s", lua_tostring(L,-1));
    lua_Debug ar;
    lua_getstack(L, 0, &ar);
    lua_getinfo(L, "lnuS", &ar);
    hookLua(L, &ar);
    Msg("hookLuaBind end");
    print_stack(L);
    if (lua_isstring(L, -1))
        Msg("Tope string %s", lua_tostring(L, -1));
    int top2 = lua_gettop(L);
    VERIFY(top2 == top1);
    return LUA_OK; // XXX: Probably, we should show message asking what value we should return.
}

void CDbgLuaHelper::hookLua(lua_State* l, lua_Debug* ar)
{
    if (!m_pThis)
        return;
    L = l;
    int top1 = lua_gettop(L);
    // Msg("hookLua start");
    // print_stack(L);
    switch (ar->event)
    {
    case LUA_HOOKTAILRET:
    case LUA_HOOKRET:
    case LUA_HOOKCALL: func_hook(L, ar); break;
    case LUA_HOOKLINE: line_hook(L, ar); break;
    }
    // Msg("hookLua end");
    // print_stack(L);
    int top2 = lua_gettop(L);
    VERIFY(top2 == top1);
}

const char* CDbgLuaHelper::GetSource() { return m_pAr->source + 1; }
void CDbgLuaHelper::DrawStackTrace()
{
    debugger()->ClearStackTrace();
    int nLevel = 0;
    lua_Debug ar;
    char szDesc[256];
    while (lua_getstack(L, nLevel, &ar))
    {
        lua_getinfo(L, "lnuS", &ar);
        if (ar.source[0] == '@')
        {
            szDesc[0] = '\0';
            /* if ( ar.name )
                xr_strcat(szDesc, ar.name);
            xr_strcat(szDesc, ",");
            if (ar.namewhat)
            xr_strcat(szDesc, ar.namewhat);
            xr_strcat(szDesc, ",");
            if (ar.what)
                xr_strcat(szDesc, ar.what);
            xr_strcat(szDesc, ",");
            */
            if (ar.name)
            {
                xr_strcat(szDesc, ar.name);
                xr_strcat(szDesc, " ");
            }
            char szTmp[6];
            xr_strcat(szDesc, xr_itoa(ar.currentline, szTmp, 10));
            xr_strcat(szDesc, " ");
            if (ar.short_src)
                xr_strcat(szDesc, ar.short_src);
            debugger()->AddStackTrace(szDesc, ar.source + 1, ar.currentline);
        }
        nLevel++;
    }
}

void CDbgLuaHelper::DrawLocalVariables()
{
    debugger()->ClearLocalVariables();
    int nLevel = debugger()->GetStackTraceLevel();
    lua_Debug ar;
    if (lua_getstack(L, nLevel, &ar))
    {
        int i = 1;
        const char* name;
        while (name = lua_getlocal(L, &ar, i++), name)
        {
            DrawVariable(L, name, true);
            lua_pop(L, 1); // remove variable value
        }
    }
}

void CDbgLuaHelper::DrawGlobalVariables()
{
    debugger()->ClearGlobalVariables();
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_pushnil(L); // first key
    string1024 var;
    var[0] = 0;
    while (lua_next(L, -2))
    {
        //!!!!	TRACE2("%s - %s\n",	lua_typename(L, lua_type(L, -2)), lua_typename(L, lua_type(L, -1)));
        //		xr_sprintf(var, "%s-%s",	lua_typename(L, lua_type(L, -2)), lua_typename(L, lua_type(L, -1)) );
        //		CScriptDebugger::GetDebugger()->AddLocalVariable(var, "global", "_g_");
        lua_pop(L, 1); // pop value, keep key for next iteration;
    }
    lua_pop(L, 1); // pop table of globals;
}

bool CDbgLuaHelper::GetCalltip(const char* szWord, char* szCalltip, int sz_calltip)
{
    int nLevel = debugger()->GetStackTraceLevel();
    lua_Debug ar;
    if (lua_getstack(L, nLevel, &ar))
    {
        int i = 1;
        const char* name;
        while (name = lua_getlocal(L, &ar, i++), name)
        {
            if (!xr_strcmp(name, szWord))
            {
                char szRet[64];
                Describe(szRet, -1, sizeof(szRet));
                xr_sprintf(szCalltip, sz_calltip, "local %s : %s ", name, szRet);
                lua_pop(L, 1); // remove variable value
                return true;
            }
            lua_pop(L, 1); // remove variable value
        }
    }
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_pushnil(L); // first key
    while (lua_next(L, -2))
    {
        const char* name = lua_tostring(L, -2);
        if (!xr_strcmp(name, szWord))
        {
            char szRet[64];
            Describe(szRet, -1, sizeof(szRet));
            xr_sprintf(szCalltip, sz_calltip, "global %s : %s ", name, szRet);
            lua_pop(L, 3); // remove table, key, value
            return true;
        }
        lua_pop(L, 1); // pop value, keep key for next iteration
    }
    lua_pop(L, 1); // pop table of globals
    return false;
}

bool CDbgLuaHelper::Eval(const char* szCode, char* szRet, int szret_size)
{
    CoverGlobals();
    int top = lua_gettop(L);
    int status = luaL_loadbuffer(L, szCode, xr_strlen(szCode), szCode);
    if (status)
        xr_sprintf(szRet, szret_size, "%s", luaL_checkstring(L, -1));
    else
    {
        status = lua_pcall(L, 0, LUA_MULTRET, 0); // call main
        if (status)
        {
            const char* szErr = luaL_checkstring(L, -1);
            const char* szErr2 = strstr(szErr, ": ");
            xr_sprintf(szRet, szret_size, "%s", szErr2 ? szErr2 + 2 : szErr);
        }
        else
            Describe(szRet, -1, szret_size);
    }
    lua_settop(L, top);
    RestoreGlobals();
    return !status;
}

void CDbgLuaHelper::Describe(char* szRet, int nIndex, int szRet_size)
{
    int ntype = lua_type(L, nIndex);
    const char* type = lua_typename(L, ntype);
    char value[64];
    switch (ntype)
    {
    case LUA_TNUMBER: xr_sprintf(value, "%f", lua_tonumber(L, nIndex)); break;
    case LUA_TSTRING: xr_sprintf(value, "%.63s", lua_tostring(L, nIndex)); break;
    case LUA_TBOOLEAN: xr_sprintf(value, "%s", lua_toboolean(L, nIndex) ? "true" : "false"); break;
    default: value[0] = '\0'; break;
    }
    xr_sprintf(szRet, szRet_size, "%s : %.64s", type, value);
}

void CDbgLuaHelper::CoverGlobals()
{
    lua_newtable(L); // save there globals covered by locals
    int nLevel = debugger()->GetStackTraceLevel();
    lua_Debug ar;
    if (lua_getstack(L, nLevel, &ar))
    {
        int i = 1;
        const char* name;
        while (name = lua_getlocal(L, &ar, i++), name)
        { // SAVE lvalue
            lua_pushstring(L, name); // SAVE lvalue name
            lua_pushvalue(L, -1); // SAVE lvalue name name
            lua_pushvalue(L, -1); // SAVE lvalue name name name
            lua_insert(L, -4); // SAVE name lvalue name name
            lua_rawget(L, LUA_GLOBALSINDEX); // SAVE name lvalue name gvalue
            lua_rawset(L, -5); // save global value in local table
            // SAVE name lvalue
            lua_rawset(L, LUA_GLOBALSINDEX); // SAVE
        }
    }
}

void CDbgLuaHelper::RestoreGlobals()
{
    // there is table of covered globals on top
    lua_pushnil(L); // first key
    // SAVE nil
    while (lua_next(L, -2)) // SAVE key value
    {
        lua_pushvalue(L, -2); // SAVE key value key
        lua_insert(L, -2); // SAVE key key value
        lua_rawset(L, LUA_GLOBALSINDEX); // restore global
        // SAVE key
    }
    lua_pop(L, 1); // pop table of covered globals;
}

void CDbgLuaHelper::DrawVariable(lua_State* l, const char* name, bool bOpenTable)
{
    Variable var;
    xr_strcpy(var.szName, name);
    const char* type;
    int ntype = lua_type(l, -1);
    type = lua_typename(l, ntype);
    xr_strcpy(var.szType, type);
    char value[64];
    switch (ntype)
    {
    case LUA_TNUMBER:
        xr_sprintf(value, "%f", lua_tonumber(l, -1));
        xr_strcpy(var.szValue, value);
        break;
    case LUA_TBOOLEAN:
        xr_sprintf(value, "%s", lua_toboolean(L, -1) ? "true" : "false");
        xr_strcpy(var.szValue, value);
        break;
    case LUA_TSTRING:
        xr_sprintf(value, "%.63s", lua_tostring(l, -1));
        xr_strcpy(var.szValue, value);
        break;
    case LUA_TTABLE:
        var.szValue[0] = 0;
        debugger()->AddLocalVariable(var);
        if (bOpenTable)
            DrawTable(l, name, false);
        return;
    /*
    case LUA_TUSERDATA:
    {
        luabind::detail::object_rep* obj = static_cast<luabind::detail::object_rep*>(lua_touserdata(L, -1));
        luabind::detail::lua_reference& r = obj->get_lua_table();
        lua_State * ls = NULL;
        r.get(ls);
        DrawTable(ls, name);
        return;
    }
    */
    default: value[0] = 0; break;
    }
    debugger()->AddLocalVariable(var);
}

void CDbgLuaHelper::DrawTable(lua_State* l, LPCSTR S, bool bRecursive)
{
    // char str[1024];
    if (!lua_istable(l, -1))
        return;
    lua_pushnil(l); // first key
    while (lua_next(l, -2))
    {
        char stype[256];
        char sname[256];
        char sFullName[256];
        xr_sprintf(stype, "%s", lua_typename(l, lua_type(l, -1)));
        xr_sprintf(sname, "%s", lua_tostring(l, -2));
        xr_sprintf(sFullName, "%s.%s", S, sname);
        DrawVariable(l, sFullName, false);
        lua_pop(l, 1); // removes `value'; keeps `key' for next iteration
    }
}

void CDbgLuaHelper::DrawVariableInfo(char* varName) {}
