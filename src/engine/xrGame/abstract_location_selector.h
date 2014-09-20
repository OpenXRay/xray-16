////////////////////////////////////////////////////////////////////////////
//	Module 		: abstract_location_selector.h
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Abstract location selector
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "restricted_object.h"

template <
	typename _Graph,
	typename _VertexEvaluator,
	typename _vertex_id_type
>
class CAbstractLocationSelector {
protected:
	bool						m_failed;
	_VertexEvaluator			*m_evaluator;
	_vertex_id_type				m_selected_vertex_id;
	const _Graph				*m_graph;
	u32							m_last_query_time;
	u32							m_query_interval;
	xr_vector<_vertex_id_type>	*m_path;
	_vertex_id_type				*dest_vertex_id;
	CRestrictedObject			*m_restricted_object;
	
protected:
	IC			void			perform_search				(const _vertex_id_type game_vertex_id);
	IC	virtual	void			before_search				(_vertex_id_type &vertex_id);
	IC	virtual	void			after_search				();

public:
	IC							CAbstractLocationSelector	(CRestrictedObject *object);
	IC	virtual					~CAbstractLocationSelector	();
	IC	virtual void			reinit						(const _Graph *graph = 0);

	IC			_vertex_id_type get_selected_vertex_id		() const;

	IC			void			set_query_interval			(const u32 query_interval);

	IC			void			set_evaluator				(_VertexEvaluator *evaluator);

	IC			bool			failed						() const;
	IC			bool			actual						(const _vertex_id_type start_vertex_id, bool path_completed);
	IC			bool			used						() const;
	IC			void			select_location				(const _vertex_id_type start_vertex_id, bool path_completed);
	// При поиске ноды сохранить найденный _кратчайший_ путь и найденную ноду
	IC			void			set_dest_path				(xr_vector<_vertex_id_type> &path);
	IC			void			set_dest_vertex				(_vertex_id_type &vertex_id);
};

#include "abstract_location_selector_inline.h"

template <
	typename _Graph,
	typename _VertexEvaluator,
	typename _vertex_id_type
>
class 
	CBaseLocationSelector :
	public CAbstractLocationSelector <
		_Graph,
		_VertexEvaluator,
		_vertex_id_type
	>
{
};
