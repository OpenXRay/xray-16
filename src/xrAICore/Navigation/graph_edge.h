////////////////////////////////////////////////////////////////////////////
//	Module 		: CEdge.h
//	Created 	: 14.01.2004
//  Modified 	: 19.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Graph edge class template
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <loki/EmptyType.h>

template <typename _edge_weight_type, typename _vertex_type>
class CEdgeBase
{
public:
    typedef _edge_weight_type _edge_weight_type;
    typedef _vertex_type _vertex_type;
    typedef typename _vertex_type::_vertex_id_type _vertex_id_type;

private:
    _edge_weight_type m_weight;
    _vertex_type* m_vertex;

public:
    IC CEdgeBase(const _edge_weight_type& weight, _vertex_type* vertex);
    IC const _edge_weight_type& weight() const;
    IC _vertex_type* vertex() const;
    IC const _vertex_id_type& vertex_id() const;
};

template <typename _edge_weight_type, typename _vertex_type, typename _edge_data_type>
class CEdge : public CEdgeBase<_edge_weight_type, _vertex_type>
{
    using inherited = CEdgeBase<_edge_weight_type, _vertex_type>;

    _edge_data_type m_data;

public:
    using _vertex_id_type = typename inherited::_vertex_id_type;

    IC CEdge(const _edge_weight_type& weight, _vertex_type* vertex);
    IC bool operator==(const _vertex_id_type& vertex_id) const;
    IC bool operator==(const CEdge& obj) const;
    IC const _edge_data_type& data() const;
    IC _edge_data_type& data();
};

template <typename _edge_weight_type, typename _vertex_type>
class CEdge<_edge_weight_type, _vertex_type, Loki::EmptyType> : public CEdgeBase<_edge_weight_type, _vertex_type>
{
    using inherited = CEdgeBase<_edge_weight_type, _vertex_type>;

public:
    using _vertex_id_type = typename inherited::_vertex_id_type;

    IC CEdge(const _edge_weight_type& weight, _vertex_type* vertex);
    IC bool operator==(const _vertex_id_type& vertex_id) const;
    IC bool operator==(const CEdge& obj) const;
};

#include "graph_edge_inline.h"
