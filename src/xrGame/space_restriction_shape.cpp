////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_shape.cpp
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction shape
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "space_restriction_shape.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "space_restrictor.h"
#include "xrAICore/Navigation/graph_engine.h"

struct CBorderMergePredicate
{
    CSpaceRestrictionShape* m_restriction;

    IC CBorderMergePredicate(CSpaceRestrictionShape* restriction) { m_restriction = restriction; }
    IC void operator()(const CLevelGraph::CVertex& vertex) const
    {
        if (m_restriction->inside(ai().level_graph().vertex_id(&vertex), true) &&
            !m_restriction->inside(ai().level_graph().vertex_id(&vertex), false))
            m_restriction->m_border.push_back(ai().level_graph().vertex_id(&vertex));
    }

    IC bool operator()(u32 level_vertex_id) const { return (m_restriction->inside(level_vertex_id, false)); }
};

#ifdef DEBUG
struct CShapeTestPredicate
{
    CSpaceRestrictionShape* m_restriction;

    IC CShapeTestPredicate(CSpaceRestrictionShape* restriction) { m_restriction = restriction; }
    IC void operator()(const CLevelGraph::CVertex& vertex) const
    {
        if (m_restriction->inside(ai().level_graph().vertex_id(&vertex), false))
            m_restriction->m_test_storage.push_back(ai().level_graph().vertex_id(&vertex));
    }
};
#endif

void CSpaceRestrictionShape::fill_shape(const CCF_Shape::shape_def& shape)
{
    Fvector start, dest;
    switch (shape.type)
    {
    case 0:
    {
        start.sub(Fvector().set(shape.data.sphere.P), Fvector().set(shape.data.sphere.R, 0.f, shape.data.sphere.R));
        dest.add(Fvector().set(shape.data.sphere.P), Fvector().set(shape.data.sphere.R, 0.f, shape.data.sphere.R));
        start.add(m_restrictor->Position());
        dest.add(m_restrictor->Position());
        break;
    }
    case 1:
    {
        Fvector points[8] = {Fvector().set(-.5f, -.5f, -.5f), Fvector().set(-.5f, -.5f, +.5f),
            Fvector().set(-.5f, +.5f, -.5f), Fvector().set(-.5f, +.5f, +.5f), Fvector().set(+.5f, -.5f, -.5f),
            Fvector().set(+.5f, -.5f, +.5f), Fvector().set(+.5f, +.5f, -.5f), Fvector().set(+.5f, +.5f, +.5f)};
        start = Fvector().set(flt_max, flt_max, flt_max);
        dest = Fvector().set(flt_min, flt_min, flt_min);
        Fmatrix Q;
        Q.mul_43(m_restrictor->XFORM(), shape.data.box);
        Fvector temp;
        for (int i = 0; i < 8; ++i)
        {
            Q.transform_tiny(temp, points[i]);
            start.x = _min(start.x, temp.x);
            start.y = _min(start.y, temp.y);
            start.z = _min(start.z, temp.z);
            dest.x = _max(dest.x, temp.x);
            dest.y = _max(dest.y, temp.y);
            dest.z = _max(dest.z, temp.z);
        }
        break;
    }
    default: NODEFAULT;
    }
    ai().level_graph().iterate_vertices(start, dest, CBorderMergePredicate(this));

#ifdef DEBUG
    ai().level_graph().iterate_vertices(start, dest, CShapeTestPredicate(this));
#endif
}

void CSpaceRestrictionShape::build_border()
{
    m_border.clear();
    CCF_Shape* shape = smart_cast<CCF_Shape*>(m_restrictor->GetCForm());
    VERIFY(shape);
    xr_vector<CCF_Shape::shape_def>::const_iterator I = shape->Shapes().begin();
    xr_vector<CCF_Shape::shape_def>::const_iterator E = shape->Shapes().end();
    for (; I != E; ++I)
        fill_shape(*I);

    {
        m_border.erase(std::remove_if(m_border.begin(), m_border.end(), CBorderMergePredicate(this)), m_border.end());
    }

    process_borders();

    VERIFY3(!border().empty(), "space restrictor has no border", *m_restrictor->cName());

#ifdef DEBUG
    test_correctness();
#endif
}

#ifdef DEBUG
void CSpaceRestrictionShape::test_correctness()
{
    m_correct = true;

    if (m_test_storage.empty())
        return;

    // leave only unique nodes in m_test_storage
    std::sort(m_test_storage.begin(), m_test_storage.end());
    m_test_storage.erase(std::unique(m_test_storage.begin(), m_test_storage.end()), m_test_storage.end());

    // flood
    ai().level_graph().set_mask(border());

    xr_vector<u32> nodes;
    ai().graph_engine().search(
        ai().level_graph(), m_test_storage.back(), m_test_storage.back(), &nodes, GraphEngineSpace::CFlooder());

    ai().level_graph().clear_mask(border());

    // compare
    m_correct = (m_test_storage.size() == nodes.size());

    //////////////////////////////////////////////////////////////////////////
    // SHOW NODES THAT NOT FLOODED
    //////////////////////////////////////////////////////////////////////////

    // if (!m_correct && (xr_strcmp(*m_restrictor->cName(), "agr_factory_hold_restrictor") == 0)) {
    //	bool flood_less = m_test_storage.size() > nodes.size();

    //	Msg("NOT Correct restrictor: [%s], flood less = [%u] Dump unique nodes: ", *m_restrictor->cName(), flood_less);

    //	xr_vector<u32>::iterator src_b, src_e, tgt_b, tgt_e;
    //
    //	u32 index = 1;
    //	if (m_test_storage.size() > nodes.size()) {
    //		src_b = m_test_storage.begin();
    //		src_e = m_test_storage.end();
    //		tgt_b = nodes.begin();
    //		tgt_e = nodes.end();
    //	} else {
    //		src_b = nodes.begin();
    //		src_e = nodes.end();
    //		tgt_b = m_test_storage.begin();
    //		tgt_e = m_test_storage.end();
    //	}
    //
    //	xr_vector<u32>::iterator I = src_b;
    //	xr_vector<u32>::iterator E = src_e;

    //	for (; I != E; ++I) {
    //		xr_vector<u32>::iterator II = tgt_b;
    //		xr_vector<u32>::iterator EE = tgt_e;

    //		bool b_found = false;
    //		for (; II != EE; ++II) {
    //			if ((*I) == (*II)) {
    //				b_found = true;
    //				break;
    //			}
    //		}

    //		if (!b_found) {
    //			Msg("Node%u :: index[%u]:: position[%f,%f,%f]", index, (*I),
    // VPUSH(ai().level_graph().vertex_position((*I))));
    //			index ++;
    //		}

    //	}
    //}
}
#endif

bool CSpaceRestrictionShape::inside(const Fsphere& sphere)
{
    VERIFY(m_initialized);
    VERIFY(m_restrictor);
    return (m_restrictor->inside(sphere));
}

shared_str CSpaceRestrictionShape::name() const
{
    VERIFY(m_restrictor);
    return (m_restrictor->cName());
}

Fsphere CSpaceRestrictionShape::sphere() const
{
    Fsphere result;
    m_restrictor->Center(result.P);
    result.R = m_restrictor->Radius();
    return (result);
}
