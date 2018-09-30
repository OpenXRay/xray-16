////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine.h
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Engine
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/xrCore.h"
#include "xrScriptEngine/xrScriptEngine.hpp"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrScriptEngine/script_space_forward.hpp"
#include "xrScriptEngine/Functor.hpp"
#include "xrCore/Threading/Lock.hpp"
#include "xrCommon/xr_unordered_map.h"

struct lua_State;

#ifndef MASTER_GOLD
#define USE_DEBUGGER
#define USE_LUA_STUDIO
#endif

#include "xrCore/Containers/AssociativeVector.hpp"

//#define DBG_DISABLE_SCRIPTS

class CScriptProcess;
class CScriptThread;
struct lua_State;
struct lua_Debug;

#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
class CScriptDebugger;
#else
namespace cs
{
namespace lua_studio
{
struct world;
}
}
class lua_studio_engine;
#endif
#endif

enum class ScriptProcessor : u32
{
    Level = 0,
    Game = 1,
    Dummy = u32(-1),
};

enum class LuaMessageType : u32
{
    Info = 0,
    Error = 1,
    Message = 2,
    HookCall = 3,
    HookReturn = 4,
    HookLine = 5,
    HookCount = 6,
    HookTailReturn = u32(-1),
};

extern Flags32 XRSCRIPTENGINE_API g_LuaDebug;

class XRSCRIPTENGINE_API CScriptEngine
{
public:
    typedef AssociativeVector<ScriptProcessor, CScriptProcess*> CScriptProcessStorage;
    static const char* const GlobalNamespace;

private:
    static Lock stateMapLock;
    static xr_unordered_map<lua_State*, CScriptEngine*> stateMap;
    lua_State* m_virtual_machine;
    CScriptThread* m_current_thread;
    bool m_reload_modules;
    string128 m_last_no_file;
    u32 m_last_no_file_length;
    static string4096 g_ca_stdout;
    bool logReenterability = false;
    bool bindingsDumped = false;
    char* scriptBuffer = nullptr;
    size_t scriptBufferSize = 0;
    bool m_is_editor;

protected:
    CScriptProcessStorage m_script_processes;
    int m_stack_level;

    CMemoryWriter m_output; // for call stack

#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    CScriptDebugger* m_scriptDebugger;
#else
    cs::lua_studio::world* m_lua_studio_world;
    lua_studio_engine* m_lua_studio_engine;
#endif
#endif

public:
    bool m_stack_is_ready;

private:
    static CScriptEngine* GetInstance(lua_State* state);
    static bool RegisterState(lua_State* state, CScriptEngine* scriptEngine);
    static bool UnregisterState(lua_State* state);
    bool no_file_exists(LPCSTR file_name, u32 string_length);
    void add_no_file(LPCSTR file_name, u32 string_length);

protected:
    int vscript_log(LuaMessageType luaMessageType, LPCSTR caFormat, va_list marker);
    bool parse_namespace(LPCSTR caNamespaceName, LPSTR b, u32 b_size, LPSTR c, u32 c_size);
    bool do_file(LPCSTR caScriptName, LPCSTR caNameSpaceName);
    void reinit();

public:
    lua_State* lua() { return m_virtual_machine; }
    void current_thread(CScriptThread* thread)
    {
        VERIFY(thread && !m_current_thread || !thread);
        m_current_thread = thread;
    }
    CScriptThread* current_thread() const { return m_current_thread; }
    bool load_buffer(
        lua_State* L, LPCSTR caBuffer, size_t tSize, LPCSTR caScriptName, LPCSTR caNameSpaceName = nullptr);
    bool load_file_into_namespace(LPCSTR caScriptName, LPCSTR caNamespaceName);
    bool namespace_loaded(LPCSTR caName, bool remove_from_stack = true);
    // check if object exists
    bool object(LPCSTR caIdentifier, int type);
    bool object(LPCSTR caNamespaceName, LPCSTR caIdentifier, int type);
    luabind::object name_space(LPCSTR namespace_name);
    int error_log(LPCSTR caFormat, ...);
    int script_log(LuaMessageType message, LPCSTR caFormat, ...);
    static bool print_output(lua_State* L, pcstr caScriptName, int iErrorCode = 0, pcstr caErrorText = nullptr);

private:
    static void print_error(lua_State* L, int iErrorCode);
    static void onErrorCallback(lua_State* L, pcstr scriptName, int errorCode, pcstr err = nullptr);

public:
    static void on_error(lua_State* state);

    void flush_log();
    void print_stack(lua_State* L = nullptr);

    void LogTable(lua_State* l, pcstr S, int level);
    void LogVariable(lua_State* l, pcstr name, int level);

    using ExporterFunc = XRay::ScriptExporter::Node::ExporterFunc;
    CScriptEngine(bool is_editor = false);
    virtual ~CScriptEngine();
    void init(ExporterFunc exporterFunc, bool loadGlobalNamespace);
    virtual void unload();
    static int lua_panic(lua_State* L);
    static void lua_error(lua_State* L);
    static int lua_pcall_failed(lua_State* L);
#if 1 //!XRAY_EXCEPTIONS
    static void lua_cast_failed(lua_State* L, const luabind::type_id& info);
#endif
#ifdef DEBUG
    static void lua_hook_call(lua_State* L, lua_Debug* dbg);
#endif
    void setup_callbacks();
    bool load_file(const char* scriptName, const char* namespaceName);
    CScriptProcess* script_process(const ScriptProcessor& process_id) const;
    void add_script_process(const ScriptProcessor& process_id, CScriptProcess* script_process);
    void remove_script_process(const ScriptProcessor& process_id);
    static int auto_load(lua_State* L);
    void setup_auto_load();
    bool process_file_if_exists(LPCSTR file_name, bool warn_if_not_exist);
    bool process_file(LPCSTR file_name);
    bool process_file(LPCSTR file_name, bool reload_modules);
    bool function_object(LPCSTR function_to_call, luabind::object& object, int type = LUA_TFUNCTION);
    void parse_script_namespace(const char* name, char* ns, u32 nsSize, char* func, u32 funcSize);
    template <typename TResult>
    IC bool functor(LPCSTR function_to_call, luabind::functor<TResult>& lua_function);
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    void stopDebugger();
    void restartDebugger();
    CScriptDebugger* debugger() { return m_scriptDebugger; }
#else
    void try_connect_to_debugger();
    void disconnect_from_debugger();
    cs::lua_studio::world* debugger() const { return m_lua_studio_world; }
    void initialize_lua_studio(lua_State* state, cs::lua_studio::world*& world, lua_studio_engine*& engine);
    void finalize_lua_studio(lua_State* state, cs::lua_studio::world*& world, lua_studio_engine*& engine);
#endif
#endif
    void collect_all_garbage();

    CScriptProcess* CreateScriptProcess(shared_str name, shared_str scripts);
    CScriptThread* CreateScriptThread(LPCSTR caNamespaceName, bool do_string = false, bool reload = false);
    // This function is called from CScriptThread destructor
    void DestroyScriptThread(const CScriptThread* thread);
    bool is_editor();
};

template <typename TResult>
IC bool CScriptEngine::functor(LPCSTR function_to_call, luabind::functor<TResult>& lua_function)
{
    luabind::object object;
    if (!function_object(function_to_call, object))
        return false;
    lua_function = object;
    return true;
}
