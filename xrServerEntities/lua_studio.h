////////////////////////////////////////////////////////////////////////////
//	Module 		: lua_studio.h
//	Created 	: 21.08.2008
//  Modified 	: 21.08.2008
//	Author		: Dmitriy Iassenev
//	Description : lua studio engine class (copied from the lua studio SDK)
////////////////////////////////////////////////////////////////////////////

#ifndef LUA_STUDIO_H_INCLUDED
#define LUA_STUDIO_H_INCLUDED

#ifdef DEBUG
#	define CS_LUA_DEBUGGER_USE_DEBUG_LIBRARY
#endif// #ifdef DEBUG

#include <cs/lua_studio_backend/interfaces.h>
#include <boost/noncopyable.hpp>

namespace luabind {
	namespace detail {
		class class_rep;
	} // namespace detail
} // namespace luabind

class lua_studio_engine :
	public cs::lua_studio::engine,
	private boost::noncopyable
{
public:
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	luaL_loadstring				(lua_State *L, const char *s);
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	luaL_newmetatable			(lua_State *L, const char *tname);

public:
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_createtable				(lua_State *L, int narray, int nrec);

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_sethook					(lua_State *L, lua_Hook func, lua_mask_type mask, int count);
	virtual	lua_Hook		CS_LUA_STUDIO_BACKEND_CALL	lua_gethook					(lua_State *L);

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_getinfo					(lua_State *L, const char *what, lua_Debug *ar);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_getfenv					(lua_State *L, int idx);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_getfield				(lua_State *L, int idx, const char *k);
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_getlocal				(lua_State *L, const lua_Debug *ar, int n);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_gettable				(lua_State *L, int idx);
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_getstack				(lua_State *L, int level, lua_Debug *ar);
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_gettop					(lua_State *L);
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_getupvalue				(lua_State *L, int funcindex, int n);

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_iscfunction				(lua_State *L, int idx);
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_next					(lua_State *L, int idx);

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_pcall					(lua_State *L, int nargs, int nresults, int errfunc);

	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushcclosure			(lua_State *L, lua_CFunction fn, int n);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushnil					(lua_State *L);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushstring				(lua_State *L, const char *s);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushvalue				(lua_State *L, int idx);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_pushnumber				(lua_State *L, lua_Number idx);

	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_remove					(lua_State *L, int idx);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_replace					(lua_State *L, int idx);

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_setfenv					(lua_State *L, int idx);
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_setmetatable			(lua_State *L, int objindex);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_settable				(lua_State *L, int idx);
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_settop					(lua_State *L, int idx);

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_toboolean				(lua_State *L, int idx);
	virtual	lua_Integer		CS_LUA_STUDIO_BACKEND_CALL	lua_tointeger				(lua_State *L, int idx);
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_tolstring				(lua_State *L, int idx, size_t *len);
	virtual	lua_Number		CS_LUA_STUDIO_BACKEND_CALL	lua_tonumber				(lua_State *L, int idx);
	virtual	const void*		CS_LUA_STUDIO_BACKEND_CALL	lua_topointer				(lua_State *L, int idx);

	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	lua_isnumber				(lua_State *L, int idx);

	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_type					(lua_State *L, int idx);
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_typename				(lua_State *L, int t);

public:
	virtual	lua_Debug*		CS_LUA_STUDIO_BACKEND_CALL	lua_debug_create			();
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	lua_debug_destroy			(lua_Debug*& instance);
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_debug_get_name			(lua_Debug& instance);
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_debug_get_source		(lua_Debug& instance);
	virtual	char const*		CS_LUA_STUDIO_BACKEND_CALL	lua_debug_get_short_source	(lua_Debug& instance);
	virtual	int				CS_LUA_STUDIO_BACKEND_CALL	lua_debug_get_current_line	(lua_Debug& instance);

public:
	virtual	void			CS_LUA_STUDIO_BACKEND_CALL	log							(log_message_types message_type, char const* message);
	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	type_to_string				(char* buffer, unsigned int size, lua_State* state, int index, bool& use_in_description);
	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	value_to_string				(cs::lua_studio::backend& backend, char* buffer, unsigned int size, lua_State* state, int index, cs::lua_studio::icon_type& icon_type, bool full_description);
	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	expand_value				(cs::lua_studio::backend& backend, cs::lua_studio::value_to_expand& value, lua_State* state);
	virtual	bool			CS_LUA_STUDIO_BACKEND_CALL	push_value					(lua_State* state, char const* id, cs::lua_studio::icon_type icon_type);

public:
													lua_studio_engine			();

private:
			void 									type_convert_class			(char *buffer, unsigned int size, lua_State *state, int index);
			bool 									type_convert_instance		(char *buffer, unsigned int size, lua_State *state, int index);
			void 									type_convert_userdata		(char *buffer, unsigned int size, lua_State *state, int index);
	static	char*									class_name					(char *buffer, unsigned int size, luabind::detail::class_rep& class_rep);

private:
			void									fill_class_info				(cs::lua_studio::backend& backend, char* buffer, unsigned int size, luabind::detail::object_rep *object, luabind::detail::class_rep *class_rep, lua_State* state);
			void									value_convert_class			(cs::lua_studio::backend& backend, char* buffer, unsigned int size, luabind::detail::class_rep* class_rep, lua_State* state, int index, cs::lua_studio::icon_type& icon_type, bool full_description);
			bool									value_convert_instance		(cs::lua_studio::backend& backend, char* buffer, unsigned int size, luabind::detail::object_rep* object, lua_State* state);
			bool									value_convert_instance		(cs::lua_studio::backend& backend, char* buffer, unsigned int size, lua_State* state, int index, cs::lua_studio::icon_type& icon_type, bool full_description);

private:
			void									push_class					(lua_State* state, char const* id);
			void									push_class_base				(lua_State* state, char const* id);
			void									push_class_instance			(lua_State* state, char const* id);
			void									push_user_data				(lua_State* state, char const* id, cs::lua_studio::icon_type icon_type);

private:
			void									fill_class_data				(
														cs::lua_studio::backend& backend,
														cs::lua_studio::value_to_expand& value_to_expand,
														lua_State* const state
													);
			void									expand_class				(
														cs::lua_studio::backend& backend,
														cs::lua_studio::value_to_expand& value,
														lua_State* const state
													);
			void									expand_class_instance		(
														cs::lua_studio::backend& backend,
														cs::lua_studio::value_to_expand& value_to_expand,
														lua_State* const state
													);
			void									expand_user_data			(
														cs::lua_studio::backend& backend,
														cs::lua_studio::value_to_expand& value,
														lua_State* const state
													);

private:
	lua_Debug				m_instances[2];
	u32						m_instance_count;
};

#endif // #ifndef LUA_STUDIO_H_INCLUDED