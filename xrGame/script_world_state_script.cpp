////////////////////////////////////////////////////////////////////////////
//	Module 		: script_world_state_script.cpp
//	Created 	: 19.03.2004
//  Modified 	: 19.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script world state script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_world_state.h"
#include "condition_state.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptWorldStateWrapper::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptWorldState>("world_state")
			.def(								constructor<>())
			.def(								constructor<CScriptWorldState>())
			.def("add_property",				(void (CScriptWorldState::*)(const CScriptWorldState::COperatorCondition &))(&CScriptWorldState::add_condition))
			.def("remove_property",				(void (CScriptWorldState::*)(const CScriptWorldState::COperatorCondition::_condition_type &))(&CScriptWorldState::remove_condition))
			.def("clear",						&CScriptWorldState::clear)
			.def("includes",					&CScriptWorldState::includes)
			.def("property",					&CScriptWorldState::property)
			.def(const_self < CScriptWorldState())
			.def(const_self == CScriptWorldState())
	];
}
