////////////////////////////////////////////////////////////////////////////
//	Module 		: property_evaluator_script.cpp
//	Created 	: 12.03.2004
//  Modified 	: 12.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property evaluator script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_property_evaluator_wrapper.h"
#include "script_game_object.h"
#include "property_evaluator_const.h"

using namespace luabind;

#pragma optimize("s",on)
void CPropertyEvaluator<CScriptGameObject>::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptPropertyEvaluator,CScriptPropertyEvaluatorWrapper>("property_evaluator")
			.def_readonly("object",				&CScriptPropertyEvaluator::m_object)
			.def_readonly("storage",			&CScriptPropertyEvaluator::m_storage)
			.def(								constructor<>())
			.def(								constructor<CScriptGameObject*>())
			.def(								constructor<CScriptGameObject*,LPCSTR>())
			.def("setup",						&CScriptPropertyEvaluator::setup, &CScriptPropertyEvaluatorWrapper::setup_static)
			.def("evaluate",					&CScriptPropertyEvaluator::evaluate, &CScriptPropertyEvaluatorWrapper::evaluate_static),

		class_<CPropertyEvaluatorConst<CScriptGameObject>, CScriptPropertyEvaluator>("property_evaluator_const")
			.def(								constructor<CPropertyEvaluatorConst<CScriptGameObject>::_value_type>())
	];
}
