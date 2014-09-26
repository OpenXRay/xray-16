////////////////////////////////////////////////////////////////////////////
//	Module 		: game_graph.h
//	Created 	: 18.02.2003
//  Modified 	: 13.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Game graph class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "game_graph_space.h"
#include "script_export_space.h"
#include "game_level_cross_table.h"

class CGameGraph {
private:
	friend class CRenumbererConverter;

public:
	typedef GameGraph::_GRAPH_ID    _GRAPH_ID;
	typedef GameGraph::_LEVEL_ID	_LEVEL_ID;
	typedef GameGraph::_LOCATION_ID	_LOCATION_ID;
	typedef GameGraph::SLevel		SLevel;
	typedef GameGraph::CEdge		CEdge;
	typedef GameGraph::CVertex		CVertex;
	typedef GameGraph::CHeader		CHeader;
	typedef GameGraph::CLevelPoint	CLevelPoint;

public:		
	typedef const CEdge				*const_iterator;
	typedef const CLevelPoint		*const_spawn_iterator;
	typedef xr_vector<CLevelPoint>	LEVEL_POINT_VECTOR;
	typedef xr_vector<bool>			ENABLED;

private:
	CHeader							m_header;
#ifdef AI_COMPILER
	IReader							*m_reader;
#endif // AI_COMPILER
	CVertex							*m_nodes;
	mutable ENABLED					m_enabled;
	_GRAPH_ID						m_current_level_some_vertex_id;

private:
	u32								*m_cross_tables;
	CGameLevelCrossTable			*m_current_level_cross_table;

public:
#ifdef AI_COMPILER
	IC 								CGameGraph				(LPCSTR file_name, u32 current_version = XRAI_CURRENT_VERSION);
#endif // AI_COMPILER

public:
	IC								CGameGraph				(const IReader &stream);
	IC		void					save					(IWriter &stream);
	IC	const CGameLevelCrossTable	&cross_table			() const;

public:
	IC virtual						~CGameGraph				();
	IC		const CHeader			&header					() const;
	IC		bool					mask					(const svector<_LOCATION_ID,GameGraph::LOCATION_TYPE_COUNT> &M, const _LOCATION_ID E[GameGraph::LOCATION_TYPE_COUNT]) const;
	IC		bool					mask					(const _LOCATION_ID M[GameGraph::LOCATION_TYPE_COUNT], const _LOCATION_ID E[GameGraph::LOCATION_TYPE_COUNT]) const;
	IC		float					distance				(const _GRAPH_ID tGraphID0, const _GRAPH_ID tGraphID1) const;
	IC		bool					accessible				(u32 vertex_id) const;
	IC		void					accessible				(u32 vertex_id, bool value) const;
	IC		bool					valid_vertex_id			(u32 vertex_id) const;
	IC		void					begin					(u32 vertex_id, const_iterator &start, const_iterator &end) const;
	IC		void					begin_spawn				(u32 vertex_id, const_spawn_iterator &start, const_spawn_iterator &end) const;
	IC		const _GRAPH_ID			&value					(u32 vertex_id, const_iterator &i) const;
	IC		const float				&edge_weight			(const_iterator i) const;
	IC		const CVertex			*vertex					(u32 vertex_id) const;
	IC		void					set_invalid_vertex		(_GRAPH_ID &vertex_id) const;
	IC		_GRAPH_ID				vertex_id				(const CVertex *vertex) const;
	IC		void					set_current_level		(u32 level_id);
	IC		const _GRAPH_ID			&current_level_vertex	() const;
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CGameGraph)
#undef script_type_list
#define script_type_list save_type_list(CGameGraph)

#include "game_graph_inline.h"