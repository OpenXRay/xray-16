#pragma once

#include "../states/monster_state_attack_melee.h"
#include "../states/monster_state_attack_run_attack.h"
#include "../states/state_hide_from_point.h"
#include "../states/monster_state_find_enemy.h"
#include "group_state_squad_move_to_radius.h "
#include "group_state_home_point_attack.h"
#include "group_state_custom.h"
#include "../ai_monster_squad.h"
#include "../ai_monster_squad_manager.h"
#include "group_state_attack_run.h"
#include "../../../entity_alive.h"
#include "../../../actor.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateGroupAttackAbstract CStateGroupAttack<_Object>

TEMPLATE_SPECIALIZATION
CStateGroupAttackAbstract::CStateGroupAttack(_Object *obj) : inherited(obj)
{
	add_state	(eStateAttack_Run,				xr_new<CStateGroupAttackRun<_Object> >				(obj));
	add_state	(eStateAttack_Melee,			xr_new<CStateMonsterAttackMelee<_Object> >			(obj));
	add_state	(eStateAttack_RunAttack,		xr_new<CStateMonsterAttackRunAttack<_Object> >		(obj));
	add_state	(eStateAttack_Attack_On_Run,	xr_new<CStateMonsterAttackOnRun<_Object> >			(obj));
	add_state	(eStateAttack_RunAway,			xr_new<CStateMonsterHideFromPoint<_Object> >		(obj));
	add_state	(eStateAttack_FindEnemy,		xr_new<CStateMonsterFindEnemy<_Object> >			(obj));	
	add_state	(eStateAttack_MoveToHomePoint,	xr_new<CStateMonsterAttackMoveToHomePoint<_Object> >(obj));	
	
	add_state	(eStateCustom,					xr_new<CStateGroupSquadMoveToRadius<_Object> >		(obj));
	add_state	(eStateAttack_AttackHidden,		xr_new<CStateGroupSquadMoveToRadius<_Object> >		(obj));
	add_state	(eStateAttackCamp,				xr_new<CStateGroupSquadMoveToRadius<_Object> >		(obj));
	add_state	(eStateAttack_Steal,			xr_new<CStateGroupSquadMoveToRadiusEx<_Object> >		(obj));
	add_state	(eStateAttack_ControlFire,		xr_new<CStateCustomGroup<_Object> >					(obj));
}

TEMPLATE_SPECIALIZATION
CStateGroupAttackAbstract::~CStateGroupAttack()
{
}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackAbstract::initialize()
{
	inherited::initialize				();

	object->MeleeChecker.init_attack	();
	m_drive_out = false;

	m_enemy = object->EnemyMan.get_enemy();

	CMonsterSquad* squad = monster_squad().get_squad(object);

	if ( squad ) 
	{
		SMemberGoal	goal;

		goal.type	= MG_AttackEnemy;
		goal.entity	= const_cast<CEntityAlive*>(m_enemy);

		if ( squad->get_index(object) == u8(-1) )
		{
			squad->SetLeader(object);
			monster_squad().get_squad(object)->set_squad_index(object->EnemyMan.get_enemy());
		}

		squad->UpdateGoal(object, goal);
		squad->UpdateSquadCommands();
	}

	//object->EnemyMan.update(); - may cause enemy returned by EnemyMan be invalid next execute

	m_time_next_run_away		= 0;
	m_time_start_check_behinder	= 0;
	m_time_start_behinder		= 0;
	m_delta_distance			= Random.randF(0, 3);
}

#define FIND_ENEMY_DELAY	12000

TEMPLATE_SPECIALIZATION
void CStateGroupAttackAbstract::finalize()
{
	inherited::finalize();
	CEntityAlive* enemy	= const_cast<CEntityAlive*>(m_enemy);
	enemy->is_agresive(false);
	object->EnemyMan.script_enemy();
}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackAbstract::critical_finalize()
{
	inherited::critical_finalize();

	if ( m_enemy && !m_enemy->g_Alive() )
	{
		CEntityAlive* enemy	= const_cast<CEntityAlive*>(m_enemy);
		enemy->is_agresive(false);
		enemy->is_start_attack(false);
	}
	object->EnemyMan.script_enemy();
}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackAbstract::execute()
{
	bool	can_attack_on_move	=	object->can_attack_on_move();
	CEntityAlive* enemy	= const_cast<CEntityAlive*>(object->EnemyMan.get_enemy());

	bool const enemy_is_actor	=	!!smart_cast<CActor*>(enemy);

	const Fvector3 enemy_pos = enemy->Position();

	const bool enemy_at_max_home = object->Home->at_home(enemy_pos);
	const bool enemy_at_mid_home = object->Home->at_mid_home(enemy_pos);

	if ( enemy == object->EnemyMan.get_script_enemy() )
	{
		object->EnemyMan.add_enemy(enemy);
		object->EnemyMan.script_enemy();

		if ( !object->EnemyMan.get_enemy() ) 
		{
			object->SetEnemy(enemy);
		}
	}

	bool aggressive = false;

	if ( object->Home->is_aggressive() ) 
	{
		aggressive = true;
	}
	else
	{
		if ( !enemy_is_actor )
		{
			aggressive	=	true;
		}

		if ( enemy_at_mid_home ) 
		{
			if ( object->hear_dangerous_sound )
			{
				aggressive = true;
			}
		}

		if ( enemy_at_max_home ) 
		{
			if ( object->Position().distance_to(enemy_pos) <= 6.f )
			{
				aggressive = true;
			}			
		}
		else
		{
			aggressive = false;
		}
	}

	CMonsterSquad* squad = monster_squad().get_squad(object);

	// now, if found reasons to be aggressive, mark home to be in danger
	if ( aggressive )
	{
		if ( squad )
		{
			squad->set_home_in_danger();
		}
	}

	// if home is still agressive, we're agressive untill it lasts
	if ( squad && squad->home_in_danger() )
	{
		aggressive = true;
	}

	if ( check_home_point() )
	{
		if ( prev_substate == eStateAttack_MoveToHomePoint ) 
		{
			if ( get_state_current()->check_completion() )
			{
				select_state(eStateAttack_FindEnemy);
			}
		}
		else 
		{
			select_state(eStateAttack_MoveToHomePoint);
		}
	} 
	else
	{

		// определить тип атаки
		bool b_melee = false; 

		if ( prev_substate == eStateAttack_Melee )
		{
			if ( !get_state_current()->check_completion() ) 
			{
				b_melee = true;
			}
		} 
		else if ( get_state(eStateAttack_Melee)->check_start_conditions() )
		{
			b_melee = true;
		}

		// установить целевое состояние
		if ( !can_attack_on_move && b_melee )
		{  
			// check if enemy is behind me for a long time
			// [TODO] make specific state and replace run_away state (to avoid rotation jumps)
			if ( check_behinder() )
			{
				select_state(eStateAttack_RunAway);
			}
			else
			{
				select_state(eStateAttack_Melee);
			}
		}
		else 
		{
			if ( aggressive )
			{
				if ( object->get_custom_anim_state() )
				{
					object->anim_end_reinit();
				}
			
				select_state(can_attack_on_move ? eStateAttack_Attack_On_Run : eStateAttack_Run);
			} 
			else
			{
				switch( prev_substate )
				{
					case eStateAttack_Steal:
						if (get_state_current()->check_completion()) 
						{
							select_state(eStateAttack_AttackHidden);
						}
						break;

					case eStateAttack_AttackHidden:
						if (get_state_current()->check_completion())
						{
							select_state(eStateCustom);
						} 
						else 
						{
							if (object->Position().distance_to(enemy->Position()) > 17.f + m_delta_distance)
							{
								select_state(eStateAttack_Steal);
							}
						}
						break;

					case eStateCustom:
						if ( get_state_current()->check_completion() ) 
						{
							if ( object->get_custom_anim_state() ) 
							{
								return;
							}

							object->set_current_animation(6);
							object->b_state_check = false;
							m_time_start_drive_out = Device.dwTimeGlobal;
							select_state(eStateAttack_ControlFire);
						} 
						else 
						{
							if ( object->Position().distance_to(enemy_pos) > 11.f + m_delta_distance ) 
							{
								select_state(eStateAttack_AttackHidden);
								m_drive_out = false;
							}
						}
						break;

					case eStateAttack_ControlFire:
						if ( object->Position().distance_to(enemy_pos) > 7.f + m_delta_distance || Device.dwTimeGlobal - m_time_start_drive_out > object->m_drive_out_time )
						{
							if ( object->get_custom_anim_state() ) 
							{
								object->anim_end_reinit();
							}

							if ( Device.dwTimeGlobal - m_time_start_drive_out > object->m_drive_out_time ) 
							{
								m_drive_out = true;
							}

							select_state(eStateCustom);
						} 
						else 
						{
							if ( object->get_custom_anim_state() ) 
							{
								return;
							}

							object->set_current_animation(6);
							object->b_state_check = false;
							select_state(eStateAttack_ControlFire);
						}
						break;

					default:
						select_state(eStateAttack_Steal);
						break;
				}
			}
		}
	}

	// clear behinder var if not melee state selected
	if ( current_substate != eStateAttack_Melee )
	{
		m_time_start_check_behinder = 0;
	}

	get_state_current()->execute();

	prev_substate = current_substate;

	// Notify squad	
	if ( squad ) 
	{
		SMemberGoal	goal;

		goal.type	= MG_AttackEnemy;
		goal.entity	= const_cast<CEntityAlive*>(object->EnemyMan.get_enemy());

		squad->UpdateGoal(object, goal);
	}
}

TEMPLATE_SPECIALIZATION
bool CStateGroupAttackAbstract::check_home_point()
{
	// Lain: intentionally
	// 	if ( object->Home->is_aggressive() )
	// 	{
	// 		return false;
	// 	}

	if ( prev_substate != eStateAttack_MoveToHomePoint ) 
	{
		if ( get_state(eStateAttack_MoveToHomePoint)->check_start_conditions() )
		{
			return true;
		}
	} 
	else 
	{
		if ( !get_state(eStateAttack_MoveToHomePoint)->check_completion() )
		{
			return true;
		}
	}

	return false;
}

TEMPLATE_SPECIALIZATION
void   CStateGroupAttackAbstract::setup_substates ()
{
	state_ptr state = get_state_current();

	if ( current_substate == eStateAttack_Steal ) 
	{
		SStateDataMoveToPointEx data;

		data.vertex				= 0;
		data.point				= object->EnemyMan.get_enemy()->Position();
		data.action.action		= ACT_RUN;
		data.action.time_out	= 0;		// do not use time out
		data.completion_dist	= 15.f + m_delta_distance;		// get exactly to the point
		data.time_to_rebuild	= object->get_attack_rebuild_time();		
		data.accelerated		= true;
		data.braking			= false;
		data.accel_type 		= eAT_Aggressive;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));

		return;
	}

	if ( current_substate == eStateAttack_AttackHidden )
	{
		SStateDataMoveToPointEx data;

		data.vertex				= 0;
		data.point				= object->EnemyMan.get_enemy()->Position();
		data.action.action		= ACT_WALK_FWD;
		data.action.time_out	= 0;		// do not use time out
		data.completion_dist	= 10.f + m_delta_distance;		// get exactly to the point
		data.time_to_rebuild	= object->get_attack_rebuild_time();		
		data.accelerated		= true;
		data.braking			= false;
		data.accel_type 		= eAT_Aggressive;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));

		return;
	}

	if ( current_substate == eStateCustom ) 
	{
		SStateDataMoveToPointEx data;

		data.vertex				= 0;
		data.point				= object->EnemyMan.get_enemy()->Position();
		data.action.action		= ACT_HOME_WALK_GROWL;
		data.action.time_out	= 0;		// do not use time out
		
		if ( m_drive_out )
		{
			data.completion_dist	= 1.f;		// get exactly to the point
		} 
		else 
		{
			data.completion_dist	= 6.f + m_delta_distance;		// get exactly to the point
		}
		data.time_to_rebuild	= object->get_attack_rebuild_time();		
		data.accelerated		= true;
		data.braking			= false;
		data.accel_type 		= eAT_Aggressive;
		data.action.sound_type	= MonsterSound::eMonsterSoundThreaten;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));

		return;
	}

	if ( current_substate == eStateAttack_RunAway )
	{
		SStateHideFromPoint		data;
		data.point				= object->EnemyMan.get_enemy()->Position();
		data.accelerated		= true;
		data.braking			= false;
		data.accel_type			= eAT_Aggressive;
		data.distance			= 20.f;
		data.action.action		= ACT_RUN;
		data.action.sound_type	= MonsterSound::eMonsterSoundAggressive;
		data.action.sound_delay = object->db().m_dwAttackSndDelay;
		data.action.time_out	= 5000;

		state->fill_data_with(&data, sizeof(SStateHideFromPoint));

		return;
	}
}

#define TIME_CHECK_BEHINDER 2000
#define TIME_IN_BEHINDER	3000
#define ANGLE_START_CHECK_BEHINDER		2 * PI_DIV_3
#define ANGLE_CONTINUE_CHECK_BEHINDER	PI_DIV_2

TEMPLATE_SPECIALIZATION
bool CStateGroupAttackAbstract::check_behinder()
{
	// if we are not in behinder state
	if (m_time_start_behinder == 0) {
		// check if we can start behinder


		// - check if we start checking 
		if (m_time_start_check_behinder == 0) {

			// - check if object is behind
			if (!object->control().direction().is_face_target(object->EnemyMan.get_enemy(), ANGLE_START_CHECK_BEHINDER)) {
				m_time_start_check_behinder = time();
			}

		} else {
			// if we already in check mode

			// - check if object is not behind (break checker)
			if (object->control().direction().is_face_target(object->EnemyMan.get_enemy(), ANGLE_CONTINUE_CHECK_BEHINDER)) {
				m_time_start_check_behinder = 0;
			} 

			// check if time is not out
			if (m_time_start_check_behinder + TIME_CHECK_BEHINDER > time())	return false;

			m_time_start_behinder		= time();
			m_time_start_check_behinder	= 0;
		}
	} 

	// if we are not in behinder state

	if (m_time_start_behinder != 0) {
		if (m_time_start_behinder + TIME_IN_BEHINDER > time()) return true;	
		else m_time_start_behinder = 0;
	}

	return false;
}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackAbstract::remove_links(CObject* object)
{
	if ( m_enemy == object )
	{
		m_enemy	= 0;
	}
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupAttackAbstract