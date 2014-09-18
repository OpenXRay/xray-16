#include "pch_script.h"
#include "helicopter.h"
#include "script_game_object.h"

int CHelicopter::GetMovementState()
{
	return m_movement.type;
}

int CHelicopter::GetHuntState()
{
	return m_enemy.type;
}

int CHelicopter::GetBodyState()
{
	return	m_body.type;
}

using namespace luabind;

#pragma optimize("s",on)
void CHelicopter::script_register(lua_State *L)
{
	module(L)
	[
		class_<CHelicopter,CGameObject>("CHelicopter")
			.def(constructor<>())
			.enum_("state")
				[
					value("eAlive",									int(CHelicopter::eAlive)),
					value("eDead",									int(CHelicopter::eDead))
				]
			.enum_("movement_state")
				[
					value("eMovNone",								int(eMovNone)),
					value("eMovToPoint",							int(eMovToPoint)),
					value("eMovPatrolPath",							int(eMovPatrolPath)),
					value("eMovRoundPath",							int(eMovRoundPath)),
					value("eMovLanding",							int(eMovLanding)),
					value("eMovTakeOff",							int(eMovTakeOff))
				]
			.enum_("hunt_state")
				[
					value("eEnemyNone",								int(eEnemyNone)),
					value("eEnemyPoint",							int(eEnemyPoint)),
					value("eEnemyEntity",							int(eEnemyEntity))
				]
			.enum_("body_state")
				[
					value("eBodyByPath",							int(eBodyByPath)),
					value("eBodyToPoint",							int(eBodyToPoint))
				]

				.def("GetState",							&CHelicopter::state_script)
				.def("GetMovementState",					&CHelicopter::GetMovementState)
				.def("GetHuntState",						&CHelicopter::GetHuntState)
				.def("GetBodyState",						&CHelicopter::GetBodyState)

				.def("GetCurrVelocity",						&CHelicopter::GetCurrVelocity)
				.def("GetMaxVelocity",						&CHelicopter::GetMaxVelocity)
				.def("SetMaxVelocity",						&CHelicopter::SetMaxVelocity)
				.def("GetCurrVelocityVec",					&CHelicopter::GetCurrVelocityVec)
				.def("GetfHealth",							&CHelicopter::GetHeliHealth)
				.def("SetfHealth",							&CHelicopter::SetHeliHealth)

				.def("SetSpeedInDestPoint",					&CHelicopter::SetSpeedInDestPoint)
				.def("GetSpeedInDestPoint",					&CHelicopter::GetSpeedInDestPoint)
				//////////////////////Start By JoHnY///////////////////////
				.def("SetLinearAcc",						&CHelicopter::SetLinearAcc)
				//////////////////////End By JoHnY/////////////////////////

				.def("SetOnPointRangeDist",					&CHelicopter::SetOnPointRangeDist)
				.def("GetOnPointRangeDist",					&CHelicopter::GetOnPointRangeDist)
				
				.def("GetDistanceToDestPosition",			&CHelicopter::GetDistanceToDestPosition)

				.def("ClearEnemy",							&CHelicopter::UnSetEnemy)
				.def("SetEnemy",							(void (CHelicopter::*)(CScriptGameObject*)) &CHelicopter::SetEnemy)
				.def("SetEnemy",							(void (CHelicopter::*)(Fvector*)) &CHelicopter::SetEnemy)
				.def("GoPatrolByPatrolPath",				&CHelicopter::goPatrolByPatrolPath)
				.def("GoPatrolByRoundPath",					&CHelicopter::goByRoundPath)
				.def("SetDestPosition",						&CHelicopter::SetDestPosition)
				.def("LookAtPoint",							&CHelicopter::LookAtPoint)
				.def("SetFireTrailLength",					&CHelicopter::SetFireTrailLength)
				.def("SetBarrelDirTolerance",				&CHelicopter::SetBarrelDirTolerance)

				.def("UseFireTrail",						(bool (CHelicopter::*)(void)) &CHelicopter::UseFireTrail)
				.def("UseFireTrail",						(void (CHelicopter::*)(bool)) &CHelicopter::UseFireTrail)

				.def("Die",									&CHelicopter::DieHelicopter)
				.def("StartFlame",							&CHelicopter::StartFlame)
				.def("Explode",								&CHelicopter::ExplodeHelicopter)

				.def("isVisible",							&CHelicopter::isVisible)

				.def("GetRealAltitude",						&CHelicopter::GetRealAltitude)
				.def("GetSafeAltitude",						&CHelicopter::GetSafeAltitude)
				.def("TurnLighting",						&CHelicopter::TurnLighting)
				.def("TurnEngineSound",						&CHelicopter::TurnEngineSound)
				.def_readwrite("m_use_rocket_on_attack",	&CHelicopter::m_use_rocket_on_attack)
				.def_readwrite("m_use_mgun_on_attack",		&CHelicopter::m_use_mgun_on_attack)
				.def_readwrite("m_min_rocket_dist",			&CHelicopter::m_min_rocket_dist)
				.def_readwrite("m_max_rocket_dist",			&CHelicopter::m_max_rocket_dist)
				.def_readwrite("m_min_mgun_dist",			&CHelicopter::m_min_mgun_dist)
				.def_readwrite("m_max_mgun_dist",			&CHelicopter::m_max_mgun_dist)
				.def_readwrite("m_time_between_rocket_attack", &CHelicopter::m_time_between_rocket_attack)
				.def_readwrite("m_syncronize_rocket",		&CHelicopter::m_syncronize_rocket)
				.def_readonly ("m_flame_started",			&CHelicopter::m_flame_started)
				.def_readonly ("m_light_started",			&CHelicopter::m_light_started)
				.def_readonly ("m_exploded",				&CHelicopter::m_exploded)
				.def_readonly ("m_dead",					&CHelicopter::m_dead)

//				.def_readwrite("", &CHelicopter::)

		];
}