#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterFindEnemyRunAbstract CStateMonsterFindEnemyRun<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterFindEnemyRunAbstract::CStateMonsterFindEnemyRun(_Object *obj) : inherited(obj)
{
}

TEMPLATE_SPECIALIZATION
CStateMonsterFindEnemyRunAbstract::~CStateMonsterFindEnemyRun()
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterFindEnemyRunAbstract::initialize()
{
	inherited::initialize					();

	object->path().prepare_builder	();	
	
	
	target_point	= object->EnemyMan.get_enemy_position();
	target_vertex	= object->EnemyMan.get_enemy_vertex();

	Fvector dir;
	dir.sub(target_point, object->Position());
	dir.normalize();
	
	Fvector test_position;
	test_position.mad(target_point, dir, 10.f);
	
	// провериь возможность пробежать дальше
	if (ai().level_graph().valid_vertex_position(test_position)) {
		u32 vertex_id = ai().level_graph().vertex_id(test_position);
		if (ai().level_graph().valid_vertex_id(vertex_id)) {
			target_point	= test_position;
			target_vertex	= vertex_id;
		}
	}
}

TEMPLATE_SPECIALIZATION
void CStateMonsterFindEnemyRunAbstract::execute()
{
	object->set_action						(ACT_RUN);
	object->anim().accel_activate			(eAT_Aggressive);
	object->anim().accel_set_braking		(false);
	object->path().set_target_point			(target_point, target_vertex);
	object->path().set_rebuild_time			(0);
	object->path().set_use_covers			();
	object->path().set_cover_params			(5.f, 30.f, 1.f, 30.f);
	object->path().set_try_min_time			(false);
	object->set_state_sound					(MonsterSound::eMonsterSoundAggressive);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterFindEnemyRunAbstract::check_completion()
{	
	if ((object->ai_location().level_vertex_id() == target_vertex) && 
		!object->control().path_builder().is_moving_on_path()) return true;

	return false;
}

