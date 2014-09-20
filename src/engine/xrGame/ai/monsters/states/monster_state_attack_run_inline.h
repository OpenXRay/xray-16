#pragma once

#include "../ai_monster_squad.h"
#include "../ai_monster_squad_manager.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterAttackRunAbstract CStateMonsterAttackRun<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAbstract::initialize()
{
	inherited::initialize();
	object->path().prepare_builder	();	
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAbstract::execute()
{
	// установка параметров функциональных блоков
	object->anim().accel_activate			(eAT_Aggressive);
	object->anim().accel_set_braking		(false);
	
	u32 const level_vertex				=	object->EnemyMan.get_enemy()->ai_location().level_vertex_id();
	Fvector const level_pos				=	ai().level_graph().vertex_position(level_vertex);
	object->path().set_target_point			(level_pos, level_vertex);

	if ( level_vertex == object->ai_location().level_vertex_id() )
		object->set_action					(ACT_STAND_IDLE);
	else
		object->set_action					(ACT_RUN);

	object->path().set_rebuild_time			(object->get_attack_rebuild_time());
	object->path().set_use_covers			();
	object->path().set_cover_params			(0.1f, 30.f, 1.f, 30.f);
	object->path().set_try_min_time			(false);
	object->set_state_sound					(MonsterSound::eMonsterSoundAggressive);
	object->path().extrapolate_path			(true);
	
	// обработать squad инфо	
	object->path().set_use_dest_orient		(false);

	CMonsterSquad *squad	= monster_squad().get_squad(object);
	if (squad && squad->SquadActive()) {
		// Получить команду
		SSquadCommand command;
		squad->GetCommand(object, command);
		
		if (command.type == SC_ATTACK) {
			object->path().set_use_dest_orient	(true);
			object->path().set_dest_direction	(command.direction);
		} 
	}
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAbstract::finalize()
{
	inherited::finalize					();
	object->path().extrapolate_path	(false);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAbstract::critical_finalize()
{
	inherited::critical_finalize		();
	object->path().extrapolate_path	(false);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackRunAbstract::check_completion()
{
	float m_fDistMin	= object->MeleeChecker.get_min_distance		();
	float dist			= object->MeleeChecker.distance_to_enemy	(object->EnemyMan.get_enemy());

	if (dist < m_fDistMin)	
		return true;

	return false;
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackRunAbstract::check_start_conditions()
{
	float m_fDistMax	= object->MeleeChecker.get_max_distance		();
	float dist			= object->MeleeChecker.distance_to_enemy	(object->EnemyMan.get_enemy());

	if (dist > m_fDistMax)	return true;

	return false;
}

