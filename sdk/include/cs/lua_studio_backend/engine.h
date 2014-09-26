////////////////////////////////////////////////////////////////////////////
//	Module 		: engine.h
//	Created 	: 10.04.2008
//  Modified 	: 10.04.2008
//	Author		: Dmitriy Iassenev
//	Description : script debugger engine  class
////////////////////////////////////////////////////////////////////////////

#ifndef CS_LUA_STUDIO_BACKEND_ENGINE_H_INCLUDED
#define CS_LUA_STUDIO_BACKEND_ENGINE_H_INCLUDED

struct lua_State;
struct lua_Debug;

namespace cs {
namespace lua_studio {

struct backend;

struct DECLSPEC_NOVTABLE engine {
public:
	enum lua_hook_type {
		lua_hook_call						= int(0),
		lua_hook_return						= int(1),
		lua_hook_line						= int(2),
		lua_hook_count						= int(3),
		lua_hook_tail_return				= int(4),
	}; // enum lua_mask_type

	enum lua_mask_type {
		lua_mask_call						= int(1) << lua_hook_call,
		lua_mask_return						= int(1) << lua_hook_return,
		lua_mask_line						= int(1) << lua_hook_line,
		lua_mask_count						= int(1) << lua_hook_count,
	}; // enum lua_mask_type

	enum lua_return_code_type {
		lua_return_code_error_no_error		= int(-1),
		lua_return_code_yield				= int(1),
		lua_return_code_runtime_error		= int(2),
		lua_return_code_syntax_error		= int(3),
		lua_return_code_out_of_memory		= int(4),
		lua_return_code_error_in_error		= int(5),
		lua_return_code_error_running_file	= int(6),
	}; // enum lua_return_code_type

	enum lua_types {
		lua_type_nil						= int(0),
		lua_type_boolean					= int(1),
		lua_type_light_user_data			= int(2),
		lua_type_number						= int(3),
		lua_type_string						= int(4),
		lua_type_table						= int(5),
		lua_type_function					= int(6),
		lua_type_user_data					= int(7),
		lua_type_coroutine					= int(8),
	}; // enum lua_type

	enum lua_indices {
		lua_globals_index					= int(-10002),
	}; // enum lua_indices

	enum lua_pcall_options {
		lua_multiple_return					= int(-1),
	}; // enum lua_pcall_options

public:
	enum log_message_types {
		message_type_error					= int(0),
		message_type_warning,
		message_type_output,
		message_type_information,
	}; // enum log_message_types

public:
	typedef double			lua_Number;
	typedef ptrdiff_t		lua_Integer;

public:
	typedef	void			(*lua_Hook)												(lua_State *L, lua_Debug *ar);
	typedef int				(*lua_CFunction)										(lua_State *L);

public:
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	luaL_loadstring				(lua_State *L, const char *s) = 0;
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	luaL_newmetatable			(lua_State *L, const char *tname) = 0;

public:
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_createtable				(lua_State *L, int narray, int nrec) = 0;

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_sethook					(lua_State *L, lua_Hook func, lua_mask_type mask, int count) = 0;
	virtual	lua_Hook		CS_LUA_STUDIO_BACKEND_CALL	lua_gethook					(lua_State *L) = 0;

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_getinfo					(lua_State *L, const char *what, lua_Debug *ar) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_getfenv					(lua_State *L, int idx) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_getfield				(lua_State *L, int idx, const char *k) = 0;
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_getlocal				(lua_State *L, const lua_Debug *ar, int n) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_gettable				(lua_State *L, int idx) = 0;
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_getstack				(lua_State *L, int level, lua_Debug *ar) = 0;
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_gettop					(lua_State *L) = 0;
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_getupvalue				(lua_State *L, int funcindex, int n) = 0;

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_iscfunction				(lua_State *L, int idx) = 0;
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_next					(lua_State *L, int idx) = 0;

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_pcall					(lua_State *L, int nargs, int nresults, int errfunc) = 0;

	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushcclosure			(lua_State *L, lua_CFunction fn, int n) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushnil					(lua_State *L) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushstring				(lua_State *L, const char *s) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushvalue				(lua_State *L, int idx) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushnumber				(lua_State *L, lua_Number idx) = 0;

	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_remove					(lua_State *L, int idx) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_replace					(lua_State *L, int idx) = 0;

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_setfenv					(lua_State *L, int idx) = 0;
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_setmetatable			(lua_State *L, int objindex) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_settable				(lua_State *L, int idx) = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_settop					(lua_State *L, int idx) = 0;

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_toboolean				(lua_State *L, int idx) = 0;
	virtual	lua_Integer		CS_LUA_STUDIO_BACKEND_CALL	lua_tointeger				(lua_State *L, int idx) = 0;
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_tolstring				(lua_State *L, int idx, size_t *len) = 0;
	virtual	lua_Number		CS_LUA_STUDIO_BACKEND_CALL	lua_tonumber				(lua_State *L, int idx) = 0;
	virtual	const void*		CS_LUA_STUDIO_BACKEND_CALL	lua_topointer				(lua_State *L, int idx) = 0;

	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	lua_isnumber				(lua_State *L, int idx) = 0;
	
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_type					(lua_State *L, int idx) = 0;
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_typename				(lua_State *L, int t) = 0;

public:
	inline	bool										lua_typestring				(lua_State *L, int n)				{ return (this->lua_type(L, n) == lua_type_string);	}
	inline	bool										lua_typenumber				(lua_State *L, int n)				{ return (this->lua_type(L, n) == lua_type_number);	}
	inline	bool										lua_is_table				(lua_State *L, int n)				{ return (lua_type(L, n) == lua_type_table);	}
	inline	bool										lua_is_nil					(lua_State *L, int n)				{ return (lua_type(L, n) == lua_type_nil);		}
	inline	void										lua_new_table				(lua_State *L)						{ lua_createtable(L, 0, 0);						}
	inline	void										lua_pop_value				(lua_State *L, int n)				{ lua_settop(L, -n-1);						}
	inline	void										lua_push_c_function			(lua_State *L, lua_CFunction fn)	{ lua_pushcclosure(L, fn, 0);					}
	inline	char const*									lua_to_string				(lua_State *L, int n)				{ return (lua_tolstring(L, n, NULL));			}

public:
	virtual	lua_Debug*		CS_LUA_STUDIO_BACKEND_CALL	lua_debug_create			() = 0;
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_debug_destroy			(lua_Debug*& instance) = 0;
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_debug_get_name			(lua_Debug& instance) = 0;
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_debug_get_source		(lua_Debug& instance) = 0;
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_debug_get_short_source	(lua_Debug& instance) = 0;
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_debug_get_current_line	(lua_Debug& instance) = 0;

public:
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	log							(log_message_types message_type, char const* message) = 0;
	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	type_to_string				(char* buffer, unsigned int size, lua_State* state, int index, bool& use_in_description) = 0;
	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	value_to_string				(cs::lua_studio::backend& backend, char* buffer, unsigned int size, lua_State* state, int index, icon_type& icon_type, bool full_description) = 0;
	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	push_value					(lua_State* state, char const* id, icon_type icon_type) = 0;
	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	expand_value				(cs::lua_studio::backend& backend, cs::lua_studio::value_to_expand& value, lua_State* state) = 0;
}; // struct DECLSPEC_NOVTABLE engine

} // namespace lua_studio
} // namespace cs

#endif // #ifndef CS_LUA_STUDIO_BACKEND_ENGINE_H_INCLUDED