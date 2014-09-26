////////////////////////////////////////////////////////////////////////////
//	Module 		: location_selector_abstract_inline.h
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Abstract location selector inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "profiler.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Graph,\
	typename _VertexEvaluator,\
	typename _vertex_id_type\
>

#define CSelectorTemplate CAbstractLocationSelector<_Graph,_VertexEvaluator,_vertex_id_type>

TEMPLATE_SPECIALIZATION
IC	CSelectorTemplate::CAbstractLocationSelector	(CRestrictedObject *object)
{
	m_restricted_object		= object;
	VERIFY					(m_restricted_object);
}

TEMPLATE_SPECIALIZATION
IC	CSelectorTemplate::~CAbstractLocationSelector	()
{
}

TEMPLATE_SPECIALIZATION
IC	void CSelectorTemplate::reinit					(const _Graph *graph)
{
	m_failed				= false;
	m_selected_vertex_id	= _vertex_id_type(-1);
	m_evaluator				= 0;
	m_last_query_time		= 0;
	m_query_interval		= 0;
	m_graph					= graph;
	m_path					= 0;
	dest_vertex_id			= 0;
}	

TEMPLATE_SPECIALIZATION
IC	_vertex_id_type CSelectorTemplate::get_selected_vertex_id() const
{
	VERIFY					(m_graph->valid_vertex_id(m_selected_vertex_id));
	return					(m_selected_vertex_id);
}

TEMPLATE_SPECIALIZATION
IC	void CSelectorTemplate::set_evaluator	(_VertexEvaluator *evaluator)
{
	m_evaluator				= evaluator;
}

TEMPLATE_SPECIALIZATION
IC	void CSelectorTemplate::set_query_interval(const u32 query_interval)
{
	m_query_interval		= query_interval;
}

TEMPLATE_SPECIALIZATION
IC	bool CSelectorTemplate::actual(const _vertex_id_type start_vertex_id, bool path_completed)
{
	if (!used() || (((m_last_query_time + m_query_interval) > Device.dwTimeGlobal) && !path_completed))
		return				(true);

	perform_search			(start_vertex_id);
	if (!failed() && dest_vertex_id)
		*dest_vertex_id		= m_selected_vertex_id;
	
	return					(failed());
}

TEMPLATE_SPECIALIZATION
IC	bool CSelectorTemplate::failed() const
{
	return					(m_failed);
}

TEMPLATE_SPECIALIZATION
IC	bool CSelectorTemplate::used() const
{
	return					(m_evaluator && m_graph);
}

TEMPLATE_SPECIALIZATION
IC	void CSelectorTemplate::select_location	(const _vertex_id_type start_vertex_id, bool path_completed)
{
	if (used() && (((m_last_query_time + m_query_interval) <= Device.dwTimeGlobal) || path_completed)) {
		perform_search		(start_vertex_id);
		if (!failed() && dest_vertex_id) 
			*dest_vertex_id	= m_selected_vertex_id;
	}
	else m_failed			= false;
}

TEMPLATE_SPECIALIZATION
IC	void CSelectorTemplate::perform_search		(const _vertex_id_type vertex_id)
{
	START_PROFILE("Build Path/Selector Path");
	
	VERIFY						(m_evaluator && m_graph);

	_vertex_id_type				start_vertex_id = vertex_id;
	before_search				(start_vertex_id);

	m_last_query_time			= Device.dwTimeGlobal;
	
	m_evaluator->m_path			= m_path;
	ai().graph_engine().search	(*m_graph,start_vertex_id,start_vertex_id,0,*m_evaluator);
	m_failed	= 
		!m_graph->valid_vertex_id(m_evaluator->selected_vertex_id()) || 
		(m_evaluator->selected_vertex_id() == m_selected_vertex_id);
	
	if (!failed())
		m_selected_vertex_id	= m_evaluator->selected_vertex_id();

	after_search				();

	STOP_PROFILE;
}

TEMPLATE_SPECIALIZATION
IC	void CSelectorTemplate::set_dest_path		(xr_vector<_vertex_id_type> &path)
{
	m_path						= &path;
}

TEMPLATE_SPECIALIZATION
IC	void CSelectorTemplate::set_dest_vertex		(_vertex_id_type &vertex_id)
{
	dest_vertex_id				= &vertex_id;
}

TEMPLATE_SPECIALIZATION
IC	void CSelectorTemplate::before_search		(_vertex_id_type &vertex_id)
{
}

TEMPLATE_SPECIALIZATION
IC	void CSelectorTemplate::after_search		()
{
}


#undef CSelectorTemplate
#undef TEMPLATE_SPECIALIZATION