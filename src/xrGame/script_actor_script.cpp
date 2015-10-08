////////////////////////////////////////////////////////////////////////////
//	Module 		: script_actor.cpp
//	Created 	: 12.08.2014
//  Modified 	: 12.08.2014
//	Author		: Alexander Petrov
//	Description : Script Actor (params)
////////////////////////////////////////////////////////////////////////////
//#include "stdafx.h"
#include "pch_script.h"
#include "script_game_object.h"
//#include "CharacterPhysicsSupport.h"
#include "script_actor_script.h"
//#include "PHSimpleCharacter.h"
//#include "Inventory.h"
//#include "Wound.h"

//#include "HUDManager.h"
//#include "ui/UIDialogWnd.h"
//#include "ui/UIInventoryWnd.h"


using namespace luabind;

//CPHMovementControl *get_movement(CActor *pActor)
//{
//	return pActor->character_physics_support()->movement();
//}

#pragma optimize("s",on)

// IC float CScriptActor::get_burn_immunity(CActorCondition *C) { }

//float get_jump_up_velocity(CPHMovementControl *M)
//{
//	CPHSimpleCharacter *sp = smart_cast <CPHSimpleCharacter *> (M->PHCharacter());
//	if (sp) return sp->get_jump_up_velocity();
//	return 0;
//}

//float	get_jump_speed(CActor *pActor) { return CScriptActor::jump_speed(pActor); }
//void	set_jump_speed(CActor *pActor, float speed)
//{
//	CScriptActor::jump_speed(pActor) = speed;
//	get_movement(pActor)->SetJumpUpVelocity(speed);
//}

void CScriptActor::script_register(lua_State *L)
{
	module(L)
		[
			class_ <CActorCondition>("CActorCondition")
			// .def_readwrite("immunities",				&CActorCondition::m_HitTypeK)

			.def_readwrite("alcohol_health",			&CActorCondition::m_fAlcohol)
			.def_readwrite("alcohol_v",					&CActorCondition::m_fV_Alcohol)
			.def_readwrite("power",					    &CActorCondition::m_fPower)
			.def_readwrite("power_max",					&CActorCondition::m_fPowerMax)
			.def_readwrite("psy_health",				&CActorCondition::m_fPsyHealth)
			.def_readwrite("psy_health_max",			&CActorCondition::m_fPsyHealthMax)
			.def_readwrite("satiety",					&CActorCondition::m_fSatiety)
			.def_readwrite("satiety_v",					&CActorCondition::m_fV_Satiety)
			.def_readwrite("satiety_health_v",			&CActorCondition::m_fV_SatietyHealth)
			.def_readwrite("thirst",					&CActorCondition::m_fThirst)
			.def_readwrite("thirst_v",					&CActorCondition::m_fV_Thirst)
			.def_readwrite("thirst_health_v",			&CActorCondition::m_fV_ThirstHealth)      
			
			.def_readwrite("radiation",					&CActorCondition::m_fRadiation)
			.def_readwrite("radiation_max",				&CActorCondition::m_fRadiationMax)
			.def_readwrite("morale",					&CActorCondition::m_fEntityMorale)																
			.def_readwrite("morale_max",				&CActorCondition::m_fEntityMoraleMax)			
			.def_readwrite("min_wound_size",			&CActorCondition::m_fMinWoundSize)
			.def_readonly("is_bleeding",				&CActorCondition::m_bIsBleeding)
			.def_readwrite("health_hit_part",			&CActorCondition::m_fHealthHitPart)
			.def_readwrite("power_hit_part",			&CActorCondition::m_fPowerHitPart)			
			.def_readwrite("max_power_leak_speed",		&CActorCondition::m_fPowerLeakSpeed)			
			.def_readwrite("jump_power",				&CActorCondition::m_fJumpPower)
			.def_readwrite("stand_power",				&CActorCondition::m_fStandPower)
			.def_readwrite("walk_power",				&CActorCondition::m_fWalkPower)
			.def_readwrite("jump_weight_power",			&CActorCondition::m_fJumpWeightPower)
			.def_readwrite("walk_weight_power",			&CActorCondition::m_fWalkWeightPower)
			.def_readwrite("overweight_walk_k",			&CActorCondition::m_fOverweightWalkK)
			.def_readwrite("overweight_jump_k",			&CActorCondition::m_fOverweightJumpK)
			.def_readwrite("accel_k",					&CActorCondition::m_fAccelK)
			.def_readwrite("sprint_k",					&CActorCondition::m_fSprintK)
			.def_readwrite("max_walk_weight",			&CActorCondition::m_MaxWalkWeight)

			.def_readwrite("limping_power_begin",		&CActorCondition::m_fLimpingPowerBegin)
			.def_readwrite("limping_power_end",			&CActorCondition::m_fLimpingPowerEnd)
			.def_readwrite("cant_walk_power_begin",		&CActorCondition::m_fCantWalkPowerBegin)
			.def_readwrite("cant_walk_power_end",		&CActorCondition::m_fCantWalkPowerEnd)
			.def_readwrite("cant_spint_power_begin",	&CActorCondition::m_fCantSprintPowerBegin)
			.def_readwrite("cant_spint_power_end",		&CActorCondition::m_fCantSprintPowerEnd)
			.def_readwrite("limping_health_begin",		&CActorCondition::m_fLimpingHealthBegin)
			.def_readwrite("limping_health_end",		&CActorCondition::m_fLimpingHealthEnd)
			// .def_readwrite("", &CActorCondition::)
			.def_readonly("limping",					&CActorCondition::m_bLimping)
			.def_readonly("cant_walk",					&CActorCondition::m_bCantWalk)
			.def_readonly("cant_sprint",				&CActorCondition::m_bCantSprint)
			//,
			//class_<CPHMovementControl>("CPHMovementControl")
			//.def_readwrite("ph_mass", &CPHMovementControl::fMass)
			//.def_readwrite("crash_speed_max",			&CPHMovementControl::fMaxCrashSpeed)
			//.def_readwrite("crash_speed_min",			&CPHMovementControl::fMinCrashSpeed)
			//.def_readwrite("collision_damage_factor",	&CPHMovementControl::fCollisionDamageFactor)
			//.def_readwrite("air_control_param",			&CPHMovementControl::fAirControlParam)			
			//.property("jump_up_velocity",				&get_jump_up_velocity,				    &CPHMovementControl::SetJumpUpVelocity)

			//,
			//class_<CActor>("CActor")
			
			//.def_readonly("condition",					&CActor::m_entity_condition)
			//.def_readwrite("hit_slowmo",				&CActor::m_hit_slowmo)
			//.def_readwrite("hit_probability",			&CActor::m_hit_probability)	
			//.def_readwrite("walk_accel",				&CActor::m_fWalkAccel)
			
			//.def_readwrite("run_factor",				&CActor::m_fRunFactor)
			//.def_readwrite("run_back_factor",			&CActor::m_fRunBackFactor)
			//.def_readwrite("walk_back_factor",			&CActor::m_fWalkBackFactor)
			//.def_readwrite("crouch_coef",				&CActor::m_fCrouchFactor)
			//.def_readwrite("climb_coef",				&CActor::m_fClimbFactor)
			//.def_readwrite("sprint_koef",				&CActor::m_fSprintFactor)
			//.def_readwrite("walk_strafe_coef",			&CActor::m_fWalk_StrafeFactor)
			//.def_readwrite("run_strafe_coef",			&CActor::m_fRun_StrafeFactor)
			//.def_readwrite("disp_base",					&CActor::m_fDispBase)
			//.def_readwrite("disp_aim",					&CActor::m_fDispAim)
			//.def_readwrite("disp_vel_factor",			&CActor::m_fDispVelFactor)
			//.def_readwrite("disp_accel_factor",			&CActor::m_fDispAccelFactor)
			//.def_readwrite("disp_crouch_factor",		&CActor::m_fDispCrouchFactor)
			//.def_readwrite("disp_crouch_no_acc_factor", &CActor::m_fDispCrouchNoAccelFactor)
			//.def_readwrite("disp_jump_factor",			&CActor::m_vMissileOffset)		

			//.property("movement",						&get_movement)
			//.property("jump_speed",						&get_jump_speed, &set_jump_speed)


		];
}