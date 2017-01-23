#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateBurerAttackRunAroundAbstract CStateBurerAttackRunAround<_Object>


#define DIST_QUANT			10.f

TEMPLATE_SPECIALIZATION
CStateBurerAttackRunAroundAbstract::CStateBurerAttackRunAround(_Object *obj) : inherited(obj)
{
}

TEMPLATE_SPECIALIZATION
void CStateBurerAttackRunAroundAbstract::initialize()
{
	inherited::initialize		();

	time_started				= Device.dwTimeGlobal;
	dest_direction.set			(0.f,0.f,0.f);

	// select point
	Fvector						dir_to_enemy, dir_from_enemy;
	dir_to_enemy.sub			(object->EnemyMan.get_enemy()->Position(),object->Position());
	dir_to_enemy.normalize		();

	dir_from_enemy.sub			(object->Position(),object->EnemyMan.get_enemy()->Position());
	dir_from_enemy.normalize	();

	float dist = object->Position().distance_to(object->EnemyMan.get_enemy()->Position());

	if (dist > 30.f) {							// бежать к врагу
		selected_point.mad(object->Position(),dir_to_enemy,DIST_QUANT);
	} else if ((dist < 20.f) && (dist > 4.f)) {	// убегать от врага
		selected_point.mad(object->Position(),dir_from_enemy,DIST_QUANT);
		dest_direction.sub			(object->EnemyMan.get_enemy()->Position(),selected_point);
		dest_direction.normalize	();
	} else {											// выбрать случайную позицию
		selected_point = random_position(object->Position(), DIST_QUANT);
		dest_direction.sub			(object->EnemyMan.get_enemy()->Position(),selected_point);
		dest_direction.normalize	();
	}
	
	object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateBurerAttackRunAroundAbstract::execute()
{
	if (!fis_zero(dest_direction.square_magnitude())) {
		object->path().set_use_dest_orient		(true);
		object->path().set_dest_direction		(dest_direction);
	} else object->path().set_use_dest_orient	(false);


	object->set_action							(ACT_RUN);
	object->path().set_target_point			(selected_point);
	object->path().set_generic_parameters	();
	object->path().set_use_covers			(false);

	object->set_state_sound						(MonsterSound::eMonsterSoundAggressive);
}


TEMPLATE_SPECIALIZATION
bool CStateBurerAttackRunAroundAbstract::check_start_conditions()
{
	return true;
}

TEMPLATE_SPECIALIZATION
bool CStateBurerAttackRunAroundAbstract::check_completion()
{
	if ((time_started + object->m_max_runaway_time < Device.dwTimeGlobal) || 
		(object->control().path_builder().is_moving_on_path() && object->control().path_builder().is_path_end(2.f))) {

		object->dir().face_target(object->EnemyMan.get_enemy());
		return true;
	}

	return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateBurerAttackRunAroundAbstract
