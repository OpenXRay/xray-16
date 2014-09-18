////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path_params.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol path parameters class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"
#include "patrol_path_manager_space.h"
#include "game_graph_space.h"

class CPatrolPath;

class CPatrolPathParams {
public:
	const CPatrolPath						*m_path;
	shared_str								m_path_name;
	PatrolPathManager::EPatrolStartType		m_tPatrolPathStart;
	PatrolPathManager::EPatrolRouteType		m_tPatrolPathStop;
	bool									m_bRandom;
	u32										m_previous_index;

public:
									CPatrolPathParams	(LPCSTR caPatrolPathToGo, const PatrolPathManager::EPatrolStartType tPatrolPathStart = PatrolPathManager::ePatrolStartTypeNearest, const PatrolPathManager::EPatrolRouteType tPatrolPathStop = PatrolPathManager::ePatrolRouteTypeContinue, bool bRandom = true, u32 index = u32(-1));
	virtual							~CPatrolPathParams	();
			u32						count				() const;
			const Fvector			&point				(u32 index) const;
			u32						level_vertex_id		(u32 index) const;
			GameGraph::_GRAPH_ID	game_vertex_id		(u32 index) const;
			u32						point				(LPCSTR name) const;
			u32						point				(const Fvector &point) const;
			LPCSTR					name				(u32 index) const;	
			bool					flag				(u32 index, u8 flag_index) const;
			Flags32					flags				(u32 index) const;
			bool					terminal			(u32 index) const;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CPatrolPathParams)
#undef script_type_list
#define script_type_list save_type_list(CPatrolPathParams)

#include "patrol_path_params_inline.h"