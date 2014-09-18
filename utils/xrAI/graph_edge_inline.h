////////////////////////////////////////////////////////////////////////////
//	Module 		: graph_edge_inline.h
//	Created 	: 14.01.2004
//  Modified 	: 19.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Graph edge class template inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _edge_weight_type,\
	typename _vertex_type\
>

#define CSGraphEdge CEdge<_edge_weight_type,_vertex_type>

TEMPLATE_SPECIALIZATION
IC	CSGraphEdge::CEdge			(const _edge_weight_type &weight, _vertex_type *vertex)
{
	m_weight		= weight;
	VERIFY			(vertex);
	m_vertex		= vertex;
}

TEMPLATE_SPECIALIZATION
IC	typename const CSGraphEdge::_edge_weight_type &CSGraphEdge::weight	() const
{
	return			(m_weight);
}

TEMPLATE_SPECIALIZATION
IC	typename CSGraphEdge::_vertex_type *CSGraphEdge::vertex				() const
{
	return			(m_vertex);
}

TEMPLATE_SPECIALIZATION
IC	const typename CSGraphEdge::_vertex_id_type &CSGraphEdge::vertex_id	() const
{
	return			(vertex()->vertex_id());
}

TEMPLATE_SPECIALIZATION
IC	bool CSGraphEdge::operator==	(const _vertex_id_type &vertex_id) const
{
	return			(vertex()->vertex_id() == vertex_id);
}

TEMPLATE_SPECIALIZATION
IC	bool CSGraphEdge::operator==	(const CEdge &obj) const
{
	if (weight() != obj.weight())
		return		(false);

	return			(vertex()->vertex_id() == obj.vertex()->vertex_id());
}

#undef TEMPLATE_SPECIALIZATION
#undef CSGraphEdge