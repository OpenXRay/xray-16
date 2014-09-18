////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_smart_terrain_task.h
//	Created 	: 20.09.2005
//  Modified 	: 20.09.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife smart terrain task
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "game_graph_space.h"
#include "script_export_space.h"

class CPatrolPoint;

class CALifeSmartTerrainTask {
private:
#ifdef DEBUG
	shared_str						m_patrol_path_name;
	u32								m_patrol_point_index;
#endif
	const CPatrolPoint				*m_patrol_point;
	GameGraph::_GRAPH_ID			m_game_vertex_id;
	u32								m_level_vertex_id;

#ifdef DEBUG
private:
	IC		const shared_str		&patrol_path_name		() const;
	IC		const u32				&patrol_point_index		() const;
#endif

private:
			void					setup_patrol_point		(const shared_str &patrol_path_name, const u32 &patrol_point_index);
	IC		const CPatrolPoint		&patrol_point			() const;
	IC		void					init					(const shared_str &patrol_path_name, const u32 &patrol_point_index);

public:
									CALifeSmartTerrainTask	(LPCSTR patrol_path_name);
									CALifeSmartTerrainTask	(LPCSTR patrol_path_name, const u32 &patrol_point_index);
									CALifeSmartTerrainTask	(const shared_str &patrol_path_name);
									CALifeSmartTerrainTask	(const shared_str &patrol_path_name, const u32 &patrol_point_index = 0);
									CALifeSmartTerrainTask	(const GameGraph::_GRAPH_ID &game_vertex_id,	const u32 &level_vertex_id);
			GameGraph::_GRAPH_ID	game_vertex_id			() const;
			u32						level_vertex_id			() const;
			Fvector					position				() const;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CALifeSmartTerrainTask)
#undef script_type_list
#define script_type_list save_type_list(CALifeSmartTerrainTask)

#include "alife_smart_terrain_task_inline.h"