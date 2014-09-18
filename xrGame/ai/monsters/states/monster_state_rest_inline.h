#pragma once

#include "monster_state_rest_sleep.h"
#include "monster_state_rest_walk_graph.h"
#include "monster_state_rest_idle.h"
#include "monster_state_rest_fun.h"
#include "monster_state_squad_rest.h"
#include "monster_state_squad_rest_follow.h"
#include "state_move_to_restrictor.h"
#include "../ai_monster_squad.h"
#include "../ai_monster_squad_manager.h"
#include "../anomaly_detector.h"
#include "monster_state_home_point_rest.h"
#include "monster_state_smart_terrain_task.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterRestAbstract CStateMonsterRest<_Object>

#define TIME_DELAY_FUN	20000
#define TIME_IDLE		60000

TEMPLATE_SPECIALIZATION
CStateMonsterRestAbstract::CStateMonsterRest(_Object *obj) : inherited(obj)
{
	add_state(eStateRest_Sleep,			xr_new<CStateMonsterRestSleep<_Object> >	(obj));
	add_state(eStateRest_WalkGraphPoint,xr_new<CStateMonsterRestWalkGraph<_Object> >(obj));
	add_state(eStateRest_Idle,			xr_new<CStateMonsterRestIdle<_Object> >		(obj));
	add_state(eStateRest_Fun,			xr_new<CStateMonsterRestFun<_Object> >		(obj));
	add_state(eStateSquad_Rest,			xr_new<CStateMonsterSquadRest<_Object> >	(obj));
	add_state(eStateSquad_RestFollow,	xr_new<CStateMonsterSquadRestFollow<_Object> >(obj));
	add_state(eStateCustomMoveToRestrictor, xr_new<CStateMonsterMoveToRestrictor<_Object> > (obj));
	add_state(eStateRest_MoveToHomePoint, xr_new<CStateMonsterRestMoveToHomePoint<_Object> > (obj));
	add_state(eStateSmartTerrainTask, xr_new<CStateMonsterSmartTerrainTask<_Object> > (obj));
}

TEMPLATE_SPECIALIZATION
CStateMonsterRestAbstract::~CStateMonsterRest	()
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestAbstract::initialize()
{
	inherited::initialize	();

	time_last_fun			= 0;
	time_idle_selected		= Random.randI(2) ? 0 : time();
	object->anomaly_detector().activate();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestAbstract::finalize()
{
	inherited::finalize();
	
	object->anomaly_detector().deactivate();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestAbstract::critical_finalize()
{
	inherited::critical_finalize();

	object->anomaly_detector().deactivate();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestAbstract::execute()
{
	// check alife control
	bool captured_by_smart_terrain = false;
	
	if (prev_substate == eStateSmartTerrainTask) {
		if (!get_state(eStateSmartTerrainTask)->check_completion()) 
			captured_by_smart_terrain = true;
	} else if (get_state(eStateSmartTerrainTask)->check_start_conditions()) 
		captured_by_smart_terrain = true;

	if (captured_by_smart_terrain) select_state(eStateSmartTerrainTask);
	else {
		// check restrictions
		bool move_to_restrictor = false;

		if (prev_substate == eStateCustomMoveToRestrictor) {
			if (!get_state(eStateCustomMoveToRestrictor)->check_completion()) 
				move_to_restrictor = true;
		} else if (get_state(eStateCustomMoveToRestrictor)->check_start_conditions()) 
			move_to_restrictor = true;

		if (move_to_restrictor) select_state(eStateCustomMoveToRestrictor);
		else {
			// check home point
			bool move_to_home_point = false;

			if (prev_substate == eStateRest_MoveToHomePoint) {
				if (!get_state(eStateRest_MoveToHomePoint)->check_completion()) 
					move_to_home_point = true;
			} else if (get_state(eStateRest_MoveToHomePoint)->check_start_conditions()) 
				move_to_home_point = true;

			if (move_to_home_point) select_state(eStateRest_MoveToHomePoint);
			else {
				// check squad behaviour
				bool use_squad = false;

				if (monster_squad().get_squad(object)->GetCommand(object).type == SC_REST) {
					select_state	(eStateSquad_Rest);
					use_squad		= true;
				} else if (monster_squad().get_squad(object)->GetCommand(object).type == SC_FOLLOW) {
					select_state	(eStateSquad_RestFollow);
					use_squad		= true;
				} 

				if (!use_squad) {
					if (time_idle_selected + TIME_IDLE > time()) 			select_state	(eStateRest_Idle);
					else if (time_idle_selected + TIME_IDLE  + TIME_IDLE/2 > time()) 	select_state	(eStateRest_WalkGraphPoint);
					else {
						time_idle_selected	= time();
						select_state		(eStateRest_Idle);
					}
				}
			}
		}
	}

	get_state_current()->execute();
	prev_substate = current_substate;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterRestAbstract
