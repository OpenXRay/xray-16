#pragma once

#include "controller_state_attack_hide.h"
#include "controller_state_attack_hide_lite.h"
#include "controller_state_attack_moveout.h"
#include "controller_state_attack_camp.h"
#include "controller_state_attack_fire.h"
#include "controller_tube.h"

#define CONTROL_FIRE_PERC 80
#define CONTROL_TUBE_PERC 20


#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateControllerAttackAbstract CStateControllerAttack<_Object>

TEMPLATE_SPECIALIZATION
CStateControllerAttackAbstract::CStateControllerAttack(_Object *obj) : inherited(obj)
{
	add_state(eStateAttack_MoveToHomePoint,	xr_new<CStateMonsterAttackMoveToHomePoint<CController> >(obj));	
 	add_state(eStateAttack_Run,				xr_new<CStateMonsterAttackRun<CController> >			(obj));
 	add_state(eStateAttack_Melee,			xr_new<CStateMonsterAttackMelee<CController> >			(obj));
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::initialize()
{	
	inherited::initialize				();
}

TEMPLATE_SPECIALIZATION
bool CStateControllerAttackAbstract::check_home_point()
{
	if (prev_substate != eStateAttack_MoveToHomePoint) {
		if (get_state(eStateAttack_MoveToHomePoint)->check_start_conditions())	return true;
	} else {
		if (!get_state(eStateAttack_MoveToHomePoint)->check_completion())		return true;
	}

	return false;
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::execute()
{
	object->anim().clear_override_animation	();

	if	( check_home_point() )
	{
		select_state					(eStateAttack_MoveToHomePoint);
		get_state_current()->execute	();
		prev_substate				=	current_substate;
		return;
	}

	EMonsterState		state_id	=	eStateUnknown;
	const CEntityAlive* enemy		=	object->EnemyMan.get_enemy();

	if ( current_substate == eStateAttack_Melee )
	{
		if ( get_state(eStateAttack_Melee)->check_completion() )
			state_id				=	eStateAttack_Run;
		else
			state_id				=	eStateAttack_Melee;
	}
	else
	{
		if ( get_state(eStateAttack_Melee)->check_start_conditions() )
			state_id				=	eStateAttack_Melee;
		else
			state_id				=	eStateAttack_Run;
	}

	if ( !object->enemy_accessible() && state_id == eStateAttack_Run )
	{
		current_substate			=	(u32)eStateUnknown;
		prev_substate				=	current_substate;

		Fvector dir_xz				=	object->Direction();
		dir_xz.y					=	0;
		Fvector self_to_enemy_xz	=	enemy->Position() - object->Position();
		self_to_enemy_xz.y			=	0;

		float const angle			=	angle_between_vectors(dir_xz, self_to_enemy_xz);
		
		if ( _abs(angle) > deg2rad(30.f) )
		{
			bool const rotate_right		=	object->control().direction().is_from_right(enemy->Position());
			object->anim().set_override_animation	(rotate_right ? 
													 eAnimStandTurnRight: eAnimStandTurnLeft, 0);
			object->dir().face_target		(enemy);
		}

		object->set_action				(ACT_STAND_IDLE);
		return;
	}

	select_state						(state_id);
	get_state_current()->execute		();
	prev_substate					=	current_substate;
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::setup_substates()
{
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::check_force_state() 
{
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::finalize()
{
	inherited::finalize();
	//object->set_mental_state(CController::eStateIdle);
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::critical_finalize()
{
	inherited::critical_finalize();
	//object->set_mental_state(CController::eStateIdle);
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateControllerAttackAbstract