////////////////////////////////////////////////////////////////////////////
//	Module 		: CVertex.h
//	Created 	: 14.01.2004
//  Modified 	: 19.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Graph vertex base class template
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/object_broker.h"
#include "xrCommon/xr_vector.h"

template <typename _data_type, typename _vertex_id_type, typename _graph_type>
class CGraphVertex
{
public:
    typedef _vertex_id_type vertex_id_type;
    typedef typename _graph_type::CEdge edge_type;
    typedef typename _graph_type::CEdge::edge_weight_type edge_weight_type;
    typedef xr_vector<edge_type> EDGES;
    typedef xr_vector<CGraphVertex*> VERTICES;

private:
    _vertex_id_type m_vertex_id;
    EDGES m_edges;
    _data_type m_data;
    // this container holds vertices, which has edges to us
    // this is needed for the fast vertex removal
    VERTICES m_vertices;
    // this counter is use for fast edge count computation in graph
    size_t* m_edge_count;

public:
    IC CGraphVertex(const _data_type& data, const _vertex_id_type& vertex_id, size_t* edge_count);
    IC ~CGraphVertex();
    IC bool operator==(const CGraphVertex& obj) const;
    IC void add_edge(CGraphVertex* vertex, const typename _graph_type::CEdge::edge_weight_type& edge_weight);
    IC void remove_edge(const _vertex_id_type& vertex_id);
    IC void on_edge_addition(CGraphVertex* vertex);
    IC void on_edge_removal(const CGraphVertex* vertex);
    IC const _vertex_id_type& vertex_id() const;
    IC const _data_type& data() const;
    IC _data_type& data();
    IC void data(const _data_type& data);
    IC const EDGES& edges() const;
    IC const edge_type* edge(const _vertex_id_type& vertex_id) const;
    IC edge_type* edge(const _vertex_id_type& vertex_id);
};

#include "graph_vertex_inline.h"
