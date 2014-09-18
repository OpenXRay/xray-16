////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine.h
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Engine
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_storage.h"
#include "script_export_space.h"
#include "script_space_forward.h"
#include "associative_vector.h"

extern "C" {
	#include <lua/lua.h>
};

//#define DBG_DISABLE_SCRIPTS

#include "script_engine_space.h"

class CScriptProcess;
class CScriptThread;
struct lua_State;
struct lua_Debug;

#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
		class CScriptDebugger;
#	else // #ifndef USE_LUA_STUDIO
		namespace cs {
			namespace lua_studio {
				struct world;
			} // namespace lua_studio
		} // namespace cs

		class lua_studio_engine;
#	endif // #ifndef USE_LUA_STUDIO
#endif

class CScriptEngine : public CScriptStorage {
public:
	typedef CScriptStorage											inherited;
	typedef ScriptEngine::EScriptProcessors							EScriptProcessors;
	typedef associative_vector<EScriptProcessors,CScriptProcess*>	CScriptProcessStorage;

private:
	bool						m_reload_modules;

protected:
	CScriptProcessStorage		m_script_processes;
	int							m_stack_level;
	shared_str					m_class_registrators;

protected:
#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
		CScriptDebugger			*m_scriptDebugger;
#	else // #ifndef USE_LUA_STUDIO
		cs::lua_studio::world*	m_lua_studio_world;
		lua_studio_engine*		m_lua_studio_engine;
#	endif // #ifndef USE_LUA_STUDIO
#endif // #ifdef USE_DEBUGGER

private:
	string128					m_last_no_file;
	u32							m_last_no_file_length;

			bool				no_file_exists				(LPCSTR file_name, u32 string_length);
			void				add_no_file					(LPCSTR file_name, u32 string_length);

public:
								CScriptEngine				();
	virtual						~CScriptEngine				();
			void				init						();
	virtual	void				unload						();
	static	int					lua_panic					(lua_State *L);
	static	void				lua_error					(lua_State *L);
	static	int					lua_pcall_failed			(lua_State *L);
#ifdef DEBUG
	static	void				lua_hook_call				(lua_State *L, lua_Debug *dbg);
#endif // #ifdef DEBUG
			void				setup_callbacks				();
			void				load_common_scripts			();
			bool				load_file					(LPCSTR	caScriptName, LPCSTR namespace_name);
	IC		CScriptProcess		*script_process				(const EScriptProcessors &process_id) const;
	IC		void				add_script_process			(const EScriptProcessors &process_id, CScriptProcess *script_process);
			void				remove_script_process		(const EScriptProcessors &process_id);
			void				setup_auto_load				();
			void				process_file_if_exists		(LPCSTR file_name, bool warn_if_not_exist);
			void				process_file				(LPCSTR file_name);
			void				process_file				(LPCSTR file_name, bool reload_modules);
			bool				function_object				(LPCSTR function_to_call, luabind::object &object, int type = LUA_TFUNCTION);
			void				register_script_classes		();
	IC		void				parse_script_namespace		(LPCSTR function_to_call, LPSTR name_space, u32 const namespace_size, LPSTR function, u32 const function_size);

	template <typename _result_type>
	IC		bool				functor						(LPCSTR function_to_call, luabind::functor<_result_type> &lua_function);

#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
			void				stopDebugger				();
			void				restartDebugger				();
			CScriptDebugger		*debugger					();
#	else // ifndef USE_LUA_STUDIO
			void				try_connect_to_debugger		();
			void				disconnect_from_debugger	();
	inline cs::lua_studio::world* debugger					() const { return m_lua_studio_world; }
#	endif // ifndef USE_LUA_STUDIO
#endif
	virtual	void				on_error					(lua_State* state);
			void				collect_all_garbage			();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptEngine)
#undef script_type_list
#define script_type_list save_type_list(CScriptEngine)

#include "script_engine_inline.h"