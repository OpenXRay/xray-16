////////////////////////////////////////////////////////////////////////////
//	Module 		: action_planner_action_script_inline.h
//	Created 	: 07.07.2004
//  Modified 	: 07.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Action planner action with script support inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION		template <typename _object_type>
#define CSActionPlannerActionScript	CActionPlannerActionScript<_object_type>

TEMPLATE_SPECIALIZATION
IC	CSActionPlannerActionScript::CActionPlannerActionScript		(const xr_vector<COperatorCondition> &conditions, const xr_vector<COperatorCondition> &effects, _object_type *object, LPCSTR action_name) :
	inherited			(conditions,effects,object ? object->lua_game_object() : 0,action_name)
{
	m_object			= object;
}

TEMPLATE_SPECIALIZATION
IC	CSActionPlannerActionScript::CActionPlannerActionScript		(_object_type *object, LPCSTR action_name) :
	inherited			(object ? object->lua_game_object() : 0,action_name)
{
	m_object			= object;
}

TEMPLATE_SPECIALIZATION
CSActionPlannerActionScript::~CActionPlannerActionScript		()
{
}

TEMPLATE_SPECIALIZATION
void CSActionPlannerActionScript::setup		(_object_type *object, CPropertyStorage *storage)
{
	VERIFY				(object);
	VERIFY				(m_object);
}

TEMPLATE_SPECIALIZATION
void CSActionPlannerActionScript::setup		(CScriptGameObject *object, CPropertyStorage *storage)
{
	VERIFY				(object);
	inherited::setup	(object,storage);
	m_object			= smart_cast<_object_type*>(&object->object());
	VERIFY				(m_object);
	setup				(m_object,storage);
}

TEMPLATE_SPECIALIZATION
IC	_object_type &CSActionPlannerActionScript::object	() const
{
	VERIFY				(m_object);
	return				(*m_object);
}

#undef TEMPLATE_SPECIALIZATION
#undef CSActionPlannerActionScript