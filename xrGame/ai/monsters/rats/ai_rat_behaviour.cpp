////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_rat_fsm.cpp
//	Created 	: 25.04.2002
//  Modified 	: 07.11.2002
//	Author		: Dmitriy Iassenev
//	Description : AI Behaviour for monster "Rat"
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ai_rat.h"
#include "rat_state_manager.h"
#include "ai_space.h"
#include "game_level_cross_table.h"
#include "ai_object_location.h"
#include "game_graph.h"

void CAI_Rat::update_home_position	()
{
	if (!g_Alive())
		return;

	CEntity						*leader = Level().seniority_holder().team(g_Team()).squad(g_Squad()).leader();
	VERIFY						(leader);

	if (ID() != leader->ID())	{
		CAI_Rat					*rat_leader = smart_cast<CAI_Rat*>(leader);
		VERIFY					(rat_leader);
		if (m_home_position.distance_to(rat_leader->m_home_position) > EPS_L)
			add_active_member	(true);

		m_home_position			= rat_leader->m_home_position;
	}

	if (Device.dwTimeGlobal < m_time_to_change_graph_point)
		return;
	
	if (ai().cross_table().vertex(ai_location().level_vertex_id()).game_vertex_id() != m_next_graph_point)
		return;

	m_next_graph_point			= ai().cross_table().vertex(ai_location().level_vertex_id()).game_vertex_id();
	select_next_home_position	();
	m_home_position.set			(ai().game_graph().vertex(m_next_graph_point)->level_point());
}

void CAI_Rat::Think()
{
	update_morale				();
	update_home_position		();
	m_state_manager->update		();
}