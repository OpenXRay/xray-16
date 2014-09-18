#pragma once

#include "../states/state_move_to_point.h"
#include "../states/monster_state_home_point_danger.h"

#include "../ai_monster_squad.h"
#include "../../../entity.h"
#include "../../../ai_object_location.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateGroupHearDangerousSoundAbstract CStateGroupHearDangerousSound<_Object>

#define LEADER_RADIUS			20.f
#define FIND_POINT_ATTEMPTS		5

TEMPLATE_SPECIALIZATION
CStateGroupHearDangerousSoundAbstract::CStateGroupHearDangerousSound(_Object *obj) : inherited(obj)
{
	add_state	(eStateHearDangerousSound_Hide,				xr_new<CStateMonsterMoveToPointEx<_Object> >		(obj));
	add_state	(eStateSquad,								xr_new<CStateMonsterMoveToPoint<_Object> >			(obj));
	add_state	(eStateHearDangerousSound_Home,				xr_new<CStateMonsterDangerMoveToHomePoint<_Object> >(obj));
}

TEMPLATE_SPECIALIZATION
void CStateGroupHearDangerousSoundAbstract::initialize()
{
	inherited::initialize				();
}

TEMPLATE_SPECIALIZATION
void CStateGroupHearDangerousSoundAbstract::reselect_state()
{
	CMonsterSquad *squad = monster_squad().get_squad(object);
	VERIFY(squad);

	if (get_state(eStateHearDangerousSound_Home)->check_start_conditions())	{
		select_state(eStateHearDangerousSound_Home);
		return;
	}

	if (squad->SquadActive() && squad->GetCommand(object).type == SC_REST)
	{
		if (object != squad->GetLeader())
		{
			select_state(eStateSquad);
		} else {
			select_state(eStateHearDangerousSound_Hide);
		}
	} else {
		squad->SetLeader(object);
		SMemberGoal			goal;

		goal.type			= MG_Rest;
		goal.entity			= const_cast<CEntityAlive*>(object->EnemyMan.get_enemy());

		squad->UpdateGoal	(object, goal);
		
		select_state(eStateHearDangerousSound_Hide);
		squad->UpdateSquadCommands();
	}
}

TEMPLATE_SPECIALIZATION
void CStateGroupHearDangerousSoundAbstract::setup_substates()
{
	state_ptr state = get_state_current();

	if (current_substate == eStateSquad) {
		SStateDataMoveToPoint data;
		CMonsterSquad	*squad = monster_squad().get_squad(object);

		if (object->control().path_builder().get_node_in_radius(squad->GetLeader()->ai_location().level_vertex_id(), 8.f, LEADER_RADIUS, FIND_POINT_ATTEMPTS, data.vertex)) {
			data.point			= ai().level_graph().vertex_position(data.vertex);
		} else {

			Fvector dest_pos = random_position(squad->GetLeader()->Position(), LEADER_RADIUS);
			if (!object->control().path_builder().restrictions().accessible(dest_pos)) {
				data.vertex		= object->control().path_builder().restrictions().accessible_nearest(dest_pos, data.point);
			} else {
				data.point		= dest_pos;
				data.vertex		= u32(-1);
			}
		}
		

		data.action.action	= ACT_RUN;
		data.accelerated	= true;
		data.braking		= false;
		data.accel_type 	= eAT_Calm;
		data.completion_dist= 3.f;
		data.action.sound_type	= MonsterSound::eMonsterSoundIdle;
		data.action.sound_delay = object->db().m_dwIdleSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPoint));

		return;
	}

	if (current_substate == eStateHearDangerousSound_Hide) {

		SStateDataMoveToPointEx data;

		data.vertex				= 0;

		Fvector home2sound = object->Home->get_home_point();
		home2sound.sub(object->SoundMemory.GetSound().position);
		home2sound.normalize_safe();

		m_target_node = object->Home->get_place_in_max_home_to_direction(home2sound);
		
		if (m_target_node == u32(-1))
		{
			data.point			= object->Position();
		} else {
			data.point			= ai().level_graph().vertex_position(m_target_node);
		}

		data.action.action		= ACT_RUN;
		data.action.time_out	= 0;		// do not use time out
		data.completion_dist	= 3.f;		// get exactly to the point
		data.time_to_rebuild	= 0;		
		data.accelerated		= true;
		data.braking			= true;
		data.accel_type 		= eAT_Aggressive;
		data.action.sound_type	= (u32)MonsterSound::eMonsterSoundDummy;
		data.action.sound_delay = object->db().m_dwAttackSndDelay;

		state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));

		return;
	}
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupHearDangerousSoundAbstract 
