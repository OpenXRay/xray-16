////////////////////////////////////////////////////////////////////////////
//	Module 		: lua_studio.cpp
//	Created 	: 21.08.2008
//  Modified 	: 21.08.2008
//	Author		: Dmitriy Iassenev
//	Description : lua studio engine class (copied from the lua studio SDK)
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "LuaStudio/LuaStudio.hpp"
#include <luabind/class.hpp>
#include <luabind/detail/class_rep.hpp>

using namespace luabind::detail;

#define pstr LPSTR
#define	pcstr LPCSTR
#define	pcvoid const void*
#define	sz_cmp xr_strcmp
#define	vector_class luabind::internal_vector
#define engine lua_studio_engine

inline pstr sz_cpy(pstr destination, const u32 &size, pcstr source)
{
    xr_strcpy(destination, size, source);
    return destination;
}

template<int size>
inline pstr sz_cpy(char (&destination)[size], pcstr source)
{
    xr_strcpy(destination, size, source);
    return destination;
}

inline pstr sz_cat(pstr destination, const u32 &size, pcstr source)
{
    xr_strcat(destination, size, source);
    return destination;
}

template<int size>
inline pstr sz_cat(char (&destination)[size], pcstr source)
{
    xr_strcat(destination, size, source);
    return destination;
}

inline u32 sz_len(pcstr string)
{
    return (u32)xr_strlen(string);
}

int engine::luaL_loadstring(lua_State *L, const char *s)
{ return ::luaL_loadstring(L, s); }

int engine::luaL_newmetatable(lua_State *L, const char *tname)
{ return ::luaL_newmetatable(L, tname); }

void engine::lua_createtable(lua_State *L, int narray, int nrec)
{ return ::lua_createtable(L, narray, nrec); }

int engine::lua_sethook(lua_State *L, lua_Hook func, lua_mask_type mask, int count)
{ return ::lua_sethook(L, func, mask, count); }

engine::lua_Hook engine::lua_gethook(lua_State *L)
{ return ::lua_gethook(L); }

int engine::lua_getinfo(lua_State *L, const char *what, lua_Debug *ar)
{ return ::lua_getinfo(L, what, ar); }

void engine::lua_getfenv(lua_State *L, int idx)
{ return ::lua_getfenv(L, idx); }

void engine::lua_getfield(lua_State *L, int idx, const char *k)
{ return ::lua_getfield(L, idx, k); }

const char * engine::lua_getlocal(lua_State *L, const lua_Debug *ar, int n)
{ return ::lua_getlocal(L, ar, n); }

void engine::lua_gettable(lua_State *L, int idx)
{ return ::lua_gettable(L, idx); }

int engine::lua_getstack(lua_State *L, int level, lua_Debug *ar)
{ return ::lua_getstack(L, level, ar); }

int engine::lua_gettop(lua_State *L)
{ return ::lua_gettop(L); }

const char * engine::lua_getupvalue(lua_State *L, int funcindex, int n)
{ return ::lua_getupvalue(L, funcindex, n); }

int engine::lua_iscfunction(lua_State *L, int idx)
{ return ::lua_iscfunction(L, idx); }

int engine::lua_next(lua_State *L, int idx)
{ return ::lua_next(L, idx); }

int engine::lua_pcall(lua_State *L, int nargs, int nresults, int errfunc)
{ return ::lua_pcall(L, nargs, nresults, errfunc); }

void engine::lua_pushcclosure(lua_State *L, lua_CFunction fn, int n)
{ return ::lua_pushcclosure(L, fn, n); }

void engine::lua_pushnil(lua_State *L)
{ return ::lua_pushnil(L); }

void engine::lua_pushstring(lua_State *L, const char *s)
{ return ::lua_pushstring(L, s); }

void engine::lua_pushvalue(lua_State *L, int idx)
{ return ::lua_pushvalue(L, idx); }

void engine::lua_pushnumber(lua_State *L, lua_Number idx)
{ return ::lua_pushnumber(L, idx); }

void engine::lua_remove(lua_State *L, int idx)
{ return ::lua_remove(L, idx); }

void engine::lua_replace(lua_State *L, int idx)
{ return ::lua_replace(L, idx); }

int engine::lua_setfenv(lua_State *L, int idx)
{ return ::lua_setfenv(L, idx); }

int engine::lua_setmetatable(lua_State *L, int objindex)
{ return ::lua_setmetatable(L, objindex); }

void engine::lua_settable(lua_State *L, int idx)
{ return ::lua_settable(L, idx); }

void engine::lua_settop(lua_State *L, int idx)
{ return ::lua_settop(L, idx); }

int engine::lua_toboolean(lua_State *L, int idx)
{ return ::lua_toboolean(L, idx); }

engine::lua_Integer engine::lua_tointeger(lua_State *L, int idx)
{ return ::lua_tointeger(L, idx); }

const char * engine::lua_tolstring(lua_State *L, int idx, size_t *len)
{ return ::lua_tolstring(L, idx, len); }

lua_Number engine::lua_tonumber(lua_State *L, int idx)
{ return ::lua_tonumber(L, idx); }

const void * engine::lua_topointer(lua_State *L, int idx)
{ return ::lua_topointer(L, idx); }

bool engine::lua_isnumber(lua_State *L, int idx)
{ return !!::lua_isnumber(L, idx); }

int engine::lua_type(lua_State *L, int idx)
{ return ::lua_type(L, idx); }

const char * engine::lua_typename(lua_State *L, int t)
{ return ::lua_typename(L, t); }

lua_Debug * engine::lua_debug_create()
{
    VERIFY(m_instance_count<sizeof(m_instances)/sizeof(m_instances[0]));
    return &m_instances[m_instance_count++];
}

void engine::lua_debug_destroy(lua_Debug *&instance)
{
    instance = nullptr;
    m_instance_count--;
}

const char * engine::lua_debug_get_name(lua_Debug &instance)
{ return instance.name; }

const char * engine::lua_debug_get_source(lua_Debug &instance)
{ return instance.source; }

const char * engine::lua_debug_get_short_source(lua_Debug &instance)
{ return instance.short_src; }

int engine::lua_debug_get_current_line(lua_Debug &instance)
{ return instance.currentline; }

void engine::log(log_message_types const message_type, const char *message) {}

char * engine::class_name(char *buffer, u32 size, class_rep &crep)
{
    switch (crep.get_class_type())
    {
    case class_rep::cpp_class:
        return sz_cpy(buffer, size, "C++ class");
    case class_rep::lua_class:
        return sz_cpy(buffer, size, "Lua class");
    default:
        NODEFAULT;
    }
#ifdef DEBUG
    return sz_cpy(buffer, size, "unknown user data");
#endif
}

void engine::type_convert_class(char *buffer, u32 size, lua_State *state, int index)
{
    object_rep *object = get_instance(state, index);
    VERIFY2(object, "invalid object userdata");
    sz_cpy(buffer, size, "");
    sz_cat(buffer, size, "class \"");
    sz_cat(buffer, size, object->crep()->name());
    sz_cat(buffer, size, "\" (");
    u32 length = sz_len(buffer);
    class_name(buffer+length, size-length, *object->crep());
    sz_cat(buffer, size, " instance)");
}

static bool is_luabind_class(lua_State *state, int index)
{
    class_rep *rep = static_cast<class_rep*>(lua_touserdata(state, index));
    if (!rep)
        return false;
    if (rep->get_class_type()==class_rep::lua_class)
        return true;
    if (class_registry::get_registry(state)->find_class(rep->type())!=rep)
        return false;
    return true;
}

bool engine::type_convert_instance(char *buffer, u32 size, lua_State *state, int index)
{
    if (!is_luabind_class(state, index))
        return false;
    class_name(buffer, size, *static_cast<class_rep*>(lua_touserdata(state, index)));
    return true;
}

void engine::type_convert_userdata(char *buffer, u32 size, lua_State *state, int index)
{
    if (get_instance(state, index))
    {
        type_convert_class(buffer, size, state, index);
        return;
    }
    if (type_convert_instance(buffer, size, state, index))
        return;
    sz_cpy(buffer, size, "unrecognized user data");
}

bool engine::type_to_string(char *buffer, u32 size, lua_State *state, int index, bool &use_in_description)
{
    switch (lua_type(state, index))
    {
    case engine::lua_type_string:
    case engine::lua_type_table:
    case engine::lua_type_nil:
    case engine::lua_type_boolean:
    case engine::lua_type_number:
    case engine::lua_type_function:
    case engine::lua_type_coroutine:
        return false;
    case engine::lua_type_light_user_data:
    case engine::lua_type_user_data:
        type_convert_userdata(buffer, size, state, index);
        return true;
    default: NODEFAULT;
    }
#ifdef DEBUG
    return false;
#endif
}

void engine::fill_class_info(Backend &backend, char *buffer, u32 size,
    object_rep *object, class_rep *crep, lua_State *state)
{
    R_ASSERT(!"not implemented");
    // XXX nitrocaster: fix luabind 0.9 conformance
    /*
    pstr stream = buffer;
    stream += xr_sprintf(stream, size -(stream-buffer), "{");
    typedef class_rep::property_map property_map;
    property_map::const_iterator I = crep->properties().begin();
    property_map::const_iterator E = crep->properties().end();
    for (u32 i = 0; I != E; ++I)
    {
        if (i==3)
        {
            stream += xr_sprintf(stream, size-(stream-buffer), "...");
            break;
        }
        lua_pushstring(state, (*I).first);
        lua_insert(state, 1);
        lua_pushlightuserdata(state, object);
        lua_insert(state, 1);
        (*I).second.func(state, (*I).second.pointer_offset);
        string4096 type;
        bool use_in_description;
        backend.type_to_string(type, sizeof(type), state, -1, use_in_description);
        string4096 value;
        cs::lua_studio::icon_type icon_type;
        backend.value_to_string(value, sizeof(value), state, -1, icon_type, false);
        lua_pop_value(state, 1);
        lua_remove(state, 1);
        lua_remove(state, 1);
        if (use_in_description)
            stream += xr_sprintf(stream, size-(stream-buffer), "%s[%s]=%s ", (*I).first, type, value);
        else
            stream += xr_sprintf(stream, size-(stream-buffer), "%s=%s ", (*I).first, value);
        i++;
    }
    stream += xr_sprintf(stream, size-(stream-buffer), "}%c", 0);
    */
}

void engine::value_convert_class(Backend &backend, char *buffer, u32 size,
    class_rep *crep, lua_State *state, int index, IconType &icon_type, bool full_description)
{
    R_ASSERT(!"not implemented");
    // XXX nitrocaster: fix luabind 0.9 conformance
    /*
    icon_type = cs::lua_studio::icon_type_class;
    if (!full_description)
    {
        sz_cpy(buffer, size, "{...}");
        return;
    }
    if (!crep->bases().empty())
    {
        sz_cpy(buffer, size, "{...}");
        return;
    }
    if (crep->properties().empty())
    {
        sz_cpy(buffer, size, "{}");
        return;
    }
    object_rep *object = is_class_object(state, index);
    if (!object)
    {
        sz_cpy(buffer, size, "{...}");
        return;
    }
    fill_class_info(backend, buffer, size, object, crep, state);
    */
}

bool engine::value_convert_instance(Backend &backend, char *buffer, u32 size,
    object_rep *object, lua_State *state)
{
    R_ASSERT(!"not implemented");
    return false;
    // XXX nitrocaster: fix luabind 0.9 conformance
    /*
    typedef lua_reference lua_reference;
    lua_reference const &tbl = object->get_lua_table();
    if (!tbl.is_valid())
        return false;
    pstr stream = buffer;
    stream += xr_sprintf(stream, size-(stream-buffer), "{");
    tbl.get(state);
    int i;
    lua_pushnil(state);
    for (i = 0; lua_next(state, -2); ++i)
    {
        if (i==3)
        {
            lua_pop_value(state, 2);
            stream += xr_sprintf(stream, size-(stream-buffer), "...");
            break;
        }
        pcstr name = lua_to_string(state, -2);
        string4096 type;
        bool use_in_description;
        backend.type_to_string(type, sizeof(type), state, -1, use_in_description);
        string4096 value;
        cs::lua_studio::icon_type icon_type;
        backend.value_to_string(value, sizeof(value), state, -1, icon_type, false);
        if (use_in_description)
            stream += xr_sprintf(stream, size-(stream-buffer), "%s[%s]=%s ", name, type, value);
        else
            stream += xr_sprintf(stream, size-(stream-buffer), "%s=%s ", name, value);
        lua_pop_value(state, 1);
    }
    lua_pop_value(state, 1);
    if (!i)
        return false;
    stream += xr_sprintf(stream, size-(stream-buffer), "}%c", 0);
    return true;
    */
}

bool engine::value_convert_instance(Backend &backend, char *buffer, u32 size,
    lua_State *state, int index, IconType &icon_type, bool full_description)
{
    object_rep *object = get_instance(state, index);
    if (!object)
        return false;
    if (full_description && !value_convert_instance(backend, buffer, size, object, state))
        value_convert_class(backend, buffer, size, object->crep(), state, index, icon_type, full_description);
    else
        sz_cpy(buffer, size, " ");
    icon_type = cs::lua_studio::icon_type_class_instance;
    return true;
}

bool engine::value_to_string(Backend &backend, char *buffer, u32 size,
    lua_State *state, int index, IconType &icon_type, bool full_description)
{
    switch (lua_type(state, index))
    {
    case engine::lua_type_string:
    case engine::lua_type_table:
    case engine::lua_type_nil:
    case engine::lua_type_boolean:
    case engine::lua_type_number:
    case engine::lua_type_function:
    case engine::lua_type_coroutine:
        return false;
    case engine::lua_type_light_user_data:
    case engine::lua_type_user_data:
        {
            if (!get_instance(state, index))
            {
                if (!is_luabind_class(state, index))
                {
                    icon_type = cs::lua_studio::icon_type_unknown;
                    pcvoid user_data = lua_topointer(state, index);
                    xr_sprintf(buffer, size, "0x%08x", *(const u32 *)&user_data);
                    return true;
                }
                class_rep *rep = static_cast<class_rep*>(lua_touserdata(state, index));
                VERIFY(rep);
                value_convert_class(backend, buffer, size, rep, state, index, icon_type, full_description);
                return true;
            }
            if (value_convert_instance(backend, buffer, size, state, index, icon_type, full_description))
                return true;
            icon_type = cs::lua_studio::icon_type_unknown;
            pcvoid user_data = lua_topointer(state, index);
            xr_sprintf(buffer, size, "0x%08x", *(const u32 *)&user_data);
            return true;
        }
    default:
        NODEFAULT;
    }
#ifdef DEBUG
    return false;
#endif
}

void engine::push_class(lua_State *state, const char *id)
{
    object_rep *object = get_instance(state, -1);
    VERIFY(object);
    class_rep *rep = object->crep();
    R_ASSERT2(rep, "null class userdata");
    R_ASSERT(!sz_cmp(rep->name(), id));
    lua_pushlightuserdata(state, rep);
}

void engine::push_class_base(lua_State *state, const char *id)
{
    class_rep *rep = static_cast<class_rep*>(lua_touserdata(state, -1));
    VERIFY(rep);
    for (const auto &baseInfo : rep->bases())
    {
        pcstr name = baseInfo.base->name();
        if (sz_cmp(id, name))
            continue;
        lua_pop_value(state, 1);
        lua_pushlightuserdata(state, baseInfo.base);
        return;
    }
    NODEFAULT;
}

void engine::push_class_instance(lua_State *state, const char *id)
{
    object_rep *object = get_instance(state, -1);
    if (!object)
    {
        lua_pop_value(state, 1);
        object = get_instance(state, -1);
        VERIFY (object);
    }
    lua_insert(state, 1);
    lua_pushstring(state, id);
    lua_insert(state, 2);
    object->crep()->get_table(state);
    lua_remove(state, 2);
    lua_pushvalue(state, 1);
    lua_remove(state, 1);
    lua_pushvalue(state, -2);
    lua_remove(state, -3);
    lua_remove(state, -2);
}

void engine::push_user_data(lua_State *state, const char *id, IconType icon_type)
{
    switch (icon_type)
    {
    case cs::lua_studio::icon_type_class:
        push_class(state, id);
        break;
    case cs::lua_studio::icon_type_class_base:
        push_class_base(state, id);
        break;
    case cs::lua_studio::icon_type_unknown:
    case cs::lua_studio::icon_type_table:
    case cs::lua_studio::icon_type_class_instance:
        push_class_instance(state, id);
        break;
    default:
        NODEFAULT;
    }
}

bool engine::push_value(lua_State *state, const char *id, IconType icon_type)
{
    switch (lua_type(state, -1))
    {
    case engine::lua_type_table:
        return false;
    case engine::lua_type_light_user_data:
    case engine::lua_type_user_data:
        push_user_data(state, id, icon_type);
        return true;
    default:
        NODEFAULT;
    }
#ifdef DEBUG
    return false;
#endif
}

void engine::fill_class_data(Backend &backend, ValueToExpand &value_to_expand, lua_State *state)
{
    R_ASSERT(!"not implemented");
    // XXX nitrocaster: fix luabind 0.9 conformance
    /*
    object_rep *object = static_cast<object_rep*>(lua_touserdata(state, -2));
    class_rep *crep = static_cast<class_rep*>(lua_touserdata(state, -1));
    R_ASSERT2(crep, "invalid class userdata");
    {
        string4096 type;
        typedef class_rep::base_info base_info;
        vector_class<base_info>::const_iterator i = crep->bases().begin();
        vector_class<base_info>::const_iterator e = crep->bases().end();
        for (; i != e; ++i)
        {
            value_to_expand.add_value((*i).base->name(), class_name(type, sizeof(type), *(*i).base),
                "{...}", cs::lua_studio::icon_type_class_base);
        }
    }
    if (!object)
        return;
    typedef class_rep::property_map property_map;
    property_map::const_iterator i = crep->properties().begin();
    property_map::const_iterator e = crep->properties().end();
    for (; i != e; ++i)
    {
        lua_pushstring(state, (*i).first);
        lua_insert(state, 1);
        lua_pushlightuserdata(state, object);
        lua_insert(state, 1);
        (*i).second.func(state, (*i).second.pointer_offset);
        bool use_in_description;
        string4096 type;
        backend.type_to_string(type, sizeof(type), state, -1, use_in_description);
        cs::lua_studio::icon_type icon_type;
        string4096 value;
        backend.value_to_string(value, sizeof(value), state, -1, icon_type, true);
        lua_pop_value(state, 1);
        lua_remove(state, 1);
        lua_remove(state, 1);
        value_to_expand.add_value((*i).first, type, value, icon_type);
    }
    */
}

void engine::expand_class(Backend &backend, ValueToExpand &value, lua_State *state)
{
    int start = lua_gettop(state);
    class_rep *crep = static_cast<class_rep*>(lua_touserdata(state, -1));
    R_ASSERT2(crep, "invalid class userdata");
    fill_class_data(backend, value, state);
    object_rep *object = get_instance(state, -2);
    if (!object)
        lua_pushnil(state);
    if (lua_gettop(state)<=start+1)
        return;
    lua_pop_value(state, 1);
}

void engine::expand_class_instance(Backend &backend, ValueToExpand &value_to_expand, lua_State *state)
{
    R_ASSERT(!"not implemented");
    // XXX nitrocaster: fix luabind 0.9 conformance
    /*
    typedef object_rep object_rep;
    object_rep *object = get_instance(state, -1);
    VERIFY2(object, "invalid object userdata");
    if (object->crep())
    {
        class_rep *crep = object->crep();
        string4096 type;
        class_name(type, sizeof(type), *crep);
        cs::lua_studio::icon_type icon_type;
        string4096 value;
        backend.value_to_string(value, sizeof(value), state, -1, icon_type, true);
        value_to_expand.add_value(crep->name(), type, value, cs::lua_studio::icon_type_class);
    }
    typedef lua_reference lua_reference;
    lua_reference const &tbl = object->get_lua_table();
    if (!tbl.is_valid())
        return;
    tbl.get(state);
    int i;
    lua_pushnil(state);
    for (i = 0; lua_next(state, -2); i++)
    {
        cs::lua_studio::icon_type icon_type;
        bool use_in_description;
        pcstr name = lua_to_string(state, -2);
        string4096 type;
        backend.type_to_string(type, sizeof(type), state, -1, use_in_description);
        string4096 value;
        backend.value_to_string(value, sizeof(value), state, -1, icon_type, true);
        value_to_expand.add_value(name, type, value, icon_type);
        lua_pop_value(state, 1);
    }
    lua_pop_value(state, 1);
    */
}

void engine::expand_user_data(Backend &backend, ValueToExpand &value, lua_State *state)
{
    object_rep *object = get_instance(state, -1);
    if (object)
    {
        expand_class_instance(backend, value, state);
        lua_pop_value(state, 1);
        return;
    }
    expand_class(backend, value, state);
    lua_pop_value(state, 2);
}

bool engine::expand_value(Backend &backend, ValueToExpand &value, lua_State *state)
{
    switch (lua_type(state, -1))
    {
    case engine::lua_type_nil:
    case engine::lua_type_table:
        return false;
    case engine::lua_type_light_user_data:
    case engine::lua_type_user_data:
        expand_user_data(backend, value, state);
        return true;
    default:
        NODEFAULT;
    }
#ifdef DEBUG
    return false;
#endif
}

engine::engine() { m_instance_count = 0; }
