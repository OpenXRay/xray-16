////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_surge_manager.cpp
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator surge manager
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_surge_manager.h"
#include "alife_object_registry.h"
#include "alife_spawn_registry.h"
#include "alife_time_manager.h"
#include "alife_graph_registry.h"
#include "alife_schedule_registry.h"
#include "alife_simulator_header.h"
#include "ai_space.h"
#include "ef_storage.h"
#include "ef_pattern.h"
#include "graph_engine.h"
#include "xrserver.h"
#include "alife_human_brain.h"

using namespace ALife;

CALifeSurgeManager::~CALifeSurgeManager	()
{
}

void CALifeSurgeManager::spawn_new_spawns			()
{
	xr_vector<ALife::_SPAWN_ID>::const_iterator	I = m_temp_spawns.begin();
	xr_vector<ALife::_SPAWN_ID>::const_iterator	E = m_temp_spawns.end();
	for ( ; I != E; ++I) {
		CSE_ALifeDynamicObject	*object, *spawn = smart_cast<CSE_ALifeDynamicObject*>(&spawns().spawns().vertex(*I)->data()->object());
		VERIFY3					(spawn,spawns().spawns().vertex(*I)->data()->object().name(),spawns().spawns().vertex(*I)->data()->object().name_replace());

#ifdef DEBUG
		CTimer					timer;
		timer.Start				();
#endif
		create					(object,spawn,*I);
#ifdef DEBUG
		if (psAI_Flags.test(aiALife))
			Msg					("LSS : SURGE : SPAWN : [%s],[%s], level %s, time %f ms",*spawn->s_name,spawn->name_replace(),*ai().game_graph().header().level(ai().game_graph().vertex(spawn->m_tGraphID)->level_id()).name(),timer.GetElapsed_sec()*1000.f);
#endif
	}
}

void CALifeSurgeManager::fill_spawned_objects		()
{
	m_temp_spawned_objects.clear	();

	D_OBJECT_P_MAP::const_iterator	I = objects().objects().begin();
	D_OBJECT_P_MAP::const_iterator	E = objects().objects().end();
	for ( ; I != E; ++I)
		if (spawns().spawns().vertex((*I).second->m_tSpawnID))
			m_temp_spawned_objects.push_back	((*I).second->m_tSpawnID);
}

void CALifeSurgeManager::spawn_new_objects			()
{
	fill_spawned_objects			();
	spawns().fill_new_spawns		(m_temp_spawns,time_manager().game_time(),m_temp_spawned_objects);
	spawn_new_spawns				();
	VERIFY							(graph().actor());
}
