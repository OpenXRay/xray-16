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
#include "script_profiler.hpp"
#include "script_thread.hpp"
#include "ScriptExporter.hpp"
#include "BindingsDumper.hpp"
#ifdef USE_DEBUGGER
#include "script_debugger.hpp"
#endif
#ifdef DEBUG
#include "script_thread.hpp"
#endif
#include <stdarg.h>
#include "Common/Noncopyable.hpp"
#include "xrCore/ModuleLookup.hpp"
#include "xrLuaFix/xrLuaFix.h"
#include "luabind/class_info.hpp"

#include <tracy/TracyLua.hpp>

Flags32 g_LuaDebug;
int g_LuaDumpDepth = 3;

#define SCRIPT_GLOBAL_NAMESPACE "_G"

static const char* file_header_old = "local function script_name() \
return \"%s\" \
end \
local this = {} \
%s this %s \
setmetatable(this, {__index = " SCRIPT_GLOBAL_NAMESPACE "}) \
setfenv(1, this) ";

static const char* file_header_new = "local function script_name() \
return \"%s\" \
end \
local this = {} \
this." SCRIPT_GLOBAL_NAMESPACE " = " SCRIPT_GLOBAL_NAMESPACE " \
%s this %s \
setfenv(1, this) ";

static const char* file_header = nullptr;

static void* lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    (void)ud;
    (void)osize;
    if (nsize == 0)
    {
        xr_free(ptr);
        return nullptr;
    }
    return xr_realloc(ptr, nsize);
}

static void* __cdecl luabind_allocator(void* context, const void* pointer, size_t const size)
{
    if (!size)
    {
        void* non_const_pointer = const_cast<LPVOID>(pointer);
        xr_free(non_const_pointer);
        return nullptr;
    }
    if (!pointer)
    {
        return xr_malloc(size);
    }
    void* non_const_pointer = const_cast<LPVOID>(pointer);
    return xr_realloc(non_const_pointer, size);
}

namespace
{
void LuaJITLogError(lua_State* ls, const char* msg)
{
    const char* info = nullptr;
    if (!lua_isnil(ls, -1))
    {
        info = lua_tostring(ls, -1);
        lua_pop(ls, 1);
    }
    Msg("! LuaJIT: %s (%s)", msg, info ? info : "no info");
}
// tries to execute 'jit'+command
bool RunJITCommand(lua_State* ls, const char* command)
{
    string128 buf;
    xr_strcpy(buf, "jit.");
    xr_strcat(buf, command);
    if (luaL_dostring(ls, buf))
    {
        LuaJITLogError(ls, "Unrecognized command");
        return false;
    }
    return true;
}
}

const char* const CScriptEngine::GlobalNamespace = SCRIPT_GLOBAL_NAMESPACE;
Lock CScriptEngine::stateMapLock;
xr_unordered_map<lua_State*, CScriptEngine*> CScriptEngine::stateMap;

string4096 CScriptEngine::g_ca_stdout;

void CScriptEngine::reinit()
{
    ZoneScoped;
    stateMapLock.Enter();
    stateMap.reserve(32); // 32 lua states should be enough
    stateMapLock.Leave();
    if (m_virtual_machine)
    {
        if (m_profiler)
            m_profiler->OnDispose(m_virtual_machine);

        lua_close(m_virtual_machine);
        UnregisterState(m_virtual_machine);
    }
    m_virtual_machine = lua_newstate(lua_alloc, nullptr);
    if (!m_virtual_machine)
    {
        Log("! ERROR : Cannot initialize script virtual machine!");
        return;
    }
    RegisterState(m_virtual_machine, this);
    if (strstr(Core.Params, "-_g"))
        file_header = file_header_new;
    else
        file_header = file_header_old;
    scriptBufferSize = 1024 * 1024;
    scriptBuffer = xr_alloc<char>(scriptBufferSize);

    if (m_profiler)
        m_profiler->OnReinit(m_virtual_machine);
}

void CScriptEngine::print_stack(lua_State* L)
{
    if (L == nullptr)
        L = lua();

    if (lua_isstring(L, -1))
    {
        pcstr err = lua_tostring(L, -1);
        script_log(LuaMessageType::Error, "%s", err);
    }

    lua_Debug l_tDebugInfo;
    for (int i = 0; lua_getstack(L, i, &l_tDebugInfo); i++)
    {
        lua_getinfo(L, "nSlu", &l_tDebugInfo);

        if (!xr_strcmp(l_tDebugInfo.what, "C"))
        {
            script_log(LuaMessageType::Error, "%2d : [C  ] %s", i, l_tDebugInfo.name ? l_tDebugInfo.name : "");
        }
        else
        {
            string_path temp;
            if (l_tDebugInfo.name)
                xr_sprintf(temp, "%s(%d)", l_tDebugInfo.name, l_tDebugInfo.linedefined);
            else
                xr_sprintf(temp, "function <%s:%d>", l_tDebugInfo.short_src, l_tDebugInfo.linedefined);

            script_log(LuaMessageType::Error, "%2d : [%3s] %s(%d) : %s", i, l_tDebugInfo.what,
                l_tDebugInfo.short_src, l_tDebugInfo.currentline, temp);
        }

        // Giperion: verbose log
        if (g_LuaDumpDepth > 0)
        {
            script_log(LuaMessageType::Error, "\t Locals: ");
            int VarID = 1;
            pcstr name;

            while ((name = lua_getlocal(L, &l_tDebugInfo, VarID++)) != nullptr)
            {
                luabind::detail::stack_pop pop{ L, 1 };
                log_value(L, name, 1);
            }
        }
        // -Giperion
    }
}

void CScriptEngine::log_value(lua_State* L, pcstr name, int depth)
{
    using namespace luabind::detail;

    string32 tab_buffer{};
    FillMemory(tab_buffer, std::min(int(sizeof(tab_buffer) - 1), depth), '\t');

    string256 value{};
    char colon{ ':' };
    bool log_table{};

    const int ntype = lua_type(L, -1);
    pcstr type = lua_typename(L, ntype);

    switch (ntype)
    {
    case LUA_TNIL:
    case LUA_TFUNCTION:
    case LUA_TTHREAD:
        colon = '\0';
        break;

    case LUA_TNUMBER:
        xr_sprintf(value, "%f", lua_tonumber(L, -1));
        break;

    case LUA_TBOOLEAN:
        xr_sprintf(value, "%s", lua_toboolean(L, -1) ? "true" : "false");
        break;

    case LUA_TSTRING:
        xr_sprintf(value, "%.255s", lua_tostring(L, -1));
        break;

    case LUA_TTABLE:
        if (depth <= g_LuaDumpDepth)
            log_table = true;
        else
            xr_sprintf(value, "[...]");
        break;

    case LUA_TUSERDATA:
        if (const auto* object = get_instance(L, -1))
        {
            if (const auto* rep = object->crep())
            {
                type = rep->name();
                if (depth <= g_LuaDumpDepth)
                {
                    lua_getfenv(L, -1);
                    log_table = true;
                }
                else
                    xr_sprintf(value, "[...]");
                break;
            }
        }
        [[fallthrough]];

    default:
        xr_strcpy(value, "[not available]");
        break;
    }

    script_log(LuaMessageType::Error, "%s %s %s %c %s", tab_buffer, type, name, colon, value);
    if (log_table)
    {
        luabind::table members(luabind::from_stack(L, -1));
        for (luabind::iterator it(members), end; it != end; ++it)
        {
            auto proxy = *it;
            proxy.push(L);
            stack_pop pop{ L, 1 };
            if (lua_iscfunction(L, -1))
                continue;
            log_value(L, lua_tostring(L, -2), depth + 1);
        }

        if (ntype == LUA_TUSERDATA)
            lua_pop(L, 1);
    }
}

bool CScriptEngine::parse_namespace(pcstr caNamespaceName, pstr b, size_t b_size, pstr c, size_t c_size)
{
    *b = 0;
    *c = 0;
    pstr S2;
    STRCONCAT(S2, caNamespaceName);
    pstr S = S2;
    for (int i = 0;; i++)
    {
        if (!xr_strlen(S))
        {
            script_log(LuaMessageType::Error, "the namespace name %s is incorrect!", caNamespaceName);
            return false;
        }
        pstr S1 = strchr(S, '.');
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

bool CScriptEngine::load_buffer(
lua_State* L, LPCSTR caBuffer, size_t tSize, LPCSTR caScriptName, LPCSTR caNameSpaceName)
{
    int l_iErrorCode;
    if (caNameSpaceName && xr_strcmp(GlobalNamespace, caNameSpaceName))
    {
        string512 insert, a, b;
        LPCSTR header = file_header;
        if (!parse_namespace(caNameSpaceName, a, sizeof(a), b, sizeof(b)))
            return false;
        xr_sprintf(insert, header, caNameSpaceName, a, b);
        const size_t str_len = xr_strlen(insert);
        const size_t total_size = str_len + tSize;
        if (total_size >= scriptBufferSize)
        {
            scriptBufferSize = total_size;
            scriptBuffer = (char*)xr_realloc(scriptBuffer, scriptBufferSize);
        }
        xr_strcpy(scriptBuffer, total_size, insert);
        CopyMemory(scriptBuffer + str_len, caBuffer, tSize);
        l_iErrorCode = luaL_loadbuffer(L, scriptBuffer, tSize + str_len, caScriptName);
    }
    else
        l_iErrorCode = luaL_loadbuffer(L, caBuffer, tSize, caScriptName);
    if (l_iErrorCode)
    {
        print_output(L, caScriptName, l_iErrorCode);
        on_error(L);
        return false;
    }
    return true;
}

bool CScriptEngine::do_file(LPCSTR caScriptName, LPCSTR caNameSpaceName)
{
    int start = lua_gettop(lua());
    string_path l_caLuaFileName;
    IReader* l_tpFileReader = FS.r_open(caScriptName);
    if (!l_tpFileReader)
    {
        script_log(LuaMessageType::Error, "Cannot open file \"%s\"", caScriptName);
        return false;
    }
    strconcat(sizeof(l_caLuaFileName), l_caLuaFileName, "@", caScriptName);
    if (!load_buffer(lua(), static_cast<LPCSTR>(l_tpFileReader->pointer()), l_tpFileReader->length(),
        l_caLuaFileName, caNameSpaceName))
    {
        // VERIFY(lua_gettop(lua())>=4);
        // lua_pop(lua(), 4);
        // VERIFY(lua_gettop(lua())==start-3);
        lua_settop(lua(), start);
        FS.r_close(l_tpFileReader);
        return false;
    }
    FS.r_close(l_tpFileReader);
    int errFuncId = -1;
#ifdef USE_DEBUGGER
    if (debugger())
        errFuncId = debugger()->PrepareLua(lua());
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
    if (debugger())
        debugger()->UnPrepareLua(lua(), errFuncId);
#endif
    if (l_iErrorCode)
    {
        print_output(lua(), caScriptName, l_iErrorCode);
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
    VERIFY(lua_gettop(lua()) == start);
    return true;
}

bool CScriptEngine::namespace_loaded(LPCSTR name, bool remove_from_stack)
{
    [[maybe_unused]] int start = lua_gettop(lua());

    lua_pushstring(lua(), GlobalNamespace);
    lua_rawget(lua(), LUA_GLOBALSINDEX);
    string256 S2 = { 0 };
    xr_strcpy(S2, name);
    pstr S = S2;
    for (;;)
    {
        if (!xr_strlen(S))
        {
            VERIFY(lua_gettop(lua()) >= 1);
            lua_pop(lua(), 1);
            VERIFY(start == lua_gettop(lua()));
            return false;
        }
        pstr S1 = strchr(S, '.');
        if (S1)
            *S1 = 0;
        lua_pushstring(lua(), S);
        lua_rawget(lua(), -2);
        if (lua_isnil(lua(), -1))
        {
            // lua_settop(lua(), 0);
            VERIFY(lua_gettop(lua()) >= 2);
            lua_pop(lua(), 2);
            VERIFY(start == lua_gettop(lua()));
            return false; // there is no namespace!
        }
        else if (!lua_istable(lua(), -1))
        {
            // lua_settop(lua(), 0);
            VERIFY(lua_gettop(lua()) >= 1);
            lua_pop(lua(), 1);
            VERIFY(start == lua_gettop(lua()));
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
        VERIFY(lua_gettop(lua()) == start + 1);
    else
    {
        VERIFY(lua_gettop(lua()) >= 1);
        lua_pop(lua(), 1);
        VERIFY(lua_gettop(lua()) == start);
    }
    return true;
}

bool CScriptEngine::object(LPCSTR identifier, int type)
{
    [[maybe_unused]] int start = lua_gettop(lua());

    lua_pushnil(lua());
    while (lua_next(lua(), -2))
    {
        if (lua_type(lua(), -1) == type && !xr_strcmp(identifier, lua_tostring(lua(), -2)))
        {
            VERIFY(lua_gettop(lua()) >= 3);
            lua_pop(lua(), 3);
            VERIFY(lua_gettop(lua()) == start - 1);
            return true;
        }
        lua_pop(lua(), 1);
    }
    VERIFY(lua_gettop(lua()) >= 1);
    lua_pop(lua(), 1);
    VERIFY(lua_gettop(lua()) == start - 1);
    return false;
}

bool CScriptEngine::object(LPCSTR namespace_name, LPCSTR identifier, int type)
{
    [[maybe_unused]] int start = lua_gettop(lua());

    if (xr_strlen(namespace_name) && !namespace_loaded(namespace_name, false))
    {
        VERIFY(lua_gettop(lua()) == start);
        return false;
    }
    bool result = object(identifier, type);
    VERIFY(lua_gettop(lua()) == start);
    return result;
}

luabind::object CScriptEngine::name_space(LPCSTR namespace_name)
{
    string256 S1 = { 0 };
    xr_strcpy(S1, namespace_name);
    pstr S = S1;
    luabind::object lua_namespace = luabind::globals(lua());
    for (;;)
    {
        if (!xr_strlen(S))
            return lua_namespace;
        pstr I = strchr(S, '.');
        if (!I)
            return lua_namespace[(const char*)S];
        *I = 0;
        lua_namespace = lua_namespace[(const char*)S];
        S = I + 1;
    }
}

bool CScriptEngine::print_output(lua_State* L, pcstr caScriptFileName, int errorCode, pcstr caErrorText)
{
    CScriptEngine* scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);

    if (caErrorText)
    {
        const auto [logHeader, luaLogHeader] = get_message_headers(LuaMessageType::Error);
        Msg("%sSCRIPT ERROR: %s\n", logHeader, caErrorText);
    }

    if (errorCode)
        print_error(L, errorCode);

    if (!lua_isstring(L, -1))
        return false;

    const auto S = lua_tostring(L, -1);

    if (!xr_strcmp(S, "cannot resume dead coroutine"))
    {
        VERIFY2("Please do not return any values from main!!!", caScriptFileName);
#if defined(USE_DEBUGGER)
        if (scriptEngine->debugger() && scriptEngine->debugger()->Active())
        {
            scriptEngine->debugger()->Write(S);
            scriptEngine->debugger()->ErrorBreak();
        }
#endif
    }
    else
    {
        if (!errorCode)
            scriptEngine->script_log(LuaMessageType::Info, "Output from %s", caScriptFileName);
#if defined(USE_DEBUGGER)
        if (scriptEngine->debugger() && scriptEngine->debugger()->Active())
        {
            scriptEngine->debugger()->Write(S);
            scriptEngine->debugger()->ErrorBreak();
        }
#endif
    }

    return true;
}

void CScriptEngine::print_error(lua_State* L, int iErrorCode)
{
    CScriptEngine* scriptEngine = GetInstance(L);
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
    default: NODEFAULT;
    }
}

void CScriptEngine::flush_log()
{
    string_path log_file_name;
    strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, "_lua.log");
    FS.update_path(log_file_name, "$logs$", log_file_name);
    m_output.save_to(log_file_name);
}

CScriptEngine::CScriptEngine(bool is_editor, bool is_with_profiler)
{
    luabind::allocator = &luabind_allocator;
    luabind::allocator_context = nullptr;
    m_current_thread = nullptr;
    m_virtual_machine = nullptr;
    m_profiler = is_with_profiler && !is_editor ? xr_new<CScriptProfiler>(this) : nullptr;
    m_stack_level = 0;
    m_reload_modules = false;
    m_last_no_file_length = 0;
    *m_last_no_file = 0;
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    static_assert(false, "Do not define USE_LUA_STUDIO macro without USE_DEBUGGER macro");
    m_scriptDebugger = nullptr;
    restartDebugger();
#endif
#endif
    m_is_editor = is_editor;
}

CScriptEngine::~CScriptEngine()
{
    if (m_profiler)
    {
        if (m_virtual_machine)
            m_profiler->OnDispose(m_virtual_machine);

        xr_delete(m_profiler);
    }

    if (m_virtual_machine)
        lua_close(m_virtual_machine);
    while (!m_script_processes.empty())
        remove_script_process(m_script_processes.begin()->first);
#ifdef DEBUG
    flush_log();
#endif
#ifdef USE_DEBUGGER
    xr_delete(m_scriptDebugger);
#endif
    if (scriptBuffer)
        xr_free(scriptBuffer);
}

void CScriptEngine::unload()
{
    lua_settop(lua(), m_stack_level);
    m_last_no_file_length = 0;
    *m_last_no_file = 0;
}

int CScriptEngine::lua_panic(lua_State* L)
{
    print_output(L, "", LUA_ERRRUN, "PANIC");
    FATAL("Lua panic");
    return 0;
}

void CScriptEngine::lua_error(lua_State* L)
{
    print_output(L, "", LUA_ERRRUN);
    on_error(L);

#if !XRAY_EXCEPTIONS
    xrDebug::Fatal(DEBUG_INFO, "LUA error: %s", lua_tostring(L, -1));
#else
    throw lua_tostring(L, -1);
#endif
}

int CScriptEngine::lua_pcall_failed(lua_State* L)
{
    print_output(L, "", LUA_ERRRUN);
    on_error(L);

    luabind::detail::stack_pop pop{ L, lua_isstring(L, -1) ? 1 : 0 };

    if (xrDebug::WouldShowErrorMessage())
    {
        const auto err = lua_tostring(L, -1);

        static bool ignoreAlways;
        const auto result = xrDebug::Fail(ignoreAlways, DEBUG_INFO, "LUA error", err);

        if (result == AssertionResult::tryAgain || result == AssertionResult::ignore)
            return LUA_OK;
    }

    return LUA_ERRRUN;
}

#if !XRAY_EXCEPTIONS
void CScriptEngine::lua_cast_failed(lua_State* L, const luabind::type_id& info)
{
    string128 buf;
    xr_sprintf(buf, "cannot cast lua value to %s", info.name());
    print_output(L, "", LUA_ERRRUN, buf);
    xrDebug::Fatal(DEBUG_INFO, "LUA error: cannot cast lua value to %s", info.name());
}
#endif

void CScriptEngine::setup_callbacks()
{
#ifdef USE_DEBUGGER
    if (debugger())
        debugger()->PrepareLuaBind();

    if (!debugger() || !debugger()->Active())
#endif
    {
#if !XRAY_EXCEPTIONS
        luabind::set_error_callback(CScriptEngine::lua_error);
#endif

        luabind::set_pcall_callback([](lua_State* L) { lua_pushcfunction(L, CScriptEngine::lua_pcall_failed); });
    }
#if !XRAY_EXCEPTIONS
    luabind::set_cast_failed_callback(CScriptEngine::lua_cast_failed);
#endif
    lua_atpanic(lua(), CScriptEngine::lua_panic);
}

void CScriptEngine::lua_hook_call(lua_State* L, lua_Debug* dbg)
{
    CScriptEngine* scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);

#ifdef DEBUG
    if (scriptEngine->current_thread())
        scriptEngine->current_thread()->script_hook(L, dbg);
#endif

    if (scriptEngine->m_profiler)
        scriptEngine->m_profiler->OnLuaHookCall(L, dbg);
}

int CScriptEngine::auto_load(lua_State* L)
{
    if (lua_gettop(L) < 2 || !lua_istable(L, 1) || !lua_isstring(L, 2))
    {
        lua_pushnil(L);
        return 1;
    }
    CScriptEngine* scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);
    scriptEngine->process_file_if_exists(lua_tostring(L, 2), false);
    lua_rawget(L, 1);
    return 1;
}

void CScriptEngine::setup_auto_load()
{
    luaL_newmetatable(lua(), "XRAY_AutoLoadMetaTable");
    lua_pushstring(lua(), "__index");
    lua_pushcfunction(lua(), CScriptEngine::auto_load);
    lua_settable(lua(), -3);
    lua_pushstring(lua(), GlobalNamespace);
    lua_gettable(lua(), LUA_GLOBALSINDEX);
    luaL_getmetatable(lua(), "XRAY_AutoLoadMetaTable");
    lua_setmetatable(lua(), -2);
    //. ??????????
    // lua_settop(lua(), 0);
}

// initialize lua standard library functions
struct luajit
{
    static void open_lib(lua_State* L, pcstr module_name, lua_CFunction function)
    {
        lua_pushcfunction(L, function);
        lua_pushstring(L, module_name);
        lua_call(L, 1, 0);
    }

    static void allow_escape_sequences(bool allowed)
    {
        lj_allow_escape_sequences(allowed ? 1 : 0);
    }
};

void CScriptEngine::init(ExporterFunc exporterFunc, bool loadGlobalNamespace)
{
    ZoneScoped;

    reinit();
    luabind::open(lua());

    // Workarounds to preserve backwards compatibility with game scripts
    {
        const bool nilConversion =
            pSettingsOpenXRay->read_if_exists<bool>("lua_scripting", "allow_nil_conversion", true);

        luabind::allow_nil_conversion(nilConversion);
        luabind::disable_super_deprecation();

        const bool escapeSequences =
            pSettingsOpenXRay->read_if_exists<bool>("lua_scripting", "allow_escape_sequences", false);
        luajit::allow_escape_sequences(escapeSequences);
    }

    luabind::bind_class_info(lua());
    setup_callbacks();
    if (exporterFunc)
        exporterFunc(lua());
    if (std::strstr(Core.Params, "-dump_bindings") && !bindingsDumped)
    {
        bindingsDumped = true;
        static int dumpId = 1;
        string_path filePath;
        xr_sprintf(filePath, "ScriptBindings_%d.txt", dumpId++);
        FS.update_path(filePath, "$app_data_root$", filePath);
        IWriter* writer = FS.w_open(filePath);
        BindingsDumper dumper;
        BindingsDumper::Options options = {};
        options.ShiftWidth = 4;
        options.IgnoreDerived = true;
        options.StripThis = true;
        dumper.Dump(lua(), writer, options);
        FS.w_close(writer);
    }

    luajit::open_lib(lua(), "", luaopen_base);
    luajit::open_lib(lua(), LUA_LOADLIBNAME, luaopen_package);
    luajit::open_lib(lua(), LUA_TABLIBNAME, luaopen_table);
    luajit::open_lib(lua(), LUA_IOLIBNAME, luaopen_io);
    luajit::open_lib(lua(), LUA_OSLIBNAME, luaopen_os);
    luajit::open_lib(lua(), LUA_MATHLIBNAME, luaopen_math);
    luajit::open_lib(lua(), LUA_STRLIBNAME, luaopen_string);
    luajit::open_lib(lua(), LUA_BITLIBNAME, luaopen_bit);
    luajit::open_lib(lua(), LUA_FFILIBNAME, luaopen_ffi);
#ifndef MASTER_GOLD
    luajit::open_lib(lua(), LUA_DBLIBNAME, luaopen_debug);
#endif

    luaopen_xrluafix(lua());

    tracy::LuaRegister(lua());

    // Game scripts doesn't call randomize but use random
    // So, we should randomize in the engine.
    {
        pcstr randomSeed = "math.randomseed(os.time())";
        pcstr mathRandom = "math.random()";

        luaL_dostring(lua(), randomSeed);
        // It's a good practice to call random few times before using it
        for (int i = 0; i < 3; ++i)
            luaL_dostring(lua(), mathRandom);
    }

    // Adds gamedata folder as module root for lua `require` and allows usage of built-in lua module system.
    // Notes:
    // - Does not resolve files inside archived game files
    // Example:
    // `local example = require("scripts.folder.file")` tries to import `gamedata\scripts\folder\file.script`
    {
        string_path gamedataPath;
        string_path packagePath;

        FS.update_path(gamedataPath, "$game_data$", "?.script;");
        xr_sprintf(packagePath, "package.path = package.path .. [[%s]]", gamedataPath);

        luaL_dostring(lua(), packagePath);
     }

    // XXX nitrocaster: with vanilla scripts, '-nojit' option requires script profiler to be disabled. The reason
    // is that lua hooks somehow make 'super' global unavailable (is's used all over the vanilla scripts).
    // You can disable script profiler by commenting out the following lines in the beginning of _g.script:
    // if (jit == nil) then
    //     profiler.setup_hook()
    // end
    //
    // Update: '-nojit' option adds garbage to stack and luabind calls fail
    if (!strstr(Core.Params, ARGUMENT_ENGINE_NOJIT))
    {
        luajit::open_lib(lua(), LUA_JITLIBNAME, luaopen_jit);
        // Xottab_DUTY: commented this. Let's use default opt level, which is 3
        //RunJITCommand(lua(), "opt.start(2)");
    }
    setup_auto_load();

#if defined(DEBUG) && !defined(USE_LUA_STUDIO)
#if defined(USE_DEBUGGER)
    if (!debugger() || !debugger()->Active())
#endif
        lua_sethook(lua(), CScriptEngine::lua_hook_call, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 0);
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

void CScriptEngine::remove_script_process(const ScriptProcessor& process_id)
{
    CScriptProcessStorage::iterator I = m_script_processes.find(process_id);
    if (I != m_script_processes.end())
    {
        xr_delete((*I).second);
        m_script_processes.erase(I);
    }
}

bool CScriptEngine::load_file(const char* scriptName, const char* namespaceName)
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
    const size_t string_length = xr_strlen(file_name);
    if (!warn_if_not_exist && no_file_exists(file_name, string_length))
        return false;
    string_path S, S1;
    if (m_reload_modules || (*file_name && !namespace_loaded(file_name)))
    {
        FS.update_path(S, "$game_scripts$", strconcat(sizeof(S1), S1, file_name, ".script"));
        if (!warn_if_not_exist && !FS.exist(S))
        {
#ifdef DEBUG
            if (false) // XXX: restore (check script engine flags)
            {
                print_stack();
                Msg("! WARNING: Access to nonexistent variable '%s' or loading nonexistent script '%s'", file_name, S1);
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

bool CScriptEngine::process_file(LPCSTR file_name) { return process_file_if_exists(file_name, true); }
bool CScriptEngine::process_file(LPCSTR file_name, bool reload_modules)
{
    m_reload_modules = reload_modules;
    bool result = process_file_if_exists(file_name, true);
    m_reload_modules = false;
    return result;
}

bool CScriptEngine::function_object(LPCSTR function_to_call, luabind::object& object, int type)
{
    if (!xr_strlen(function_to_call))
        return false;
    string256 name_space = { 0 }, function = { 0 };
    parse_script_namespace(function_to_call, name_space, sizeof(name_space), function, sizeof(function));
    if (xr_strcmp(name_space, GlobalNamespace))
    {
        pstr file_name = strchr(name_space, '.');
        if (!file_name)
            process_file_if_exists(name_space, false);
        else
        {
            *file_name = 0;
            process_file_if_exists(name_space, false);
            *file_name = '.';
        }
    }
    if (!this->object(name_space, function, type))
        return false;
    luabind::object lua_namespace = this->name_space(name_space);
    object = lua_namespace[function];
    return true;
}

void CScriptEngine::add_script_process(const ScriptProcessor& process_id, CScriptProcess* script_process)
{
    VERIFY(m_script_processes.find(process_id) == m_script_processes.end());
    m_script_processes.emplace(process_id, script_process);
}

CScriptProcess* CScriptEngine::script_process(const ScriptProcessor& process_id) const
{
    auto it = m_script_processes.find(process_id);
    if (it != m_script_processes.end())
        return it->second;
    return nullptr;
}

void CScriptEngine::parse_script_namespace(pcstr name, pstr ns, size_t nsSize, pstr func, size_t funcSize)
{
    const char* p = strrchr(name, '.');
    if (!p)
    {
        xr_strcpy(ns, nsSize, GlobalNamespace);
        p = name - 1;
    }
    else
    {
        VERIFY(size_t(p - name + 1) <= nsSize);
        strncpy(ns, name, p - name);
        ns[p - name] = 0;
    }
    xr_strcpy(func, funcSize, p + 1);
}

#if defined(USE_DEBUGGER)
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
    if (debugger())
        stopDebugger();
    m_scriptDebugger = xr_new<CScriptDebugger>(this);
    debugger()->PrepareLuaBind();
    Msg("Script debugger restarted.");
}
#endif

CScriptEngine* CScriptEngine::GetInstance(lua_State* state)
{
    CScriptEngine* instance = nullptr;
    stateMapLock.Enter();
    auto it = stateMap.find(state);
    if (it != stateMap.end())
        instance = it->second;
    stateMapLock.Leave();
    return instance;
}

bool CScriptEngine::RegisterState(lua_State* state, CScriptEngine* scriptEngine)
{
    bool result = false;
    stateMapLock.Enter();
    auto it = stateMap.find(state);
    if (it == stateMap.end())
    {
        stateMap.insert({state, scriptEngine});
        result = true;
    }
    stateMapLock.Leave();
    return result;
}

bool CScriptEngine::UnregisterState(lua_State* state)
{
    if (!state)
        return true;
    bool result = false;
    stateMapLock.Enter();
    auto it = stateMap.find(state);
    if (it != stateMap.end())
    {
        stateMap.erase(it);
        result = true;
    }
    stateMapLock.Leave();
    return result;
}

bool CScriptEngine::no_file_exists(pcstr file_name, size_t string_length)
{
    if (m_last_no_file_length != string_length)
        return false;
    return !memcmp(m_last_no_file, file_name, string_length);
}

void CScriptEngine::add_no_file(pcstr file_name, size_t string_length)
{
    m_last_no_file_length = string_length;
    CopyMemory(m_last_no_file, file_name, string_length + 1);
}

void CScriptEngine::collect_all_garbage()
{
    lua_gc(lua(), LUA_GCCOLLECT, 0);
    lua_gc(lua(), LUA_GCCOLLECT, 0);
}

void CScriptEngine::on_error(lua_State* state)
{
    [[maybe_unused]] CScriptEngine* scriptEngine = GetInstance(state);
    VERIFY(scriptEngine);
}

CScriptProcess* CScriptEngine::CreateScriptProcess(shared_str name, shared_str scripts)
{
    return xr_new<CScriptProcess>(this, name, scripts);
}

CScriptThread* CScriptEngine::CreateScriptThread(LPCSTR caNamespaceName, bool do_string, bool reload)
{
    auto thread = xr_new<CScriptThread>(this, caNamespaceName, do_string, reload);
    lua_State* threadLua = thread->lua();
    if (threadLua)
        RegisterState(threadLua, this);
    else
        xr_delete(thread);
    return thread;
}

void CScriptEngine::DestroyScriptThread(const CScriptThread* thread)
{
#ifdef DEBUG
    Msg("* Destroying script thread %s", *thread->script_name());
#endif
    try
    {
#ifndef LUABIND_HAS_BUGS_WITH_LUA_THREADS
        luaL_unref(lua(), LUA_REGISTRYINDEX, thread->thread_reference());
#endif
    }
    catch (...)
    {
    }
    UnregisterState(thread->lua());
}

bool CScriptEngine::is_editor()
{
    return m_is_editor;
}
