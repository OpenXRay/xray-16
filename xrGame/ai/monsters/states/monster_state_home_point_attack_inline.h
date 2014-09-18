#pragma once

#include "state_move_to_point.h"
#include "state_look_point.h"
#include "state_custom_action.h"
#include "../../../cover_point.h"
#include "../monster_cover_manager.h"
#include "../monster_home.h"


#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterAttackMoveToHomePointAbstract CStateMonsterAttackMoveToHomePoint<_Object>

//////////////////////////////////////////////////////////////////////////
// Construct Substates
//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
CStateMonsterAttackMoveToHomePointAbstract::CStateMonsterAttackMoveToHomePoint(_Object *obj) 
                             : inherited(obj)
{
}

//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize
//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackMoveToHomePointAbstract::select_target()
{
	CMonsterSquad * const squad			=	monster_squad().get_squad(object);
	u32 const self_node					=	object->ai_location().level_vertex_id();

	if ( m_target_node != u32(-1) )
		squad->unlock_cover					(m_target_node);

	for ( u32 i=0; i<5; ++i )
	{
		m_target_node					=	object->Home->get_place_in_cover();
		if ( m_target_node != self_node )
			break;
		m_target_node					=	u32(-1);
	}

	if ( m_target_node == u32(-1) ) 
	{
		for ( u32 i=0; i<5; ++i )
		{
			m_target_node				=	object->Home->get_place();
			if ( m_target_node != self_node )
				break;
			m_target_node				=	u32(-1);
		}
	}

	m_selected_target_time				=	current_time();

	if ( m_target_node == u32(-1) )
		object->control().path_builder().get_node_in_radius(self_node, 5, 25, 10, m_target_node);

	if ( m_target_node != u32(-1) )
	{
		m_target_pos					=	ai().level_graph().vertex_position(m_target_node);
		squad->lock_cover					(m_target_node);
	}
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackMoveToHomePointAbstract::initialize()
{
	inherited::initialize					();

	m_selected_target_time				=	0;
	m_target_node						=	u32(-1);
	m_target_pos						=	object->Position();
	select_target							();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackMoveToHomePointAbstract::execute()
{
	if ( m_target_node == u32(-1) )
	{
		if ( current_time() > m_selected_target_time + 500 )
			select_target					();
	}
	else
	{
		float const dist_to_target			=	object->Position().distance_to_xz(m_target_pos);
		if ( dist_to_target < 2 )
			select_target					();
	}

	if ( m_target_node == u32(-1) )
	{
		object->set_action					(ACT_STAND_IDLE);
		object->path().set_target_point		(object->EnemyMan.get_enemy()->Position(), 
											 object->EnemyMan.get_enemy()->ai_location().level_vertex_id());
	}
	else
	{
		object->set_action					(ACT_RUN);
		object->path().set_target_point		(m_target_pos, m_target_node);
	}

	object->path().set_rebuild_time			(250);
	object->path().set_distance_to_end		(1);
	object->path().set_use_covers			();
	object->path().set_cover_params			(5.f, 30.f, 1.f, 30.f);
	object->path().set_use_dest_orient		(false);

	object->anim().accel_activate			(eAT_Aggressive);
	object->anim().accel_set_braking		(false);

	object->set_state_sound					(MonsterSound::eMonsterSoundAggressive, 
											 object->db().m_dwAttackSndDelay == u32(-1));
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackMoveToHomePointAbstract::finalize()
{
	inherited::finalize();
	clean();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackMoveToHomePointAbstract::clean()
{
	inherited::finalize();

	if ( m_target_node != u32(-1) )
	{
		CMonsterSquad * squad			=	monster_squad().get_squad(object);
		squad->unlock_cover					(m_target_node);
	}
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackMoveToHomePointAbstract::critical_finalize()
{
	inherited::critical_finalize();
	clean();
}

//////////////////////////////////////////////////////////////////////////
// Check Start Conditions / Completion
//////////////////////////////////////////////////////////////////////////


TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackMoveToHomePointAbstract::check_start_conditions()
{
	if ( !object->at_home() )
		return									true;

	if ( !object->run_home_point_when_enemy_inaccessible() )
		return									false;

	if ( !object->enemy_accessible() )
		return									true;

	return										false;
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackMoveToHomePointAbstract::check_completion()
{
	if ( !object->at_home() ) 
		return									false;

	if ( object->run_home_point_when_enemy_inaccessible() )
	{
		if ( !object->enemy_accessible() )
			return								false;
	}

	return										true;
}


#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterAttackMoveToHomePointAbstract
