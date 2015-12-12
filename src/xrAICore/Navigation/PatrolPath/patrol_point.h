////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_point.h
//	Created 	: 15.06.2004
//  Modified 	: 15.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol point
////////////////////////////////////////////////////////////////////////////

#pragma once

class CPatrolPath;
class CLevelGraph;
class CGameLevelCrossTable;
class CGameGraph;

#include "xrAICore/xrAICore.hpp"
#include "Common/object_interfaces.h"
#include "xrAICore/Navigation/game_graph_space.h"

class XRAICORE_API CPatrolPoint : public ISerializable
{
protected:
	shared_str							m_name;
	Fvector								m_position;
	u32									m_flags;
	u32									m_level_vertex_id;
	GameGraph::_GRAPH_ID				m_game_vertex_id;

protected:
#ifdef DEBUG
	bool								m_initialized;
	const CPatrolPath					*m_path;
#endif

private:
	IC		void						correct_position	(const CLevelGraph *level_graph, const CGameLevelCrossTable *cross, const CGameGraph *game_graph);
#ifdef DEBUG
			void						verify_vertex_id	(const CLevelGraph *level_graph, const CGameLevelCrossTable *cross, const CGameGraph *game_graph) const;
#endif

public:
										CPatrolPoint		(const CLevelGraph *level_graph, const CGameLevelCrossTable *cross, const CGameGraph *game_graph, const CPatrolPath *path, const Fvector &position, u32 level_vertex_id, u32 flags, shared_str name);
										CPatrolPoint		(const CPatrolPath *path = 0);
    inline bool operator==(const CPatrolPoint &rhs) const;
	virtual	void						load				(IReader &stream);
	virtual	void						save				(IWriter &stream);
			CPatrolPoint				&load_raw			(const CLevelGraph *level_graph, const CGameLevelCrossTable *cross, const CGameGraph *game_graph, IReader &stream);
	IC		const Fvector				&position			() const;
	IC		const u32					&level_vertex_id	(const CLevelGraph *level_graph, const CGameLevelCrossTable *cross, const CGameGraph *game_graph) const;
	IC		const GameGraph::_GRAPH_ID	&game_vertex_id		(const CLevelGraph *level_graph, const CGameLevelCrossTable *cross, const CGameGraph *game_graph) const;
	IC		const u32					&flags				() const;
	IC		const shared_str			&name				() const;
    // for xrGame
	const u32 &level_vertex_id() const;
    // for xrGame
	const GameGraph::_GRAPH_ID &game_vertex_id() const;

#ifdef DEBUG
public:
	IC		void						path				(const CPatrolPath *path);
#endif

};

#include "xrAICore/Navigation/PatrolPath/patrol_point_inline.h"
