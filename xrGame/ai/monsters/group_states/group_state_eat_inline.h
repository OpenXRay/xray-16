#pragma once

#include "../states/state_data.h"
#include "../states/state_move_to_point.h"
#include "../states/state_hide_from_point.h"
#include "../states/state_custom_action.h"
#include "../../../../xrphysics/PhysicsShell.h"
#include "../../../PHMovementControl.h"
#include "../../../CharacterPhysicsSupport.h"
#include "group_state_eat_drag.h"
#include "group_state_custom.h"
#include "group_state_eat_eat.h "


#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateGroupEatAbstract CStateGroupEat<_Object>

#define TIME_NOT_HUNGRY 20000

TEMPLATE_SPECIALIZATION
CStateGroupEatAbstract::CStateGroupEat(_Object *obj) : inherited(obj)
{
	add_state	(eStateEat_CorpseApproachRun,	xr_new<CStateMonsterMoveToPoint<_Object> >	(obj));
	add_state	(eStateEat_CorpseApproachWalk,	xr_new<CStateMonsterMoveToPoint<_Object> >	(obj));
	add_state	(eStateEat_CheckCorpse,			xr_new<CStateMonsterCustomAction<_Object> >	(obj));
	add_state	(eStateEat_Eat,					xr_new<CStateGroupEating<_Object> >		(obj));
	add_state	(eStateEat_WalkAway,			xr_new<CStateMonsterHideFromPoint<_Object> >(obj));
	add_state	(eStateEat_Rest,				xr_new<CStateMonsterCustomAction<_Object> >	(obj));
	add_state	(eStateEat_Drag,				xr_new<CStateGroupDrag<_Object> >				(obj));
	add_state	(eStateCustom,					xr_new<CStateCustomGroup<_Object> >			(obj));
}

TEMPLATE_SPECIALIZATION
CStateGroupEatAbstract::~CStateGroupEat()
{
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::reinit()
{
	inherited::reinit();

	m_time_last_eat = 0;
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::initialize()
{
	inherited::initialize();
	corpse = object->EatedCorpse;
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::finalize()
{
	inherited::finalize();

	if ( (corpse==object->EatedCorpse) && object->EatedCorpse )
	{
		const_cast<CEntityAlive *>(object->EatedCorpse)->m_use_timeout = object->m_corpse_use_timeout;
		const_cast<CEntityAlive *>(object->EatedCorpse)->set_lock_corpse(false);
	}
	if (object->character_physics_support()->movement()->PHCapture())
		object->character_physics_support()->movement()->PHReleaseObject();
	object->EatedCorpse = NULL;
	object->b_end_state_eat = true;
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::critical_finalize()
{
	inherited::critical_finalize();
	if ( (corpse==object->EatedCorpse) && object->EatedCorpse && check_completion() )
	{
		if (object->character_physics_support()->movement()->PHCapture())
			object->character_physics_support()->movement()->PHReleaseObject();
		const_cast<CEntityAlive *>(object->EatedCorpse)->m_use_timeout = object->m_corpse_use_timeout;
		const_cast<CEntityAlive *>(object->EatedCorpse)->set_lock_corpse(false);
		object->EatedCorpse = NULL;
		object->b_end_state_eat = true;
	}
	if (object->EnemyMan.get_enemy())
		if (object->character_physics_support()->movement()->PHCapture())
			object->character_physics_support()->movement()->PHReleaseObject();
	object->EatedCorpse = NULL;
	object->b_end_state_eat = true;
}


TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::reselect_state()
{
	if (object->b_state_check)
	{
		select_state	(eStateCustom);
		object->b_state_check = false;
		m_time_last_eat = time() + TIME_NOT_HUNGRY;
		return;
	}

	if (object->saved_state == eStateEat_Eat)
	{
		object->saved_state = u32(-1);
		if (object->character_physics_support()->movement()->PHCapture())
			object->character_physics_support()->movement()->PHReleaseObject();
		select_state(eStateEat_Eat);					
		return;
	}

/*	
	if (prev_substate == eStateEat_CorpseApproachRun) { select_state(eStateEat_CheckCorpse); return; }

	if (prev_substate == eStateEat_CheckCorpse) { 
		if (object->ability_can_drag()) select_state(eStateEat_Drag);
		else {							
			if (get_state(eStateEat_Eat)->check_start_conditions())
				select_state(eStateEat_Eat);					
			else 
				select_state(eStateEat_CorpseApproachWalk);
		}
		return; 
	}*/

	if ( prev_substate == u32(-1) )
	{
		select_state(eStateEat_CorpseApproachWalk);
		return;
	}
	
	if ( prev_substate == eStateEat_CorpseApproachWalk )
	{
		if ( !get_state(eStateEat_CorpseApproachWalk)->check_completion() )
		{
			select_state(eStateEat_CorpseApproachWalk);
			return;
		}
		// Lain: added
		if ( object->ability_can_drag() && object->check_eated_corpse_draggable() ) 
		{
			select_state(eStateEat_Drag);
		}
		else 
		{							
			if (get_state(eStateEat_Eat)->check_start_conditions())
				select_state(eStateEat_Eat);					
			else 
				select_state(eStateEat_CorpseApproachWalk);
		}
		return; 
	}

	if ( prev_substate == eStateEat_Drag )		
	{ 
		if ( !get_state(eStateEat_Drag)->check_completion() )
		{
			select_state(eStateEat_Drag);
			return;
		}

		if ( get_state(eStateEat_Eat)->check_start_conditions() ) 
		{
			object->set_current_animation(15);
			object->saved_state = eStateEat_Eat;
			select_state(eStateCustom);					
			object->b_state_check = false;
		}
		else 
		{
			select_state(eStateEat_CorpseApproachWalk);
		}
		return; 
	}

	if ( prev_substate == eStateEat_Eat )
	{
		m_time_last_eat = time();

		if (!hungry()) 
			select_state(eStateEat_WalkAway); 
		else 
			select_state(eStateEat_CorpseApproachWalk);
		return;
	}

	if ( prev_substate == eStateEat_WalkAway )
	{ 
		select_state(eStateEat_Rest);		
		return; 
	}

	if ( prev_substate == eStateEat_Rest )		
	{ 
		select_state(eStateEat_Rest);		
		return; 
	}

	select_state(eStateEat_Rest);
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::setup_substates()
{
	state_ptr state = get_state_current();

	if (current_substate == eStateEat_CorpseApproachRun) {

		// Определить позицию ближайшей боны у трупа
		Fvector nearest_bone_pos;
		const CEntityAlive *corpse = object->EatedCorpse;
		if ((corpse->m_pPhysicsShell == NULL) || (!corpse->m_pPhysicsShell->isActive())) {
			nearest_bone_pos	= corpse->Position(); 
		} else nearest_bone_pos = object->character_physics_support()->movement()->PHCaptureGetNearestElemPos(corpse);

		SStateDataMoveToPoint data;
		data.point			= nearest_bone_pos;
		data.vertex			= u32(-1);
		data.action.action	= ACT_RUN;
		data.accelerated	= true;
		data.braking		= true;
		data.accel_type 	= eAT_Calm;
		data.completion_dist= object->db().m_fDistToCorpse;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPoint));
		return;
	}

	if (current_substate == eStateEat_CheckCorpse) {
		SStateDataAction data;
		data.action			= ACT_STAND_IDLE;
		data.spec_params	= 0;
		data.time_out		= 500;
		data.sound_type	= MonsterSound::eMonsterSoundEat;
		data.sound_delay = object->db().m_dwEatSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataAction));

		return;
	}

	if (current_substate == eStateEat_WalkAway) {
		SStateHideFromPoint data;

		data.point					= object->EatedCorpse->Position();
		data.action.action			= ACT_WALK_FWD;
		data.distance				= 15.f;	
		data.accelerated			= true;
		data.braking				= true;
		data.accel_type				= eAT_Calm;
		data.cover_min_dist			= 20.f;
		data.cover_max_dist			= 30.f;
		data.cover_search_radius	= 25.f;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateHideFromPoint));

		return;
	}

	if (current_substate == eStateEat_Rest) {
		SStateDataAction data;
		data.action			= ACT_STAND_IDLE;
		data.spec_params	= 0;
		data.time_out		= 500;
		data.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataAction));

		return;
	}

	if (current_substate == eStateEat_CorpseApproachWalk) {

		// Определить позицию ближайшей боны у трупа
		Fvector nearest_bone_pos;
		const CEntityAlive *corpse = object->EatedCorpse;

		#ifdef DEBUG
			if ( !corpse )
			{
				debug::text_tree tree;
				object->add_debug_info(tree);
				debug::log_text_tree(tree);
				FATAL("Debug info has been added, plz save log");
			}
		#endif //#ifdef DEBUG

		if ( (corpse->m_pPhysicsShell == NULL) || (!corpse->m_pPhysicsShell->isActive()) ) {
			nearest_bone_pos	= corpse->Position();
		} else nearest_bone_pos = object->character_physics_support()->movement()->PHCaptureGetNearestElemPos(corpse);

		SStateDataMoveToPoint data;
		data.point			= nearest_bone_pos;
		data.vertex			= u32(-1);
		data.action.action	= ACT_WALK_FWD;
		data.accelerated	= true;
		data.braking		= true;
		data.accel_type 	= eAT_Calm;
		data.completion_dist= object->db().m_fDistToCorpse;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPoint));
		return;
	}
}

TEMPLATE_SPECIALIZATION
bool CStateGroupEatAbstract::check_completion()
{
	if (corpse != object->EatedCorpse) return true;
	if (!hungry()) return true;

	return false;
}

TEMPLATE_SPECIALIZATION
bool CStateGroupEatAbstract::check_start_conditions()
{
	if (object->EatedCorpse) return true;
	return (
		object->CorpseMan.get_corpse() && 
		object->Home->at_home( object->CorpseMan.get_corpse()->Position()) &&
		hungry() &&
		!const_cast<CEntityAlive *>(object->CorpseMan.get_corpse())->is_locked_corpse()
		);

}

TEMPLATE_SPECIALIZATION
bool CStateGroupEatAbstract::hungry()
{
	return ((m_time_last_eat == 0) || (m_time_last_eat + TIME_NOT_HUNGRY < time()));
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::remove_links	(CObject* object)
{
	if (corpse == object)
		corpse	= 0;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupEatAbstract