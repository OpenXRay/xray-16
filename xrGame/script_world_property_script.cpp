////////////////////////////////////////////////////////////////////////////
//	Module 		: script_world_property_script.h
//	Created 	: 19.03.2004
//  Modified 	: 19.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script world property script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_world_property.h"
#include "operator_abstract.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptWorldPropertyWrapper::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptWorldProperty>("world_property")
			.def(								constructor<CScriptWorldProperty::_condition_type, CScriptWorldProperty::_value_type>())
			.def("condition",					&CScriptWorldProperty::condition)
			.def("value",						&CScriptWorldProperty::value)
			.def(const_self < other<CScriptWorldProperty>())
			.def(const_self == other<CScriptWorldProperty>())
	];
}