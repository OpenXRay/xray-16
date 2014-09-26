////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restrictor_wrapper.cpp
//	Created 	: 28.11.2005
//  Modified 	: 28.11.2005
//	Author		: Dmitriy Iassenev
//	Description : space restrictor wrapper
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "space_restrictor_wrapper.h"
#include "xrServer_Objects_ALife.h"
#include "level_graph.h"
#include "graph_engine.h"

IC	Fvector construct_position		(CLevelGraph &level_graph,u32 level_vertex_id, float x, float z)
{
	return							(Fvector().set(x,level_graph.vertex_plane_y(level_vertex_id,x,z),z));
}

CSpaceRestrictorWrapper::CSpaceRestrictorWrapper	(CSE_ALifeSpaceRestrictor *object)
{
	m_object						= object;
	m_level_graph					= 0;
	m_graph_engine					= 0;
	m_xform.setXYZ					(object->o_Angle);
	m_xform.c.set					(object->o_Position);
}

void CSpaceRestrictorWrapper::clear					()
{
	m_border.clear					();
	m_level_graph					= 0;
	m_graph_engine					= 0;
}

bool CSpaceRestrictorWrapper::inside				(const Fvector &position, float radius) const
{
	Fsphere							sphere;
	sphere.P						= position;
	sphere.R						= radius;

	typedef CShapeData::ShapeVec	ShapeVec;
	ShapeVec::const_iterator		I = object().shapes.begin();
	ShapeVec::const_iterator		E = object().shapes.end();
	for ( ; I != E; ++I) {
		switch ((*I).type) {
			case 0 : {
				Fsphere				temp;
				m_xform.transform_tiny(temp.P,(*I).data.sphere.P);
				temp.R				= (*I).data.sphere.R;
				if (sphere.intersect(temp))
					return			(true);

				continue;
			}
			case 1 : {
				Fmatrix				temp;
				temp.mul_43			(m_xform,(*I).data.box);

				// Build points
				Fvector				vertices;
				Fvector				points[8];
				Fplane				plane;

				vertices.set(-.5f, -.5f, -.5f);	temp.transform_tiny(points[0],vertices);
				vertices.set(-.5f, -.5f, +.5f);	temp.transform_tiny(points[1],vertices);
				vertices.set(-.5f, +.5f, +.5f);	temp.transform_tiny(points[2],vertices);
				vertices.set(-.5f, +.5f, -.5f);	temp.transform_tiny(points[3],vertices);
				vertices.set(+.5f, +.5f, +.5f);	temp.transform_tiny(points[4],vertices);
				vertices.set(+.5f, +.5f, -.5f);	temp.transform_tiny(points[5],vertices);
				vertices.set(+.5f, -.5f, +.5f);	temp.transform_tiny(points[6],vertices);
				vertices.set(+.5f, -.5f, -.5f);	temp.transform_tiny(points[7],vertices);

				plane.build(points[0],points[3],points[5]);	if (plane.classify(sphere.P)>sphere.R) break;
				plane.build(points[1],points[2],points[3]);	if (plane.classify(sphere.P)>sphere.R) break;
				plane.build(points[6],points[5],points[4]);	if (plane.classify(sphere.P)>sphere.R) break;
				plane.build(points[4],points[2],points[1]);	if (plane.classify(sphere.P)>sphere.R) break;
				plane.build(points[3],points[2],points[4]);	if (plane.classify(sphere.P)>sphere.R) break;
				plane.build(points[1],points[0],points[6]);	if (plane.classify(sphere.P)>sphere.R) break;
				return				(true);
			}
			default :				NODEFAULT;
		}
	}

	return							(false);
}

struct border_merge_predicate {
	CSpaceRestrictorWrapper			*m_restriction;
	CLevelGraph						*m_level_graph;

	IC			border_merge_predicate	(CSpaceRestrictorWrapper *restriction, CLevelGraph *level_graph)
	{
		m_restriction				= restriction;
		m_level_graph				= level_graph;
	}

	IC	void	operator()				(const CLevelGraph::CVertex &vertex) const
	{
		if (m_restriction->inside(m_level_graph->vertex_id(&vertex),true) && !m_restriction->inside(m_level_graph->vertex_id(&vertex),false))
			m_restriction->m_border.push_back	(m_level_graph->vertex_id(&vertex));

		if (m_restriction->inside(m_level_graph->vertex_id(&vertex),true))
			m_restriction->m_internal.push_back	(m_level_graph->vertex_id(&vertex));
	}

	IC	bool operator()					(const u32 &level_vertex_id) const
	{
		return						(m_restriction->inside(level_vertex_id,false));
	}
};

void CSpaceRestrictorWrapper::fill_shape			(const CShapeData::shape_def &shape)
{
	Fvector							start,dest;
	switch (shape.type) {
		case 0 : {
			start.sub				(Fvector().set(shape.data.sphere.P),Fvector().set(shape.data.sphere.R,0.f,shape.data.sphere.R));
			dest.add				(Fvector().set(shape.data.sphere.P),Fvector().set(shape.data.sphere.R,0.f,shape.data.sphere.R));
			start.add				(object().o_Position);
			dest.add				(object().o_Position);
			break;
		}
		case 1 : {
			Fvector					points[8] = {
				Fvector().set(-.5f,-.5f,-.5f),
				Fvector().set(-.5f,-.5f,+.5f),
				Fvector().set(-.5f,+.5f,-.5f),
				Fvector().set(-.5f,+.5f,+.5f),
				Fvector().set(+.5f,-.5f,-.5f),
				Fvector().set(+.5f,-.5f,+.5f),
				Fvector().set(+.5f,+.5f,-.5f),
				Fvector().set(+.5f,+.5f,+.5f)
			};
			start					= Fvector().set(flt_max,flt_max,flt_max);
			dest					= Fvector().set(flt_min,flt_min,flt_min);
			Fmatrix					Q;
			Q.mul_43				(m_xform,shape.data.box);
			Fvector					temp;
			for (int i=0; i<8; ++i) {
                Q.transform_tiny	(temp,points[i]);
				start.x				= _min(start.x,temp.x);
				start.y				= _min(start.y,temp.y);
				start.z				= _min(start.z,temp.z);
				dest.x				= _max(dest.x,temp.x);
				dest.y				= _max(dest.y,temp.y);
				dest.z				= _max(dest.z,temp.z);
			}
			break;
		}
		default : NODEFAULT;
	}
	
	level_graph().iterate_vertices	(start,dest,border_merge_predicate(this,m_level_graph));
}

bool CSpaceRestrictorWrapper::inside				(u32 level_vertex_id, bool partially_inside, float radius) const
{
	const Fvector					&position = level_graph().vertex_position(level_vertex_id);
	float							offset = level_graph().header().cell_size()*.5f - EPS_L;
	if (partially_inside)
		return						(
			inside					(construct_position(level_graph(),level_vertex_id,position.x + offset,position.z + offset),radius) || 
			inside					(construct_position(level_graph(),level_vertex_id,position.x + offset,position.z - offset),radius) ||
			inside					(construct_position(level_graph(),level_vertex_id,position.x - offset,position.z + offset),radius) || 
			inside					(construct_position(level_graph(),level_vertex_id,position.x - offset,position.z - offset),radius) ||
			inside					(Fvector().set(position.x,position.y,position.z),radius)
		);
	else
		return						(
			inside					(construct_position(level_graph(),level_vertex_id,position.x + offset,position.z + offset),radius) && 
			inside					(construct_position(level_graph(),level_vertex_id,position.x + offset,position.z - offset),radius) && 
			inside					(construct_position(level_graph(),level_vertex_id,position.x - offset,position.z + offset),radius) && 
			inside					(construct_position(level_graph(),level_vertex_id,position.x - offset,position.z - offset),radius) &&
			inside					(Fvector().set(position.x,position.y,position.z),radius)
		);
}

struct sort_by_xz_predicate {
	CLevelGraph						*m_level_graph;

	IC			sort_by_xz_predicate(CLevelGraph *level_graph)
	{
		VERIFY						(level_graph);
		m_level_graph				= level_graph;
	}

	IC	bool	operator()			(u32 v0, u32 v1) const
	{
		return						(m_level_graph->vertex(v0)->position().xz() < m_level_graph->vertex(v1)->position().xz());
	}
};

void CSpaceRestrictorWrapper::build_border			()
{
	typedef CShapeData::ShapeVec	ShapeVec;
	ShapeVec::const_iterator		I = object().shapes.begin();
	ShapeVec::const_iterator		E = object().shapes.end();
	for ( ; I != E; ++I)
		fill_shape					(*I);
	
	{
		BORDER::iterator			I = std::remove_if(m_border.begin(),m_border.end(),border_merge_predicate(this,m_level_graph));
		m_border.erase				(I,m_border.end());
	}

	{
		std::sort					(m_border.begin(),m_border.end());
		BORDER::iterator			I = std::unique(m_border.begin(),m_border.end());
		m_border.erase				(I,m_border.end());
		std::sort					(m_border.begin(),m_border.end(),sort_by_xz_predicate(m_level_graph));
	}

	VERIFY3							(!m_border.empty(),"space restrictor has no border",object().name_replace());
}

void CSpaceRestrictorWrapper::verify_connectivity	()
{
	{
		std::sort					(m_internal.begin(),m_internal.end());
		BORDER::iterator			I = std::unique(m_internal.begin(),m_internal.end());
		m_internal.erase			(I,m_internal.end());
	}

	u32								start_vertex_id = u32(-1);
	CLevelGraph::const_vertex_iterator	I = level_graph().begin();
	CLevelGraph::const_vertex_iterator	E = level_graph().end();
	for ( ; I != E; ++I)
		if (!inside(level_graph().vertex(I),true)) {
			start_vertex_id			= level_graph().vertex(I);
			break;
		}

	if (!level_graph().valid_vertex_id(start_vertex_id)) {
		Msg							("Warning : restrictor %s covers the whole AI map");
		return;
	}

	level_graph().set_mask			(m_border);
	
	xr_vector<u32>					nodes;
	
	graph_engine().search			(
		level_graph(),
		start_vertex_id,
		start_vertex_id,
		&nodes,
		GraphEngineSpace::CFlooder(
			GraphEngineSpace::_dist_type(6000),
			GraphEngineSpace::_iteration_type(-1),
			u32(-1)
		)
	);

	level_graph().clear_mask		(m_border);

	VERIFY							(nodes.size() + m_internal.size() <= level_graph().header().vertex_count());
	if (nodes.size() + m_internal.size() == level_graph().header().vertex_count())
		return;

	Msg								("! %d nodes are disconnected!",level_graph().header().vertex_count() - (nodes.size() + m_internal.size()));

	R_ASSERT3						(
		nodes.size() + m_internal.size() == level_graph().header().vertex_count(),
		"Restrictor separates AI map into several disconnected components",
		object().name_replace()
	);
}

void CSpaceRestrictorWrapper::verify				(CLevelGraph &level_graph, CGraphEngine &graph_engine, bool no_separator_check)
{
	VERIFY							(!m_level_graph);
	m_level_graph					= &level_graph;

	VERIFY							(!m_graph_engine);
	m_graph_engine					= &graph_engine;

	build_border					();

	if (!no_separator_check)
		verify_connectivity			();

	clear							();
}
