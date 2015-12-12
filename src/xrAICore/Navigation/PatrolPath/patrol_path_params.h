////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path_params.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol path parameters class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrAICore/xrAICore.hpp"
#include "xrAICore/Navigation/game_graph_space.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"

class CPatrolPath;
class CPatrolPathStorage;

class XRAICORE_API CPatrolPathParams
{
public:
	const CPatrolPath						*m_path;
	shared_str								m_path_name;
	EPatrolStartType		m_tPatrolPathStart;
	EPatrolRouteType		m_tPatrolPathStop;
	bool									m_bRandom;
	u32										m_previous_index;

public:
    CPatrolPathParams(LPCSTR caPatrolPathToGo, EPatrolStartType tPatrolPathStart = ePatrolStartTypeNearest,
        EPatrolRouteType tPatrolPathStop = ePatrolRouteTypeContinue, bool bRandom = true, u32 index = u32(-1));
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
};

#include "xrAICore/Navigation/PatrolPath/patrol_path_params_inline.h"
