////////////////////////////////////////////////////////////////////////////
//	Module 		: action_base_script.cpp
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Base action script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_action_wrapper.h"
#include "script_game_object.h"

using namespace luabind;

#pragma optimize("s",on)
void CActionBase<CScriptGameObject>::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptActionBase,CScriptActionWrapper>("action_base")
			.def_readonly("object",				&CScriptActionBase::m_object)
			.def_readonly("storage",			&CScriptActionBase::m_storage)
			.def(								constructor<>())
			.def(								constructor<CScriptGameObject*>())
			.def(								constructor<CScriptGameObject*,LPCSTR>())
			.def("add_precondition",			(void (CScriptActionBase::*)(const CScriptActionBase::COperatorCondition &))(&CScriptActionBase::add_condition))
			.def("add_effect",					(void (CScriptActionBase::*)(const CScriptActionBase::COperatorCondition &))(&CScriptActionBase::add_effect))
			.def("remove_precondition",			(void (CScriptActionBase::*)(const CScriptActionBase::COperatorCondition::_condition_type &))(&CScriptActionBase::remove_condition))
			.def("remove_effect",				(void (CScriptActionBase::*)(const CScriptActionBase::COperatorCondition::_condition_type &))(&CScriptActionBase::remove_effect))
			.def("setup",						&CScriptActionBase::setup,		&CScriptActionWrapper::setup_static)
			.def("initialize",					&CScriptActionBase::initialize, &CScriptActionWrapper::initialize_static)
			.def("execute",						&CScriptActionBase::execute,	&CScriptActionWrapper::execute_static)
			.def("finalize",					&CScriptActionBase::finalize,	&CScriptActionWrapper::finalize_static)
//			.def("weight",						&CScriptActionBase::weight,		&CScriptActionWrapper::weight_static)
			.def("set_weight",					&CScriptActionBase::set_weight)
#ifdef LOG_ACTION
			.def("show",						&CScriptActionBase::show)
#endif
	];
}
