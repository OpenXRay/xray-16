#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterMoveToRestrictorAbstract CStateMonsterMoveToRestrictor<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveToRestrictorAbstract::initialize()
{
	inherited::initialize();
	object->path().prepare_builder();

	Fvector position;
	u32		node = object->control().path_builder().restrictions().accessible_nearest(object->Position(), position);
	object->path().set_target_point	(ai().level_graph().vertex_position(node), node);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveToRestrictorAbstract::execute()
{
	object->set_action					(ACT_RUN);
	
	object->anim().accel_activate		(EAccelType(eAT_Aggressive));
	object->anim().accel_set_braking	(true);
	object->set_state_sound				(MonsterSound::eMonsterSoundIdle);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterMoveToRestrictorAbstract::check_start_conditions()
{
	return (!object->control().path_builder().accessible(object->Position()));
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterMoveToRestrictorAbstract::check_completion()
{
	return (object->control().path_builder().accessible(object->Position()));
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterMoveToRestrictorAbstract