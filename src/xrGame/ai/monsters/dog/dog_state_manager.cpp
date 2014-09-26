#include "stdafx.h"
#include "dog.h"
#include "dog_state_manager.h"
#include "../control_animation_base.h"
#include "../control_direction_base.h"
#include "../control_movement_base.h"
#include "../control_path_builder_base.h"
#include "../states/monster_state_hear_int_sound.h"
#include "../states/monster_state_hitted.h"
#include "../states/monster_state_controlled.h"
#include "../states/monster_state_help_sound.h"
#include "../group_states/group_state_attack.h"
#include "../group_states/group_state_rest.h"
#include "../group_states/group_state_eat.h"
#include "../group_states/group_state_panic.h"
#include "../group_states/group_state_hear_danger_sound.h"

namespace detail
{

namespace dog
{
	const float atack_decision_maxdist = 6.f;

} // dog

} // detail

CStateManagerDog::CStateManagerDog(CAI_Dog *monster) : inherited(monster)
{
	add_state(eStateRest,					xr_new<CStateGroupRest<CAI_Dog> >					(monster));
	add_state(eStatePanic,					xr_new<CStateGroupPanic<CAI_Dog> >					(monster));
	add_state(eStateAttack,					xr_new<CStateGroupAttack<CAI_Dog> >					(monster));
	add_state(eStateEat,					xr_new<CStateGroupEat<CAI_Dog> >					(monster));
	add_state(eStateHearInterestingSound,	xr_new<CStateMonsterHearInterestingSound<CAI_Dog> >	(monster));
	add_state(eStateHearDangerousSound,		xr_new<CStateGroupHearDangerousSound<CAI_Dog> >		(monster));
	add_state(eStateHitted,					xr_new<CStateMonsterHitted<CAI_Dog> >				(monster));
	add_state(eStateControlled,				xr_new<CStateMonsterControlled<CAI_Dog> >			(monster));
	add_state(eStateHearHelpSound,			xr_new<CStateMonsterHearHelpSound<CAI_Dog> >		(monster));
	object->EatedCorpse	= NULL;
}

void CStateManagerDog::execute()
{
	u32   state_id = u32(-1);

	CMonsterSquad* squad = monster_squad().get_squad(object);

	const CEntityAlive* enemy = object->EnemyMan.get_enemy();

	bool atack = false;
	if ( enemy )
	{
		const Fvector3& enemy_pos = enemy->Position();

		if ( squad )
		{
			if ( object->Home->at_min_home(enemy_pos) )
			{
				squad->set_home_in_danger();
			}

			if ( object->Position().distance_to(enemy_pos) < detail::dog::atack_decision_maxdist )
			{
				squad->set_home_in_danger();
			}

			if ( squad->home_in_danger() )
			{
				atack = true;
			}
		}

		if ( object->Home->at_mid_home(enemy_pos) )
		{
			atack = true;
		}
	}

	if ( !object->is_under_control() )
	{
		if ( atack )
		{
			CMonsterSquad* squad = monster_squad().get_squad(object);
			switch ( object->EnemyMan.get_danger_type() ) 
			{
				case eStrong: state_id = eStatePanic;  break;
				case eWeak:   state_id = eStateAttack; break;
			}
			if ( state_id == eStatePanic && squad->squad_alife_count() > 2 )
			{
				state_id = eStateAttack;
			}
		} 
		else if ( object->HitMemory.is_hit() )
		{
			// only inform squad of new hit (made not later then after 1 sec)
			if ( current_substate != eStateHitted && 
				 time() < object->HitMemory.get_last_hit_time()+1000 )
			{
				if ( squad )
				{
					squad->set_home_in_danger();
				}				
			}

			state_id = eStateHitted;			
		} 
		else if ( check_state(eStateHearHelpSound) )
		{
			state_id = eStateHearHelpSound;
		} 
		else if ( object->hear_interesting_sound )
		{
			state_id = eStateHearInterestingSound;
		}
		else if ( object->hear_dangerous_sound )
		{
			//comment by Lain: || monster_squad().get_squad(object)->GetCommand(object).type == SC_REST) {
			state_id = eStateHearDangerousSound;	
		} 
		else
		{
			if ( object->get_custom_anim_state() ) 
			{
				return; 
			}

			if ( check_eat() )	
			{
				state_id = eStateEat;
				if (!object->EatedCorpse)
				{
					object->EatedCorpse = object->CorpseMan.get_corpse();
					const_cast<CEntityAlive *>(object->EatedCorpse)->set_lock_corpse(true);
				}
			}
			else 
			{
				state_id = eStateRest;
			}
		}
	}
	else 
	{
		state_id = eStateControlled;
	}

	select_state(state_id); 

	if ( prev_substate != current_substate && object->get_custom_anim_state() )
	{
		object->anim_end_reinit();
	}

	if ( prev_substate == eStateEat && current_substate != eStateEat )
	{
		if ( object->character_physics_support()->movement()->PHCapture() )
		{
			object->character_physics_support()->movement()->PHReleaseObject();
		}
	}

	// выполнить текущее состояние
	get_state_current()->execute();

	prev_substate = current_substate;
}

bool CStateManagerDog::check_eat ()
{
	if ( !object->CorpseMan.get_corpse() )
	{
		if ( !object->EatedCorpse )
		{
			return false;
		}
	}

	return inherited::check_state(eStateEat);
}
