////////////////////////////////////////////////////////////////////////////
//	Module 		: lua_studio.h
//	Created 	: 21.08.2008
//  Modified 	: 21.08.2008
//	Author		: Dmitriy Iassenev
//	Description : lua studio engine class (copied from the lua studio SDK)
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef DEBUG
#define CS_LUA_DEBUGGER_USE_DEBUG_LIBRARY
#endif

#include "xrCore/xrCore.h"
#include "xrScriptEngine/xrScriptEngine.hpp"
#include "xrScriptEngine/LuaStudio/Backend/Interfaces.hpp"
#include "Common/Noncopyable.hpp"

namespace luabind
{
namespace detail
{
class class_rep;
}
}

class XRSCRIPTENGINE_API lua_studio_engine :
    public cs::lua_studio::engine,
    private Noncopyable
{
public:
    using Backend = cs::lua_studio::backend;
    using IconType = cs::lua_studio::icon_type;
    using ValueToExpand = cs::lua_studio::value_to_expand;

private:
    lua_Debug m_instances[2];
    u32 m_instance_count;

public:
    lua_studio_engine();

#define BACKEND_CALL CS_LUA_STUDIO_BACKEND_CALL
    virtual int BACKEND_CALL luaL_loadstring(lua_State *L, const char *s);
    virtual int BACKEND_CALL luaL_newmetatable(lua_State *L, const char *tname);
    virtual void BACKEND_CALL lua_createtable(lua_State *L, int narray, int nrec);
    virtual int BACKEND_CALL lua_sethook(lua_State *L, lua_Hook func, lua_mask_type mask, int count);
    virtual lua_Hook BACKEND_CALL lua_gethook(lua_State *L);
    virtual int BACKEND_CALL lua_getinfo(lua_State *L, const char *what, lua_Debug *ar);
    virtual void BACKEND_CALL lua_getfenv(lua_State *L, int idx);
    virtual void BACKEND_CALL lua_getfield(lua_State *L, int idx, const char *k);
    virtual const char * BACKEND_CALL lua_getlocal(lua_State *L, const lua_Debug *ar, int n);
    virtual void BACKEND_CALL lua_gettable(lua_State *L, int idx);
    virtual int BACKEND_CALL lua_getstack(lua_State *L, int level, lua_Debug *ar);
    virtual int BACKEND_CALL lua_gettop(lua_State *L);
    virtual const char * BACKEND_CALL lua_getupvalue(lua_State *L, int funcindex, int n);
    virtual int BACKEND_CALL lua_iscfunction(lua_State *L, int idx);
    virtual int BACKEND_CALL lua_next(lua_State *L, int idx);
    virtual int BACKEND_CALL lua_pcall(lua_State *L, int nargs, int nresults, int errfunc);
    virtual void BACKEND_CALL lua_pushcclosure(lua_State *L, lua_CFunction fn, int n);
    virtual void BACKEND_CALL lua_pushnil(lua_State *L);
    virtual void BACKEND_CALL lua_pushstring(lua_State *L, const char *s);
    virtual void BACKEND_CALL lua_pushvalue(lua_State *L, int idx);
    virtual void BACKEND_CALL lua_pushnumber(lua_State *L, lua_Number idx);
    virtual void BACKEND_CALL lua_remove(lua_State *L, int idx);
    virtual void BACKEND_CALL lua_replace(lua_State *L, int idx);
    virtual int BACKEND_CALL lua_setfenv(lua_State *L, int idx);
    virtual int BACKEND_CALL lua_setmetatable(lua_State *L, int objindex);
    virtual void BACKEND_CALL lua_settable(lua_State *L, int idx);
    virtual void BACKEND_CALL lua_settop(lua_State *L, int idx);
    virtual int BACKEND_CALL lua_toboolean(lua_State *L, int idx);
    virtual lua_Integer BACKEND_CALL lua_tointeger(lua_State *L, int idx);
    virtual const char * BACKEND_CALL lua_tolstring(lua_State *L, int idx, size_t *len);
    virtual lua_Number BACKEND_CALL lua_tonumber(lua_State *L, int idx);
    virtual const void * BACKEND_CALL lua_topointer(lua_State *L, int idx);
    virtual bool BACKEND_CALL lua_isnumber(lua_State *L, int idx);
    virtual int BACKEND_CALL lua_type(lua_State *L, int idx);
    virtual const char * BACKEND_CALL lua_typename(lua_State *L, int t);
    virtual lua_Debug * BACKEND_CALL lua_debug_create();
    virtual void BACKEND_CALL lua_debug_destroy(lua_Debug *&instance);
    virtual const char * BACKEND_CALL lua_debug_get_name(lua_Debug &instance);
    virtual const char * BACKEND_CALL lua_debug_get_source(lua_Debug &instance);
    virtual const char * BACKEND_CALL lua_debug_get_short_source(lua_Debug &instance);
    virtual int BACKEND_CALL lua_debug_get_current_line(lua_Debug &instance);
    virtual void BACKEND_CALL log(log_message_types message_type, const char *message);
    virtual bool BACKEND_CALL type_to_string(char *buffer, u32 size, lua_State *state, int index, bool &use_in_description);
    virtual bool BACKEND_CALL value_to_string(Backend &backend, char *buffer, u32 size, lua_State *state, int index, IconType &icon_type, bool full_description);
    virtual bool BACKEND_CALL expand_value(Backend &backend, ValueToExpand &value, lua_State *state);
    virtual bool BACKEND_CALL push_value(lua_State *state, const char *id, IconType icon_type);
#undef BACKEND_CALL

private:
    void type_convert_class(char *buffer, u32 size, lua_State *state, int index);
    bool type_convert_instance(char *buffer, u32 size, lua_State *state, int index);
    void type_convert_userdata(char *buffer, u32 size, lua_State *state, int index);
    static char *class_name(char *buffer, u32 size, luabind::detail::class_rep &crep);
    void fill_class_info(Backend &backend, char *buffer, u32 size, luabind::detail::object_rep *object, luabind::detail::class_rep *crep, lua_State *state);
    void value_convert_class(Backend &backend, char *buffer, u32 size, luabind::detail::class_rep *crep, lua_State *state, int index, IconType &icon_type, bool full_description);
    bool value_convert_instance(Backend &backend, char *buffer, u32 size, luabind::detail::object_rep *object, lua_State *state);
    bool value_convert_instance(Backend &backend, char *buffer, u32 size, lua_State *state, int index, IconType &icon_type, bool full_description);
    void push_class(lua_State *state, const char *id);
    void push_class_base(lua_State *state, const char *id);
    void push_class_instance(lua_State *state, const char *id);
    void push_user_data(lua_State *state, const char *id, IconType icon_type);
    void fill_class_data(Backend &backend, ValueToExpand &value_to_expand, lua_State *state);
    void expand_class(Backend &backend, ValueToExpand &value, lua_State *state);
    void expand_class_instance(Backend &backend, ValueToExpand &value_to_expand, lua_State *state);
    void expand_user_data(Backend &backend, ValueToExpand &value, lua_State *state);
};
