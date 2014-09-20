////////////////////////////////////////////////////////////////////////////
//	Module 		: action_planner_script_inline.h
//	Created 	: 28.03.2004
//  Modified 	: 28.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Action planner with script support inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <typename _object_type>
#define _CActionPlannerScript	CActionPlannerScript<_object_type>

TEMPLATE_SPECIALIZATION
IC	_CActionPlannerScript::CActionPlannerScript		()
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
void _CActionPlannerScript::setup					(_object_type *object)
{
	VERIFY				(object);
	inherited::setup	(object->lua_game_object());
	m_object			= object;
}

TEMPLATE_SPECIALIZATION
IC _object_type &_CActionPlannerScript::object		() const
{
	return				(*m_object);
}

#undef TEMPLATE_SPECIALIZATION
#undef _CActionPlannerScript