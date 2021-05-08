#include "pch_script.h"

#include "PHScriptCall.h"
#include "PHCommander.h"

#include "xrEngine/xr_object.h"

/*
IC bool compare_safe(const luabind::object &o1 , const luabind::object &o2)
{
    return (o1.type()==LUA_TNIL && o2.type()==LUA_TNIL) || o1==o2;
}
**/

CPHScriptCondition::CPHScriptCondition(const luabind::functor<bool>& func)
{
    m_lua_function = xr_new<luabind::functor<bool>>(func);
}

CPHScriptCondition::CPHScriptCondition(const CPHScriptCondition& func)
{
    m_lua_function = xr_new<luabind::functor<bool>>(*func.m_lua_function);
}

CPHScriptCondition::~CPHScriptCondition() { xr_delete(m_lua_function); }
bool CPHScriptCondition::is_true() { return (*m_lua_function)(); }
bool CPHScriptCondition::obsolete() const { return false; }
//
CPHScriptAction::CPHScriptAction(const luabind::functor<void>& func)
{
    b_obsolete = false;
    m_lua_function = xr_new<luabind::functor<void>>(func);
}

CPHScriptAction::CPHScriptAction(const CPHScriptAction& action)
{
    b_obsolete = action.b_obsolete;
    m_lua_function = xr_new<luabind::functor<void>>(*action.m_lua_function);
}

CPHScriptAction::~CPHScriptAction() { xr_delete(m_lua_function); }
void CPHScriptAction::run()
{
    (*m_lua_function)();
    b_obsolete = true;
}

bool CPHScriptAction::obsolete() const { return b_obsolete; }
/////////////////////////////////////////////////////////////////////////////////////////////
CPHScriptObjectAction::CPHScriptObjectAction(const luabind::object& lua_object, LPCSTR method)
    : b_obsolete(false), m_lua_object(xr_new<luabind::object>(lua_object)), m_method_name(method)
{
}

CPHScriptObjectAction::CPHScriptObjectAction(const CPHScriptObjectAction& object)
{
    b_obsolete = object.b_obsolete;
    m_lua_object = xr_new<luabind::object>(*object.m_lua_object);
    m_method_name = object.m_method_name;
}

CPHScriptObjectAction::~CPHScriptObjectAction() { xr_delete(m_lua_object); }
bool CPHScriptObjectAction::compare(const CPHScriptObjectAction* v) const
{
    return m_method_name == v->m_method_name && compare_safe(*m_lua_object, *(v->m_lua_object));
}
void CPHScriptObjectAction::run()
{
    luabind::call_member<void>(*m_lua_object, *m_method_name);
    b_obsolete = true;
}

bool CPHScriptObjectAction::obsolete() const { return b_obsolete; }
//
CPHScriptObjectCondition::CPHScriptObjectCondition(const luabind::object& lua_object, LPCSTR method)
{
    m_lua_object = xr_new<luabind::object>(lua_object);
    m_method_name = method;
}

CPHScriptObjectCondition::CPHScriptObjectCondition(const CPHScriptObjectCondition& object)
{
    m_lua_object = xr_new<luabind::object>(*object.m_lua_object);
    m_method_name = object.m_method_name;
}

CPHScriptObjectCondition::~CPHScriptObjectCondition() { xr_delete(m_lua_object); }
bool CPHScriptObjectCondition::compare(const CPHScriptObjectCondition* v) const
{
    return m_method_name == v->m_method_name && compare_safe(*m_lua_object, *(v->m_lua_object));
}

bool CPHScriptObjectCondition::is_true() { return luabind::call_member<bool>(*m_lua_object, *m_method_name); }
bool CPHScriptObjectCondition::obsolete() const { return false; }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CPHScriptObjectActionN::CPHScriptObjectActionN(const luabind::object& object, const luabind::functor<void>& functor)
    : b_obsolete(false)
{
    m_callback.set(functor, object);
}

CPHScriptObjectActionN::~CPHScriptObjectActionN() { m_callback.clear(); }
void CPHScriptObjectActionN::run()
{
    m_callback();
    b_obsolete = true;
}

bool CPHScriptObjectActionN::obsolete() const { return b_obsolete; }
CPHScriptObjectConditionN::CPHScriptObjectConditionN(
    const luabind::object& object, const luabind::functor<bool>& functor)
{
    m_callback.set(functor, object);
}

CPHScriptObjectConditionN::~CPHScriptObjectConditionN() { m_callback.clear(); }
bool CPHScriptObjectConditionN::is_true() { return m_callback(); }
bool CPHScriptObjectConditionN::obsolete() const { return false; }
