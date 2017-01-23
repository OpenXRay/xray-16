#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

//////////////////////////////////////////////////////////////////////////
// CStateGroupSquadMoveToRadiusEx with path rebuild options
//////////////////////////////////////////////////////////////////////////

#define CStateGroupSquadMoveToRadiusExAbstract CStateGroupSquadMoveToRadiusEx<_Object>

TEMPLATE_SPECIALIZATION
void CStateGroupSquadMoveToRadiusExAbstract::initialize()
{
	inherited::initialize();
	object->path().prepare_builder();	
}

TEMPLATE_SPECIALIZATION
void CStateGroupSquadMoveToRadiusExAbstract::execute()
{
	CMonsterSquad *squad	= monster_squad().get_squad(object);
	if (squad && squad->SquadActive())
	{
		if (squad->get_index(object) != u8(-1)) {
			float m_Angle = (PI - PI_DIV_2) / (squad->squad_alife_count() - 1) * (squad->get_index(object) - 1);
			float m_Delta_Angle = Random.randF(PI_DIV_3 / (squad->squad_alife_count() - 1));
			float m_heading, m_pitch;
			Fvector m_enemy_position = object->EnemyMan.get_enemy()->Position();

			Fvector to_direction = object->Home->get_home_point();
			to_direction.sub(m_enemy_position);
			to_direction.normalize_safe();

			to_direction.getHP(m_heading, m_pitch);
			m_heading = angle_normalize(m_heading - (PI - PI_DIV_3) / 2 + m_Angle + m_Delta_Angle);
			to_direction.setHP(m_heading, m_pitch);
			data.point.x = m_enemy_position.x + data.completion_dist * to_direction.x;
			data.point.y = m_enemy_position.y + data.completion_dist * to_direction.y;
			data.point.z = m_enemy_position.z + data.completion_dist * to_direction.z;
			if (!ai().level_graph().valid_vertex_position(data.point))
			{
				data.point = object->EnemyMan.get_enemy()->Position();
			}
		} else {
			data.point = object->EnemyMan.get_enemy()->Position();
		}
	} else {
		data.point = object->EnemyMan.get_enemy()->Position();
	}
	object->set_action									(data.action.action);
	object->anim().SetSpecParams						(data.action.spec_params);

	object->path().set_target_point			(data.point,data.vertex);
	object->path().set_rebuild_time			(data.time_to_rebuild);
	object->path().set_distance_to_end		(2.f);
	object->path().set_use_covers			();
	object->path().set_cover_params			(5.f, 30.f, 1.f, 30.f);

	if (data.accelerated) {
		object->anim().accel_activate	(EAccelType(data.accel_type));
		object->anim().accel_set_braking (data.braking);
	}

	if (data.action.sound_type != u32(-1)) {
		object->set_state_sound(data.action.sound_type, data.action.sound_delay == u32(-1));
	}
}

TEMPLATE_SPECIALIZATION
bool CStateGroupSquadMoveToRadiusExAbstract::check_completion()
{	
	if (data.action.time_out !=0) {
		if (time_state_started + data.action.time_out < Device.dwTimeGlobal) return true;
	}
	if (object->Position().distance_to_xz(object->EnemyMan.get_enemy()->Position()) < data.completion_dist - 2.f) return true;
	if (data.point.distance_to_xz(object->Position()) <= 2.f) return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////
// CStateGroupSquadMoveToRadiusEx with path rebuild options
//////////////////////////////////////////////////////////////////////////

#define CStateGroupSquadMoveToRadiusAbstract CStateGroupSquadMoveToRadius<_Object>

TEMPLATE_SPECIALIZATION
void CStateGroupSquadMoveToRadiusAbstract::initialize()
{
	inherited::initialize();
	object->path().prepare_builder();	
}

TEMPLATE_SPECIALIZATION
void CStateGroupSquadMoveToRadiusAbstract::execute()
{
	Fvector m_enemy_position = object->EnemyMan.get_enemy()->Position();

	Fvector to_direction = object->Position();
	to_direction.sub(m_enemy_position);
	to_direction.normalize_safe();

	data.point.x = m_enemy_position.x + data.completion_dist * to_direction.x;
	data.point.y = m_enemy_position.y + data.completion_dist * to_direction.y;
	data.point.z = m_enemy_position.z + data.completion_dist * to_direction.z;
	if (!ai().level_graph().valid_vertex_position(data.point))
	{
		data.point = object->EnemyMan.get_enemy()->Position();
	}

	object->set_action									(data.action.action);
	object->anim().SetSpecParams						(data.action.spec_params);

	object->path().set_target_point			(data.point,data.vertex);
	object->path().set_rebuild_time			(data.time_to_rebuild);
	object->path().set_distance_to_end		(1.f);
	object->path().set_use_covers			();
	object->path().set_cover_params			(5.f, 30.f, 1.f, 30.f);

	if (data.accelerated) {
		object->anim().accel_activate	(EAccelType(data.accel_type));
		object->anim().accel_set_braking (data.braking);
	}

	if (data.action.sound_type != u32(-1)) {
		object->set_state_sound(data.action.sound_type, data.action.sound_delay == u32(-1));
	}
}

TEMPLATE_SPECIALIZATION
bool CStateGroupSquadMoveToRadiusAbstract::check_completion()
{	
	if (data.action.time_out !=0) {
		if (time_state_started + data.action.time_out < Device.dwTimeGlobal) return true;
	}

	if (data.point.distance_to_xz(object->Position()) <= 1.f) return true;

	return false;
}

