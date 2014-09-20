#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterAttackCampStealOutAbstract CStateMonsterAttackCampStealOut<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterAttackCampStealOutAbstract::CStateMonsterAttackCampStealOut(_Object *obj) : inherited(obj) 
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackCampStealOutAbstract::execute()
{
	if (object->EnemyMan.get_my_vertex_enemy_last_seen() == u32(-1)) return;
	
	object->path().set_target_point			(object->EnemyMan.get_my_vertex_enemy_last_seen());	
	object->path().set_rebuild_time			(0);
	object->path().set_distance_to_end		(0.f);
	object->path().set_use_covers			(false);
	
	object->set_action						(ACT_STEAL);
	object->anim().accel_deactivate			();
	object->anim().accel_set_braking		(false);
	object->set_state_sound					(MonsterSound::eMonsterSoundSteal);
}

#define STATE_EXECUTE_TIME 8000


TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackCampStealOutAbstract::check_completion()
{
	if (object->EnemyMan.get_my_vertex_enemy_last_seen() == u32(-1)) return true;
	if (object->EnemyMan.see_enemy_now()) return true;
	if (object->HitMemory.get_last_hit_time() > time_state_started) return true;
	if (time_state_started + STATE_EXECUTE_TIME < time()) return true;
	
	Fvector pos = ai().level_graph().vertex_position(object->EnemyMan.get_my_vertex_enemy_last_seen());
	if ((object->Position().distance_to(pos) < 2.f) && object->control().path_builder().is_path_end(0.f)) return true;

	return false;
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackCampStealOutAbstract::check_start_conditions()
{
	if (object->EnemyMan.get_my_vertex_enemy_last_seen() == u32(-1)) return false;
	if (object->EnemyMan.see_enemy_now()) return false;
	return true;	
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterAttackCampStealOutAbstract

