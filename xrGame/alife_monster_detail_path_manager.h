////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_detail_path_manager.h
//	Created 	: 01.11.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster detail path manager class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "game_graph_space.h"
#include "alife_space.h"
#include "script_export_space.h"

class CMovementManagerHolder;
class CALifeSmartTerrainTask;

class CALifeMonsterDetailPathManager {
public:
	typedef CMovementManagerHolder		object_type;
	typedef xr_vector<u32>				PATH;

private:
	struct parameters {
		GameGraph::_GRAPH_ID			m_game_vertex_id;
		u32								m_level_vertex_id;
		Fvector							m_position;
	};

private:
	object_type							*m_object;
	ALife::_TIME_ID						m_last_update_time;
	parameters							m_destination;
	float								m_walked_distance;
	float								m_speed;

private:
	PATH								m_path;						
	// this is INVERTED path, i.e. 
	// start vertex is the last one
	// destination vertex is the first one.
	// this is useful, since iterating back
	// on this vector during path following
	// we just repeatedly remove the last 
	// vertex, and this operation is 
	// efficiently implemented in std::vector

private:
			void		actualize						();
			void		setup_current_speed				();
			void		follow_path						(const ALife::_TIME_ID &time_delta);
			void		update							(const ALife::_TIME_ID &time_delta);

public:
						CALifeMonsterDetailPathManager	(object_type *object);
	IC		object_type	&object							() const;

public:
			void		target							(const GameGraph::_GRAPH_ID &game_vertex_id, const u32 &level_vertex_id, const Fvector &position);
			void		target							(const GameGraph::_GRAPH_ID &game_vertex_id);
			void		target							(const CALifeSmartTerrainTask &task);
			void		target							(const CALifeSmartTerrainTask *task);

public:
			void		update							();
			void		on_switch_online				();
			void		on_switch_offline				();
	IC		void		speed							(const float &speed);

public:
	IC		const float	&speed							() const;
			bool		completed						() const;
			bool		actual							() const;
			void		make_inactual					();
			bool		failed							() const;
	IC		const PATH	&path							() const;
	IC		const float	&walked_distance				() const;
			Fvector		draw_level_position				() const;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CALifeMonsterDetailPathManager)
#undef script_type_list
#define script_type_list save_type_list(CALifeMonsterDetailPathManager)

#include "alife_monster_detail_path_manager_inline.h"