////////////////////////////////////////////////////////////////////////////
//	Module 		: graph_vertex_base_inline.h
//	Created 	: 14.01.2004
//  Modified 	: 19.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Graph vertex base class template inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _data_type,\
	typename _vertex_id_type,\
	typename _graph_type\
>

#define CSGraphVertex CVertex<\
	_data_type,\
	_vertex_id_type,\
	_graph_type\
>

TEMPLATE_SPECIALIZATION
IC	CSGraphVertex::CVertex					(const _data_type &data, const _vertex_id_type &vertex_id, size_t *edge_count)
{
	m_data					= data;
	m_vertex_id				= vertex_id;
	VERIFY					(edge_count);
	m_edge_count			= edge_count;
}

TEMPLATE_SPECIALIZATION
IC	CSGraphVertex::~CVertex				()
{
	while (!edges().empty())
		remove_edge			(edges().back().vertex_id());

	while (!m_vertices.empty())
		m_vertices.back()->remove_edge(vertex_id());

	try {
		delete_data			(m_data);
	}
	catch(...) {
	}
}

TEMPLATE_SPECIALIZATION
IC	const typename CSGraphVertex::_edge_type *CSGraphVertex::edge	(const _vertex_id_type &vertex_id) const
{
	EDGES::const_iterator	I = std::find(edges().begin(),edges().end(),vertex_id);
	if (m_edges.end() == I)
		return				(0);
	return					(&*I);
}

TEMPLATE_SPECIALIZATION
IC	typename CSGraphVertex::_edge_type *CSGraphVertex::edge			(const _vertex_id_type &vertex_id)
{
	EDGES::iterator			I = std::find(m_edges.begin(),m_edges.end(),vertex_id);
	if (m_edges.end() == I)
		return				(0);
	return					(&*I);
}

TEMPLATE_SPECIALIZATION
IC	void CSGraphVertex::add_edge						(CVertex *vertex, const _edge_weight_type &edge_weight)
{
	EDGES::iterator			I = std::find(m_edges.begin(),m_edges.end(),vertex->vertex_id());
	VERIFY					(m_edges.end() == I);
	vertex->on_edge_addition(this);
	m_edges.push_back		(_edge_type(edge_weight,vertex));
	++*m_edge_count;
}

TEMPLATE_SPECIALIZATION
IC	void CSGraphVertex::remove_edge						(const _vertex_id_type &vertex_id)
{
	EDGES::iterator			I = std::find(m_edges.begin(),m_edges.end(),vertex_id);
	VERIFY					(m_edges.end() != I);
	CVertex					*vertex = (*I).vertex();
	vertex->on_edge_removal	(this);
	m_edges.erase			(I);
	--*m_edge_count;
}

TEMPLATE_SPECIALIZATION
IC	void CSGraphVertex::on_edge_addition				(CVertex *vertex)
{
	VERTICES::const_iterator	I = std::find(m_vertices.begin(),m_vertices.end(),vertex);
	VERIFY						(I == m_vertices.end());
	m_vertices.push_back		(vertex);
}

TEMPLATE_SPECIALIZATION
IC	void CSGraphVertex::on_edge_removal					(const CVertex *vertex)
{
	VERTICES::iterator			I = std::find(m_vertices.begin(),m_vertices.end(),vertex);
	VERIFY						(I != m_vertices.end());
	m_vertices.erase			(I);
}

TEMPLATE_SPECIALIZATION
IC	const _vertex_id_type &CSGraphVertex::vertex_id		() const
{
	return			(m_vertex_id);
}

TEMPLATE_SPECIALIZATION
IC	const _data_type &CSGraphVertex::data	() const
{
	return			(m_data);
}

TEMPLATE_SPECIALIZATION
IC	_data_type &CSGraphVertex::data			()
{
	return			(m_data);
}

TEMPLATE_SPECIALIZATION
IC	void CSGraphVertex::data				(const _data_type &data)
{
	m_data			= data;
}

TEMPLATE_SPECIALIZATION
IC	const typename CSGraphVertex::EDGES &CSGraphVertex::edges	() const
{
	return			(m_edges);
}

TEMPLATE_SPECIALIZATION
IC	bool CSGraphVertex::operator==	(const CVertex &obj) const
{
	if (vertex_id() != obj.vertex_id())
		return		(false);

	if (!equal(edges(),obj.edges()))
		return		(false);

	return			(equal(data(),obj.data()));
}

#undef TEMPLATE_SPECIALIZATION
#undef CSGraphVertex