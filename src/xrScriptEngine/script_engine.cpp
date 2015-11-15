////////////////////////////////////////////////////////////////////////////
//  Module      : script_engine.cpp
//  Created     : 01.04.2004
//  Modified    : 01.04.2004
//  Author      : Dmitriy Iassenev
//  Description : XRay Script Engine
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "script_engine.hpp"
#include "script_process.hpp"
#include "script_thread.hpp"
#include "xrCore/doug_lea_allocator.h"
#ifndef DEBUG
#include "opt.lua.h"
#include "opt_inline.lua.h"
#endif
#include "Include/xrAPI/xrAPI.h"
#include "ScriptExporter.hpp"
#ifdef USE_DEBUGGER
#include "script_debugger.hpp"
#endif
#include <stdarg.h>
#include <boost/noncopyable.hpp>

Flags32 g_LuaDebug;

#define SCRIPT_GLOBAL_NAMESPACE "_G"

static const char *file_header_old =
"local function script_name() \
return \"%s\" \
end \
local this = {} \
%s this %s \
setmetatable(this, {__index = " SCRIPT_GLOBAL_NAMESPACE "}) \
setfenv(1, this) ";

static const char *file_header_new =
"local function script_name() \
return \"%s\" \
end \
local this = {} \
this." SCRIPT_GLOBAL_NAMESPACE " = " SCRIPT_GLOBAL_NAMESPACE " \
%s this %s \
setfenv(1, this) ";

static const char *file_header = nullptr;

#ifdef PURE_ALLOC
static void *lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    (void)ud;
    (void)osize;
    if (!nsize)
    {
        xr_free(ptr);
        return nullptr;
    }
#ifdef DEBUG_MEMORY_NAME
    return Memory.mem_realloc(ptr, nsize, "LUA");
#else
    return Memory.mem_realloc(ptr, nsize);
#endif
}
#else

#include "xrCore/memory_allocator_options.h"

#ifdef USE_ARENA_ALLOCATOR
static const u32 s_arena_size = 96*1024*1024;
static char s_fake_array[s_arena_size];
static doug_lea_allocator s_allocator(s_fake_array, s_arena_size, "lua");
#else
static doug_lea_allocator s_allocator(nullptr, 0, "lua");
#endif

static void *lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
#ifndef USE_MEMORY_MONITOR
    (void)ud;
    (void)osize;
    if (!nsize)
    {
        s_allocator.free_impl(ptr);
        return 0;
    }    
    if (!ptr)
        return s_allocator.malloc_impl((u32)nsize);    
    return s_allocator.realloc_impl(ptr, (u32)nsize);
#else
    if (!nsize)
    {
        memory_monitor::monitor_free(ptr);
        s_allocator.free_impl(ptr);
        return nullptr;
    }
    if (!ptr)
    {
        void *result = s_allocator.malloc_impl((u32)nsize);
        memory_monitor::monitor_alloc(result, nsize, "LUA");
        return result;
    }
    memory_monitor::monitor_free(ptr);
    void *result = s_allocator.realloc_impl(ptr, (u32)nsize);
    memory_monitor::monitor_alloc(result, nsize, "LUA");
    return result;
#endif
}

#endif // PURE_ALLOC

static void *__cdecl luabind_allocator(luabind::memory_allocation_function_parameter,
    const void *pointer, size_t const size)
{
    if (!size)
    {
        void *non_const_pointer = const_cast<LPVOID>(pointer);
        xr_free(non_const_pointer);
        return nullptr;
    }
    if (!pointer)
    {
#ifdef DEBUG
        return Memory.mem_alloc(size, "luabind");
#else
        return Memory.mem_alloc(size);
#endif
    }
    void *non_const_pointer = const_cast<LPVOID>(pointer);
#ifdef DEBUG
    return Memory.mem_realloc(non_const_pointer, size, "luabind");
#else
    return Memory.mem_realloc(non_const_pointer, size);
#endif
}

// ---- start of LuaJIT extensions
static void l_message(lua_State *state, const char *msg)
{ Msg("! [LUA_JIT] %s", msg); }

static int report(lua_State *L, int status)
{
    if (status && !lua_isnil(L, -1))
    {
        const char *msg = lua_tostring(L, -1);
        if (!msg)
            msg = "(error object is not a string)";
        l_message(L, msg);
        lua_pop(L, 1);
    }
    return status;
}

static int loadjitmodule(lua_State *L, const char *notfound)
{
    lua_getglobal(L, "require");
    lua_pushliteral(L, "jit.");
    lua_pushvalue(L, -3);
    lua_concat(L, 2);
    if (lua_pcall(L, 1, 1, 0))
    {
        const char *msg = lua_tostring(L, -1);
        if (msg && !strncmp(msg, "module ", 7))
        {
            l_message(L, notfound);
            return 1;
        }
        return report(L, 1);
    }
    lua_getfield(L, -1, "start");
    lua_remove(L, -2); // drop module table
    return 0;
}

// JIT engine control command: try jit library first or load add-on module
static int dojitcmd(lua_State *L, const char *cmd)
{
    const char *val = strchr(cmd, '=');
    lua_pushlstring(L, cmd, val ? val-cmd : xr_strlen(cmd));
    lua_getglobal(L, "jit"); // get jit.* table
    lua_pushvalue(L, -2);
    lua_gettable(L, -2); // lookup library function
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2); // drop non-function and jit.* table, keep module name
        if (loadjitmodule(L, "unknown luaJIT command"))
            return 1;
    }
    else
        lua_remove(L, -2); // drop jit.* table
    lua_remove(L, -2); // drop module name
    if (val)
        lua_pushstring(L, val + 1);
    return report(L, lua_pcall(L, val ? 1 : 0, 0, 0));
}

void jit_command(lua_State *state, LPCSTR command)
{ dojitcmd(state, command); }

#ifndef DEBUG
// start optimizer
static int dojitopt(lua_State *L, const char *opt)
{
    lua_pushliteral(L, "opt");
    if (loadjitmodule(L, "LuaJIT optimizer module not installed"))
        return 1;
    lua_remove(L, -2); // drop module name
    if (*opt)
        lua_pushstring(L, opt);
    return report(L, lua_pcall(L, *opt ? 1 : 0, 0, 0));
}
#endif
// ---- end of LuaJIT extensions

#ifndef DEBUG
static void put_function(lua_State* state, const u8 *buffer, u32 buffer_size, LPCSTR package_id)
{
    lua_getglobal(state, "package");
    lua_pushstring(state, "preload");
    lua_gettable(state, -2);
    lua_pushstring(state, package_id);
    luaL_loadbuffer(state, (char*)buffer, buffer_size, package_id);
    lua_settable(state, -3);
}
#endif

const char *const CScriptEngine::GlobalNamespace = SCRIPT_GLOBAL_NAMESPACE;
Lock CScriptEngine::stateMapLock;
xr_hash_map<lua_State*, CScriptEngine*> *CScriptEngine::stateMap = nullptr;

string4096 CScriptEngine::g_ca_stdout;

void CScriptEngine::reinit()
{
    stateMapLock.Enter();
    if (!stateMap)
    {
        stateMap = xr_new<xr_hash_map<lua_State*, CScriptEngine*>>();
        stateMap->reserve(32); // 32 lua states should be enough
    }
    stateMapLock.Leave();
    if (m_virtual_machine)
    {
        lua_close(m_virtual_machine);
        UnregisterState(m_virtual_machine);
    }
    m_virtual_machine = lua_newstate(lua_alloc, nullptr);
    if (!m_virtual_machine)
    {
        Msg("! ERROR : Cannot initialize script virtual machine!");
        return;
    }
    RegisterState(m_virtual_machine, this);
    // initialize lua standard library functions 
    struct luajit
    {
        static void open_lib(lua_State *L, pcstr module_name, lua_CFunction function)
        {
            lua_pushcfunction(L, function);
            lua_pushstring(L, module_name);
            lua_call(L, 1, 0);
        }
    };

    luajit::open_lib(lua(), "", luaopen_base);
    luajit::open_lib(lua(), LUA_LOADLIBNAME, luaopen_package);
    luajit::open_lib(lua(), LUA_TABLIBNAME, luaopen_table);
    luajit::open_lib(lua(), LUA_IOLIBNAME, luaopen_io);
    luajit::open_lib(lua(), LUA_OSLIBNAME, luaopen_os);
    luajit::open_lib(lua(), LUA_MATHLIBNAME, luaopen_math);
    luajit::open_lib(lua(), LUA_STRLIBNAME, luaopen_string);
#ifdef DEBUG
    luajit::open_lib(lua(), LUA_DBLIBNAME, luaopen_debug);
#endif
    if (!strstr(Core.Params, "-nojit"))
    {
        luajit::open_lib(lua(), LUA_JITLIBNAME, luaopen_jit);
#ifndef DEBUG
        put_function(lua(), opt_lua_binary, sizeof(opt_lua_binary), "jit.opt");
        put_function(lua(), opt_inline_lua_binary, sizeof(opt_lua_binary), "jit.opt_inline");
        dojitopt(lua(), "2");
#endif
    }
    if (strstr(Core.Params, "-_g"))
        file_header = file_header_new;
    else
        file_header = file_header_old;
}

int CScriptEngine::vscript_log(LuaMessageType luaMessageType, LPCSTR caFormat, va_list marker)
{
#ifdef DEBUG
    if (!g_LuaDebug.test(1) && luaMessageType!=LuaMessageType::Error)
        return 0;
    LPCSTR S = "", SS = "";
    LPSTR S1;
    string4096 S2;
    switch (luaMessageType)
    {
    case LuaMessageType::Info:
        S = "* [LUA] ";
        SS = "[INFO]        ";
        break;
    case LuaMessageType::Error:
        S = "! [LUA] ";
        SS = "[ERROR]       ";
        break;
    case LuaMessageType::Message:
        S = "[LUA] ";
        SS = "[MESSAGE]     ";
        break;
    case LuaMessageType::HookCall:
        S = "[LUA][HOOK_CALL] ";
        SS = "[CALL]        ";
        break;
    case LuaMessageType::HookReturn:
        S = "[LUA][HOOK_RETURN] ";
        SS = "[RETURN]      ";
        break;
    case LuaMessageType::HookLine:
        S = "[LUA][HOOK_LINE] ";
        SS = "[LINE]        ";
        break;
    case LuaMessageType::HookCount:
        S = "[LUA][HOOK_COUNT] ";
        SS = "[COUNT]       ";
        break;
    case LuaMessageType::HookTailReturn:
        S = "[LUA][HOOK_TAIL_RETURN] ";
        SS = "[TAIL_RETURN] ";
        break;
    default:
        NODEFAULT;
    }
    xr_strcpy(S2, S);
    S1 = S2 + xr_strlen(S);
    int l_iResult = vsprintf(S1, caFormat, marker);
    Msg("%s", S2);
    xr_strcpy(S2, SS);
    S1 = S2 + xr_strlen(SS);
    vsprintf(S1, caFormat, marker);
    xr_strcat(S2, "\r\n");
    m_output.w(S2, xr_strlen(S2));
    return l_iResult;
#else
    return 0;
#endif
}

#ifdef DEBUG
void CScriptEngine::print_stack()
{
    if (!m_stack_is_ready)
        return;
    m_stack_is_ready = false;
    lua_State *L = lua();
    lua_Debug l_tDebugInfo;
    for (int i = 0; lua_getstack(L, i, &l_tDebugInfo); i++)
    {
        lua_getinfo(L, "nSlu", &l_tDebugInfo);
        if (!l_tDebugInfo.name)
        {
            script_log(LuaMessageType::Error, "%2d : [%s] %s(%d) : %s",
                i, l_tDebugInfo.what, l_tDebugInfo.short_src, l_tDebugInfo.currentline, "");
        }
        else if (!xr_strcmp(l_tDebugInfo.what, "C"))
            script_log(LuaMessageType::Error, "%2d : [C  ] %s", i, l_tDebugInfo.name);
        else
        {
            script_log(LuaMessageType::Error, "%2d : [%s] %s(%d) : %s",
                i, l_tDebugInfo.what, l_tDebugInfo.short_src, l_tDebugInfo.currentline, l_tDebugInfo.name);
        }
    }
}
#endif

int CScriptEngine::script_log(LuaMessageType message, LPCSTR caFormat, ...)
{
    va_list marker;
    va_start(marker,caFormat);
    int result = vscript_log(message, caFormat, marker);
    va_end(marker);
#ifdef DEBUG
    if (message==LuaMessageType::Error && !logReenterability)
    {
        logReenterability = true;
        print_stack();
        logReenterability = false;
    }
#endif
    return result;
}

bool CScriptEngine::parse_namespace(LPCSTR caNamespaceName, LPSTR b, u32 b_size, LPSTR c, u32 c_size)
{
    *b = 0;
    *c = 0;
    LPSTR S2;
    STRCONCAT(S2, caNamespaceName);
    LPSTR S = S2;
    for (int i = 0;; i++)
    {
        if (!xr_strlen(S))
        {
            script_log(LuaMessageType::Error, "the namespace name %s is incorrect!", caNamespaceName);
            return false;
        }
        LPSTR S1 = strchr(S, '.');
        if (S1)
            *S1 = 0;
        if (i)
            xr_strcat(b, b_size, "{");
        xr_strcat(b, b_size, S);
        xr_strcat(b, b_size, "=");
        if (i)
            xr_strcat(c, c_size, "}");
        if (S1)
            S = ++S1;
        else
            break;
    }
    return true;
}

bool CScriptEngine::load_buffer(lua_State *L, LPCSTR caBuffer, size_t tSize, LPCSTR caScriptName, LPCSTR caNameSpaceName)
{
    int l_iErrorCode;
    if (caNameSpaceName && xr_strcmp(GlobalNamespace, caNameSpaceName))
    {
        string512 insert, a, b;
        LPCSTR header = file_header;
        if (!parse_namespace(caNameSpaceName, a, sizeof(a), b, sizeof(b)))
            return false;
        xr_sprintf(insert, header, caNameSpaceName, a, b);
        u32 str_len = xr_strlen(insert);
        u32 const total_size = str_len+tSize;
        LPSTR script = nullptr;
        bool dynamic_allocation = false;
        __try
        {
            if (total_size < 768 * 1024)
                script = (LPSTR)_alloca(total_size);
            else
            {
#ifdef DEBUG
                script = (LPSTR)Memory.mem_alloc(total_size, "lua script file");
#else
                script = (LPSTR)Memory.mem_alloc(total_size);
#endif
                dynamic_allocation = true;
            }
        }
        __except (GetExceptionCode()==STATUS_STACK_OVERFLOW)
        {
            int errcode = _resetstkoflw();
            R_ASSERT2(errcode, "Could not reset the stack after \"Stack overflow\" exception!");
#ifdef DEBUG
            script = (LPSTR)Memory.mem_alloc(total_size, "lua script file (after exception)");
#else
            script = (LPSTR)Memory.mem_alloc(total_size);
#endif
            dynamic_allocation = true;
        }
        xr_strcpy(script, total_size, insert);
        CopyMemory(script+str_len, caBuffer, u32(tSize));
        l_iErrorCode = luaL_loadbuffer(L, script, tSize+str_len, caScriptName);
        if (dynamic_allocation)
            xr_free(script);
    }
    else
        l_iErrorCode = luaL_loadbuffer(L, caBuffer, tSize, caScriptName);
    if (l_iErrorCode)
    {
#ifdef DEBUG
        print_output(L, caScriptName, l_iErrorCode);
#endif
        on_error(L);
        return false;
    }
    return true;
}

bool CScriptEngine::do_file(LPCSTR caScriptName, LPCSTR caNameSpaceName)
{
    int start = lua_gettop(lua());
    string_path l_caLuaFileName;
    IReader *l_tpFileReader = FS.r_open(caScriptName);
    if (!l_tpFileReader)
    {
        script_log(LuaMessageType::Error, "Cannot open file \"%s\"", caScriptName);
        return false;
    }
    strconcat(sizeof(l_caLuaFileName), l_caLuaFileName, "@", caScriptName);
    if (!load_buffer(lua(), static_cast<LPCSTR>(l_tpFileReader->pointer()), (size_t)l_tpFileReader->length(), l_caLuaFileName, caNameSpaceName))
    {
        //VERIFY(lua_gettop(lua())>=4);
        //lua_pop(lua(), 4);
        //VERIFY(lua_gettop(lua())==start-3);
        lua_settop(lua(), start);
        FS.r_close(l_tpFileReader);
        return false;
    }
    FS.r_close(l_tpFileReader);
    int errFuncId = -1;
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    if (debugger())
        errFuncId = debugger()->PrepareLua(lua());
#endif
#endif
    if (0) //.
    {
        for (int i = 0; lua_type(lua(), -i - 1); i++)
            Msg("%2d : %s", -i - 1, lua_typename(lua(), lua_type(lua(), -i - 1)));
    }
    // because that's the first and the only call of the main chunk - there is no point to compile it
    // luaJIT_setmode(lua(), 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_OFF); // Oles
    int l_iErrorCode = lua_pcall(lua(), 0, 0, (-1 == errFuncId) ? 0 : errFuncId); // new_Andy
    // luaJIT_setmode(lua(), 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_ON); // Oles
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    if (debugger())
        debugger()->UnPrepareLua(lua(), errFuncId);
#endif
#endif
    if (l_iErrorCode)
    {
#ifdef DEBUG
        print_output(lua(), caScriptName, l_iErrorCode);
#endif
        on_error(lua());
        lua_settop(lua(), start);
        return false;
    }
    return true;
}

bool CScriptEngine::load_file_into_namespace(LPCSTR caScriptName, LPCSTR caNamespaceName)
{
    int start = lua_gettop(lua());
    if (!do_file(caScriptName, caNamespaceName))
    {
        lua_settop(lua(), start);
        return false;
    }
    VERIFY(lua_gettop(lua())==start);
    return true;
}

bool CScriptEngine::namespace_loaded(LPCSTR name, bool remove_from_stack)
{
    int start = lua_gettop(lua());
    lua_pushstring(lua(), GlobalNamespace);
    lua_rawget(lua(), LUA_GLOBALSINDEX);
    string256 S2;
    xr_strcpy(S2, name);
    LPSTR S = S2;
    for (;;)
    {
        if (!xr_strlen(S))
        {
            VERIFY(lua_gettop(lua())>=1);
            lua_pop(lua(), 1);
            VERIFY(start==lua_gettop(lua()));
            return false;
        }
        LPSTR S1 = strchr(S, '.');
        if (S1)
            *S1 = 0;
        lua_pushstring(lua(), S);
        lua_rawget(lua(), -2);
        if (lua_isnil(lua(), -1))
        {
            // lua_settop(lua(), 0);
            VERIFY(lua_gettop(lua())>=2);
            lua_pop(lua(), 2);
            VERIFY(start==lua_gettop(lua()));
            return false; // there is no namespace!
        }
        else if (!lua_istable(lua(),-1))
        {
            // lua_settop(lua(), 0);
            VERIFY(lua_gettop(lua())>=1);
            lua_pop(lua(),1);
            VERIFY(start==lua_gettop(lua()));
            FATAL(" Error : the namespace name is already being used by the non-table object!\n");
            return false;
        }
        lua_remove(lua(), -2);
        if (S1)
            S = ++S1;
        else
            break;
    }
    if (!remove_from_stack)
        VERIFY(lua_gettop(lua())==start+1);
    else
    {
        VERIFY(lua_gettop(lua())>=1);
        lua_pop(lua(), 1);
        VERIFY(lua_gettop(lua())==start);
    }
    return true;
}

bool CScriptEngine::object(LPCSTR identifier, int type)
{
    int start = lua_gettop(lua());
    lua_pushnil(lua());
    while (lua_next(lua(), -2))
    {
        if (lua_type(lua(), -1)==type && !xr_strcmp(identifier,lua_tostring(lua(), -2)))
        {
            VERIFY(lua_gettop(lua())>=3);
            lua_pop(lua(), 3);
            VERIFY(lua_gettop(lua())==start-1);
            return true;
        }
        lua_pop(lua(), 1);
    }
    VERIFY(lua_gettop(lua())>=1);
    lua_pop(lua(), 1);
    VERIFY(lua_gettop(lua())==start-1);
    return false;
}

bool CScriptEngine::object(LPCSTR namespace_name, LPCSTR identifier, int type)
{
    int start = lua_gettop(lua());
    if (xr_strlen(namespace_name) && !namespace_loaded(namespace_name, false))
    {
        VERIFY(lua_gettop(lua())==start);
        return false;
    }
    bool result = object(identifier, type);
    VERIFY(lua_gettop(lua())==start);
    return result;
}

luabind::object CScriptEngine::name_space(LPCSTR namespace_name)
{
    string256 S1;
    xr_strcpy(S1, namespace_name);
    LPSTR S = S1;
    luabind::object lua_namespace = luabind::get_globals(lua());
    for (;;)
    {
        if (!xr_strlen(S))
            return lua_namespace;
        LPSTR I = strchr(S, '.');
        if (!I)
            return lua_namespace[S];
        *I = 0;
        lua_namespace = lua_namespace[S];
        S = I + 1;
    }
}

struct raii_guard : private boost::noncopyable
{
    CScriptEngine *scriptEngine;
    int m_error_code;
    const char *m_error_description;

    raii_guard(CScriptEngine *scriptEngine, int error_code, const char *error_description)
    {
        this->scriptEngine = scriptEngine;
        m_error_code = error_code;
        m_error_description = error_description;
    }
    
    ~raii_guard()
    {
#ifdef DEBUG
        bool lua_studio_connected = !!scriptEngine->debugger();
        if (!lua_studio_connected)
#endif
        {
#ifdef DEBUG
            static const bool break_on_assert = !!strstr(Core.Params, "-break_on_assert");
#else
            static const bool break_on_assert = true;
#endif
            if (!m_error_code)
                return;
            if (break_on_assert)
            R_ASSERT2(!m_error_code, m_error_description);
            else
                Msg("! SCRIPT ERROR: %s", m_error_description);
        }
    }
};

bool CScriptEngine::print_output(lua_State *L, LPCSTR caScriptFileName, int errorCode)
{
    CScriptEngine *scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);
    if (errorCode)
        print_error(L, errorCode);
    LPCSTR S = "see call_stack for details!";
    raii_guard guard(scriptEngine, errorCode, S);
    if (!lua_isstring(L, -1))
        return false;
    S = lua_tostring(L,-1);
    if (!xr_strcmp(S, "cannot resume dead coroutine"))
    {
        VERIFY2("Please do not return any values from main!!!", caScriptFileName);
#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
        if (debugger() && debugger()->Active())
        {
            debugger()->Write(S);
            debugger()->ErrorBreak();
        }
#endif
    }
    else
    {
        if (!errorCode)
            scriptEngine->script_log(LuaMessageType::Info, "Output from %s", caScriptFileName);
        scriptEngine->script_log(errorCode ? LuaMessageType::Error : LuaMessageType::Message, "%s", S);
#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
        if (debugger() && debugger()->Active())
        {
            debugger()->Write(S);
            debugger()->ErrorBreak();
        }
#endif
    }
    return true;
}

void CScriptEngine::print_error(lua_State *L, int iErrorCode)
{
    CScriptEngine *scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);
    switch (iErrorCode)
    {
    case LUA_ERRRUN:
        scriptEngine->script_log(LuaMessageType::Error, "SCRIPT RUNTIME ERROR");
        break;
    case LUA_ERRMEM:
        scriptEngine->script_log(LuaMessageType::Error, "SCRIPT ERROR (memory allocation)");
        break;
    case LUA_ERRERR:
        scriptEngine->script_log(LuaMessageType::Error, "SCRIPT ERROR (while running the error handler function)");
        break;
    case LUA_ERRFILE:
        scriptEngine->script_log(LuaMessageType::Error, "SCRIPT ERROR (while running file)");
        break;
    case LUA_ERRSYNTAX:
        scriptEngine->script_log(LuaMessageType::Error, "SCRIPT SYNTAX ERROR");
        break;
    case LUA_YIELD:
        scriptEngine->script_log(LuaMessageType::Info, "Thread is yielded");
        break;
    default:
        NODEFAULT;
    }
}

#ifdef DEBUG
void CScriptEngine::flush_log()
{
    string_path log_file_name;
    strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, "_lua.log");
    FS.update_path(log_file_name, "$logs$", log_file_name);
    m_output.save_to(log_file_name);
}
#endif

int CScriptEngine::error_log(LPCSTR format, ...)
{
    va_list marker;
    va_start(marker, format);
    LPCSTR S = "! [LUA][ERROR] ";
    LPSTR S1;
    string4096 S2;
    xr_strcpy(S2, S);
    S1 = S2+xr_strlen(S);
    int result = vsprintf(S1, format, marker);
    va_end(marker);
    Msg("%s", S2);
    return result;
}

#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
#include "script_debugger.hpp"
#else
#include "LuaStudio/LuaStudio.hpp"
typedef cs::lua_studio::create_world_function_type create_world_function_type;
typedef cs::lua_studio::destroy_world_function_type destroy_world_function_type;
static create_world_function_type s_create_world = nullptr;
static destroy_world_function_type s_destroy_world = nullptr;
static HMODULE s_script_debugger_handle = nullptr;
static LogCallback s_old_log_callback = nullptr;
#endif
#endif

void jit_command(lua_State *, LPCSTR);

#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
static void log_callback(void *context, const char *message)
{
    if (s_old_log_callback)
        s_old_log_callback(message);
    CScriptEngine *scriptEngine = (CScriptEngine*)context;
    if (!scriptEngine->debugger())
        return;
    scriptEngine->debugger()->add_log_line(message);
}

void CScriptEngine::initialize_lua_studio(lua_State *state, cs::lua_studio::world *&world, lua_studio_engine *&engine)
{
    engine = 0;
    world = 0;
    u32 const old_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);
    s_script_debugger_handle = LoadLibrary(CS_LUA_STUDIO_BACKEND_FILE_NAME);
    SetErrorMode(old_error_mode);
    if (!s_script_debugger_handle)
    {
        Msg("! cannot load %s dynamic library", CS_LUA_STUDIO_BACKEND_FILE_NAME);
        return;
    }
    R_ASSERT2(s_script_debugger_handle, "can't load script debugger library");
    s_create_world = (create_world_function_type)
        GetProcAddress(s_script_debugger_handle, "_cs_lua_studio_backend_create_world@12");
    R_ASSERT2(s_create_world, "can't find function \"cs_lua_studio_backend_create_world\"");
    s_destroy_world = (destroy_world_function_type)
        GetProcAddress(s_script_debugger_handle, "_cs_lua_studio_backend_destroy_world@4");
    R_ASSERT2 (s_destroy_world, "can't find function \"cs_lua_studio_backend_destroy_world\" in the library");
    engine = xr_new<lua_studio_engine>();
    world = s_create_world(*engine, false, false);
    VERIFY(world);
    s_old_log_callback = SetLogCB(LogCallback(log_callback, this));
    jit_command(state, "debug=2");
    jit_command(state, "off");
    world->add(state);
}

void CScriptEngine::finalize_lua_studio(lua_State *state, cs::lua_studio::world *&world, lua_studio_engine *&engine)
{
    world->remove(state);
    VERIFY (world);
    s_destroy_world(world);
    world = nullptr;
    VERIFY (engine);
    xr_delete(engine);
    FreeLibrary(s_script_debugger_handle);
    s_script_debugger_handle = nullptr;
    SetLogCB(s_old_log_callback);
}

void CScriptEngine::try_connect_to_debugger()
{
    if (m_lua_studio_world)
        return;
    initialize_lua_studio(lua(), m_lua_studio_world, m_lua_studio_engine);
}

void CScriptEngine::disconnect_from_debugger()
{
    if (!m_lua_studio_world)
        return;
    finalize_lua_studio(lua(), m_lua_studio_world, m_lua_studio_engine);
}
#endif

CScriptEngine::CScriptEngine()
{
    luabind::allocator = &luabind_allocator;
    luabind::allocator_parameter = nullptr;
    m_current_thread = nullptr;
#ifdef DEBUG
    m_stack_is_ready = false;
#endif
    m_virtual_machine = nullptr;
    m_stack_level = 0;
    m_reload_modules = false;
    m_last_no_file_length = 0;
    *m_last_no_file = 0;
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    STATIC_CHECK(false, Do_Not_Define_USE_LUA_STUDIO_macro_without_USE_DEBUGGER_macro);
    m_scriptDebugger = nullptr;
    restartDebugger();  
#else
    m_lua_studio_world = nullptr;
    m_lua_studio_engine = nullptr;
#endif
#endif
}

CScriptEngine::~CScriptEngine()
{
    if (m_virtual_machine)
        lua_close(m_virtual_machine);
    while (!m_script_processes.empty())
        remove_script_process(m_script_processes.begin()->first);
#ifdef DEBUG
    flush_log();
#endif
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    xr_delete(m_scriptDebugger);
#else
    disconnect_from_debugger();
#endif
#endif
}

void CScriptEngine::unload()
{
    lua_settop(lua(), m_stack_level);
    m_last_no_file_length = 0;
    *m_last_no_file = 0;
}

int CScriptEngine::lua_panic(lua_State *L)
{
    print_output(L, "PANIC", LUA_ERRRUN);
    return 0;
}

void CScriptEngine::lua_error(lua_State *L)
{
    print_output(L, "", LUA_ERRRUN);
    on_error(L);
#if !XRAY_EXCEPTIONS 
    Debug.fatal(DEBUG_INFO, "LUA error: %s", lua_tostring(L, -1));
#else
    throw lua_tostring(L,-1);
#endif
}

int CScriptEngine::lua_pcall_failed(lua_State *L)
{
    print_output(L, "", LUA_ERRRUN);
    on_error(L);
#if !XRAY_EXCEPTIONS
    Debug.fatal(DEBUG_INFO, "LUA error: %s", lua_isstring(L, -1) ? lua_tostring(L, -1) : "");
#endif
    if (lua_isstring(L, -1))
        lua_pop (L, 1);
    return LUA_ERRRUN;
}

void CScriptEngine::lua_cast_failed(lua_State *L, LUABIND_TYPE_INFO info)
{
    print_output(L, "", LUA_ERRRUN);
    Debug.fatal(DEBUG_INFO, "LUA error: cannot cast lua value to %s", info->name());
}

void CScriptEngine::setup_callbacks()
{
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    if (debugger())
        debugger()->PrepareLuaBind();
#endif
#endif
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    if (!debugger() || !debugger()->Active())
#endif
#endif
    {
#if !XRAY_EXCEPTIONS 
        luabind::set_error_callback(CScriptEngine::lua_error);
#endif
#ifndef MASTER_GOLD
        luabind::set_pcall_callback(CScriptEngine::lua_pcall_failed);
#endif
    }
#if !XRAY_EXCEPTIONS 
    luabind::set_cast_failed_callback(CScriptEngine::lua_cast_failed);
#endif
    lua_atpanic(lua(), CScriptEngine::lua_panic);
}

#ifdef DEBUG
#include "script_thread.hpp"

void CScriptEngine::lua_hook_call(lua_State *L, lua_Debug *dbg)
{
    CScriptEngine *scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);
    if (scriptEngine->current_thread())
        scriptEngine->current_thread()->script_hook(L, dbg);
    else
        scriptEngine->m_stack_is_ready = true;
}
#endif

int CScriptEngine::auto_load(lua_State *L)
{
    if (lua_gettop(L)<2 || !lua_istable(L, 1) || !lua_isstring(L, 2))
    {
        lua_pushnil(L);
        return 1;
    }
    CScriptEngine *scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);
    scriptEngine->process_file_if_exists(lua_tostring(L, 2), false);
    lua_rawget(L, 1);
    return 1;
}

void CScriptEngine::setup_auto_load()
{
    luaL_newmetatable(lua(), "XRAY_AutoLoadMetaTable");
    lua_pushstring(lua(), "__index");
    lua_pushcfunction (lua(), CScriptEngine::auto_load);
    lua_settable(lua(), -3);
    lua_pushstring(lua(), GlobalNamespace);
    lua_gettable(lua(), LUA_GLOBALSINDEX);
    luaL_getmetatable (lua(), "XRAY_AutoLoadMetaTable");
    lua_setmetatable(lua(), -2);
    //. ??????????
    // lua_settop(lua(), 0);
}

void CScriptEngine::init(ExporterFunc exporterFunc, bool loadGlobalNamespace)
{
#ifdef USE_LUA_STUDIO
    bool lua_studio_connected = !!m_lua_studio_world;
    if (lua_studio_connected)
        m_lua_studio_world->remove(lua());
#endif
    reinit();
#ifdef USE_LUA_STUDIO
    if (m_lua_studio_world || strstr(Core.Params, "-lua_studio"))
    {
        if (!lua_studio_connected)
            try_connect_to_debugger();
        else
        {
            jit_command(lua(), "debug=2");
            jit_command(lua(), "off");
            m_lua_studio_world->add(lua());
        }
    }
#endif
    luabind::open(lua());
    setup_callbacks();
    if (exporterFunc)
        exporterFunc(lua());
    setup_auto_load();
#ifdef DEBUG
    m_stack_is_ready = true;
#endif
#if defined(DEBUG) && !defined(USE_LUA_STUDIO)
#if defined(USE_DEBUGGER)
    if (!debugger() || !debugger()->Active())
#endif
        lua_sethook(lua(), CScriptEngine::lua_hook_call, LUA_MASKLINE|LUA_MASKCALL|LUA_MASKRET, 0);
#endif
    if (loadGlobalNamespace)
    {
        bool save = m_reload_modules;
        m_reload_modules = true;
        process_file_if_exists(GlobalNamespace, false);
        m_reload_modules = save;
    }
    m_stack_level = lua_gettop(lua());
    setvbuf(stderr, g_ca_stdout, _IOFBF, sizeof(g_ca_stdout));
}

void CScriptEngine::remove_script_process(const ScriptProcessor &process_id)
{
    CScriptProcessStorage::iterator I = m_script_processes.find(process_id);
    if (I != m_script_processes.end())
    {
        xr_delete((*I).second);
        m_script_processes.erase(I);
    }
}

bool CScriptEngine::load_file(const char *scriptName, const char *namespaceName)
{
    if (!process_file(scriptName))
        return false;
    string1024 initializerName;
    xr_strcpy(initializerName, scriptName);
    xr_strcat(initializerName, "_initialize");
    if (object(namespaceName, initializerName, LUA_TFUNCTION))
    {
        // lua_dostring(lua(), xr_strcat(initializerName, "()"));
        luabind::functor<void> f;
        R_ASSERT(functor(initializerName, f));
        f();
    }
    return true;
}

bool CScriptEngine::process_file_if_exists(LPCSTR file_name, bool warn_if_not_exist)
{
    u32 string_length = xr_strlen(file_name);
    if (!warn_if_not_exist && no_file_exists(file_name, string_length))
        return false;
    string_path S, S1;
    if (m_reload_modules || *file_name && !namespace_loaded(file_name))
    {
        FS.update_path(S, "$game_scripts$", strconcat(sizeof(S1), S1, file_name, ".script"));
        if (!warn_if_not_exist && !FS.exist(S))
        {
#ifdef DEBUG
            if (false) // XXX: restore (check script engine flags)
            {
                print_stack();
                Msg("! WARNING: Access to nonexistent variable '%s' or loading nonexistent script '%s'",
                    file_name, S1);
                m_stack_is_ready = true;
            }
#endif
            add_no_file(file_name, string_length);
            return false;
        }
#ifndef MASTER_GOLD
        Msg("* Loading script: %s", S1);
#endif
        m_reload_modules = false;
        return load_file_into_namespace(S, *file_name ? file_name : GlobalNamespace);
    }
    return true;
}

bool CScriptEngine::process_file(LPCSTR file_name)
{ return process_file_if_exists(file_name, true); }

bool CScriptEngine::process_file(LPCSTR file_name, bool reload_modules)
{
    m_reload_modules = reload_modules;
    bool result = process_file_if_exists(file_name, true);
    m_reload_modules = false;
    return result;
}

bool CScriptEngine::function_object(LPCSTR function_to_call, luabind::object &object, int type)
{
    if (!xr_strlen(function_to_call))
        return false;
    string256 name_space, function;
    parse_script_namespace(function_to_call, name_space, sizeof(name_space), function, sizeof(function));
    if (xr_strcmp(name_space, GlobalNamespace))
    {
        LPSTR file_name = strchr(name_space, '.');
        if (!file_name)
            process_file(name_space);
        else
        {
            *file_name = 0;
            process_file(name_space);
            *file_name = '.';
        }
    }
    if (!this->object(name_space, function, type))
        return false;
    luabind::object lua_namespace = this->name_space(name_space);
    object = lua_namespace[function];
    return true;
}

#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
void CScriptEngine::stopDebugger()
{
    if (debugger())
    {
        xr_delete(m_scriptDebugger);
        Msg("Script debugger stopped.");
    }
    else
        Msg("Script debugger not present.");
}

void CScriptEngine::restartDebugger()
{
    if(debugger())
        stopDebugger();
    m_scriptDebugger = xr_new<CScriptDebugger>(this);
    debugger()->PrepareLuaBind();
    Msg("Script debugger restarted.");
}
#endif 

CScriptEngine *CScriptEngine::GetInstance(lua_State *state)
{
    CScriptEngine *instance = nullptr;
    stateMapLock.Enter();
    if (stateMap)
    {
        auto it = stateMap->find(state);
        if (it!=stateMap->end())
            instance = it->second;
    }
    stateMapLock.Leave();
    return instance;
}

bool CScriptEngine::RegisterState(lua_State *state, CScriptEngine *scriptEngine)
{
    bool result = false;
    stateMapLock.Enter();
    if (stateMap)
    {
        auto it = stateMap->find(state);
        if (it==stateMap->end())
        {
            stateMap->insert({state, scriptEngine});
            result = true;
        }
    }
    stateMapLock.Leave();
    return result;
}

bool CScriptEngine::UnregisterState(lua_State *state)
{
    if (!state)
        return true;
    bool result = false;
    stateMapLock.Enter();
    if (stateMap)
    {
        auto it = stateMap->find(state);
        if (it!=stateMap->end())
        {
            stateMap->erase(it);
            result = true;
        }
    }
    stateMapLock.Leave();
    return result;
}

bool CScriptEngine::no_file_exists(LPCSTR file_name, u32 string_length)
{
    if (m_last_no_file_length!=string_length)
        return false;
    return !memcmp(m_last_no_file, file_name, string_length);
}

void CScriptEngine::add_no_file(LPCSTR file_name, u32 string_length)
{
    m_last_no_file_length = string_length;
    CopyMemory(m_last_no_file, file_name, string_length+1);
}

void CScriptEngine::collect_all_garbage()
{
    lua_gc(lua(), LUA_GCCOLLECT, 0);
    lua_gc(lua(), LUA_GCCOLLECT, 0);
}

u32 CScriptEngine::GetMemoryUsage()
{
#ifdef USE_DL_ALLOCATOR
    return s_allocator.get_allocated_size();
#else
    return 0;
#endif
}

void CScriptEngine::on_error(lua_State *state)
{
    CScriptEngine *scriptEngine = GetInstance(state);
    VERIFY(scriptEngine);
#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
    if (!scriptEngine->debugger())
        return;
    scriptEngine->debugger()->on_error(state);
#endif
}

#ifdef DEBUG

static xr_string to_string(luabind::object const &o)
{
    using namespace luabind;
    if (o.type()==LUA_TSTRING)
        return object_cast<luabind::internal_string>(o).c_str();
    lua_State *L = o.lua_state();
    LUABIND_CHECK_STACK(L);
    if (o.type()==LUA_TNUMBER)
    {
        char buffer[_CVTBUFSIZE ];
        _gcvt_s(buffer, object_cast<float>(o), 16);
        return buffer;
    }
    return xr_string("<")+lua_typename(L, o.type())+">";
}

static void strreplaceall(xr_string &str, LPCSTR what, LPCSTR to)
{
    LPCSTR A;
    int S_len = xr_strlen(what);
    while (A = strstr(str.c_str(), what), A)
        str.replace(A-str.c_str(), S_len, to);
}

static xr_string &process_signature(xr_string &str)
{
    strreplaceall(str, "custom [", "");
    strreplaceall(str, "]", "");
    strreplaceall(str, "float", "number");
    strreplaceall(str, "lua_State*, ", "");
    strreplaceall(str, " ,lua_State*", "");
    return str;
}

static xr_string member_to_string(luabind::object const &e, LPCSTR function_signature)
{
#ifndef LUABIND_NO_ERROR_CHECKING
    using namespace luabind::detail;
    lua_State *L = e.lua_state();
    LUABIND_CHECK_STACK(L);
    if (e.type() == LUA_TFUNCTION)
    {
        e.pushvalue();
        stack_pop p(L, 1);
        {
            if (!lua_getupvalue(L, -1, 3))
                return to_string(e);
            stack_pop p2(L, 1);
            if (lua_touserdata(L, -1)!=reinterpret_cast<void*>(0x1337))
                return to_string(e);
        }
        // #ifdef BOOST_NO_STRINGSTREAM
        //      std::strstream s;
        // #else
        //      std::stringstream s;
        // #endif
        xr_string s = "";
        {
            lua_getupvalue(L, -1, 2);
            stack_pop p2(L, 1);
        }
        {
            lua_getupvalue(L, -1, 1);
            stack_pop p2(L, 1);
            method_rep *m = static_cast<method_rep*>(lua_touserdata(L, -1));
            for (auto i = m->overloads().begin(); i != m->overloads().end(); ++i)
            {
                luabind::internal_string str;
                i->get_signature(L, str);
                if (i != m->overloads().begin())
                    s += "\n";
                xr_string xr_str(str.c_str());
                s += function_signature + process_signature(xr_str) + ";";
            }
        }
#ifdef BOOST_NO_STRINGSTREAM
        s += "\n";// std::ends;
#endif
        return s;
    }
    return to_string(e);
#else
    return "";
#endif
}

static void print_class(lua_State *L, luabind::detail::class_rep *crep)
{
    using namespace luabind::detail;
    xr_string S;
    // print class and bases
    {
        S = crep->get_class_type()!=class_rep::cpp_class ? "LUA class " : "C++ class ";
        S.append(crep->name());
        typedef luabind::internal_vector<class_rep::base_info> BASES;
        const BASES &bases = crep->bases();
        BASES::const_iterator I = bases.begin(), B = I;
        BASES::const_iterator E = bases.end();
        if (B!=E)
            S.append(" : ");
        for (; I!=E; ++I)
        {
            if (I!=B)
                S.append(",");
            S.append((*I).base->name());
        }
        Msg("%s {", S.c_str());
    }
    // print class constants
    {
        const class_rep::STATIC_CONSTANTS &constants = crep->static_constants();
        for (auto &c : constants)
#ifndef USE_NATIVE_LUA_STRINGS
            Msg("    const %s = %d;", c.first, c.second);
#else
            Msg("    const %s = %d;", getstr(c.first.m_object), c.second);
#endif
        if (!constants.empty())
            Msg("    ");
    }
    // print class properties
    {
#ifndef USE_NATIVE_LUA_STRINGS
        typedef luabind::internal_map<const char*, class_rep::callback, ltstr> PROPERTIES;
#else
        typedef class_rep::callback_map PROPERTIES;
#endif
        const PROPERTIES &properties = crep->properties();
        for (auto &p : properties)
#ifndef USE_NATIVE_LUA_STRINGS
            Msg("    property %s;", p.first);
#else
            Msg("    property %s;", getstr(p.first.m_object));
#endif
        if (!properties.empty())
            Msg("    ");
    }
    // print class constructors
    {
        typedef luabind::internal_vector<construct_rep::overload_t> Constructors;
        const Constructors &constructors = crep->constructors().overloads;
        for (auto &ctor : constructors)
        {
            luabind::internal_string luaStr;
            ctor.get_signature(L, luaStr);
            xr_string str(luaStr.c_str());
            strreplaceall(str, "custom [", "");
            strreplaceall(str, "]", "");
            strreplaceall(str, "float", "number");
            strreplaceall(str, "lua_State*, ", "");
            strreplaceall(str, " ,lua_State*", "");
            Msg("    %s %s;", crep->name(), str.c_str());
        }
        if (!constructors.empty())
            Msg("    ");
    }
    // print class methods
    {
        crep->get_table(L);
        luabind::object table(L);
        table.set();
        for (auto &obj : table) // XXX: conversion from proxy_object to proxy_object &
        {
            xr_string str = "    function ";
            str.append(to_string(obj).c_str());
            strreplaceall(S, "function __add", "operator +");
            strreplaceall(S, "function __sub", "operator -");
            strreplaceall(S, "function __mul", "operator *");
            strreplaceall(S, "function __div", "operator /");
            strreplaceall(S, "function __pow", "operator ^");
            strreplaceall(S, "function __lt", "operator <");
            strreplaceall(S, "function __le", "operator <=");
            strreplaceall(S, "function __gt", "operator >");
            strreplaceall(S, "function __ge", "operator >=");
            strreplaceall(S, "function __eq", "operator ==");
            Msg("%s", member_to_string(obj, str.c_str()).c_str());
        }
    }
    Msg("};\n");
}

static void print_free_functions(lua_State *L, const luabind::object &object, LPCSTR header, const xr_string &indent)
{
    using namespace luabind::detail::free_functions;
    u32 count = 0;
    for (auto &proxy : object) // XXX: conversion from proxy_object to proxy_object &
    {
        if (proxy.type() != LUA_TFUNCTION)
            continue;
        proxy.pushvalue();
        if (lua_iscfunction(L, -1) && lua_getupvalue(L, -1, 2))
        {
            // check the magic number that identifies luabind's functions
            if (lua_touserdata(L, -1)==(void*)0x1337 && lua_getupvalue(L, -2, 1))
            {
                if (!count)
                    Msg("\n%snamespace %s {", indent.c_str(), header);
                count++;
                auto rep = static_cast<function_rep*>(lua_touserdata(L, -1));
                for (auto &overload : rep->overloads())
                {
                    luabind::internal_string luaStr;
                    overload.get_signature(L, luaStr);
                    xr_string str(luaStr.c_str());
                    Msg("    %sfunction %s%s;", indent.c_str(), rep->name(), process_signature(str).c_str());
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    {
        xr_string _indent = indent;
        _indent.append("    ");
        object.pushvalue();
        lua_pushnil(L);
        while (lua_next(L, -2))
        {
            if (lua_type(L, -1)==LUA_TTABLE)
            {
                LPCSTR S = lua_tostring(L, -2);
                if (xr_strcmp("_G", S) && xr_strcmp("package", S))
                {
                    luabind::object obj(L);
                    obj.set();
                    if (!xr_strcmp("security", S)) // XXX: why?
                        S = S;
                    print_free_functions(L, obj, S, _indent);
                }
            }
#pragma todo("Dima to Dima : Remove this hack if find out why")
            if (lua_isnumber(L, -2))
            {
                lua_pop(L, 1);
                lua_pop(L, 1);
                break;
            }
            lua_pop(L, 1);
        }
    }
    if (count)
        Msg("%s};", indent.c_str());
}

void CScriptEngine::PrintHelp()
{
    Msg("\nList of the classes exported to LUA\n");
    luabind::detail::class_registry::get_registry(lua())->iterate_classes(lua(), &print_class);
    Msg("End of list of the classes exported to LUA\n");
    Msg("\nList of the namespaces exported to LUA\n");
    print_free_functions(lua(), luabind::get_globals(lua()), "", "");
    Msg("End of list of the namespaces exported to LUA\n");
}

#else

void CScriptEngine::PrintHelp()
{ Msg("! Release build doesn't support lua-help :("); }

#endif

CScriptProcess *CScriptEngine::CreateScriptProcess(shared_str name, shared_str scripts)
{ return new CScriptProcess(this, name, scripts); }

CScriptThread *CScriptEngine::CreateScriptThread(LPCSTR caNamespaceName, bool do_string, bool reload)
{
    auto thread = new CScriptThread(this, caNamespaceName, do_string, reload);
    lua_State *threadLua = thread->lua();
    if (threadLua)
        RegisterState(threadLua, this);
    else
        xr_delete(thread);
    return thread;
}

void CScriptEngine::DestroyScriptThread(const CScriptThread *thread)
{
#ifdef DEBUG
    Msg("* Destroying script thread %s", *thread->script_name());
#endif
    try
    {
#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
        if (debugger())
            debugger()->remove(thread->lua());
#endif
#ifndef LUABIND_HAS_BUGS_WITH_LUA_THREADS
        luaL_unref(lua(), LUA_REGISTRYINDEX, thread->thread_reference());
#endif
    } catch (...) {
    }
    UnregisterState(thread->lua());
}
