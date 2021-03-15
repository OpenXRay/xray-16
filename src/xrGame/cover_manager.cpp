////////////////////////////////////////////////////////////////////////////
//	Module 		: cover_manager.cpp
//	Created 	: 24.03.2004
//  Modified 	: 24.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Cover manager class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "xrCore/Threading/ParallelFor.hpp"

#include "xrAICore/Navigation/level_graph.h"
#include "cover_manager.h"
#include "ai_space.h"
#include "cover_point.h"
#include "Common/object_broker.h"
#include "smart_cover.h"
#include "smart_cover_loophole.h"
#include "smart_cover_storage.h"
#include "smart_cover_object.h"

#define MIN_COVER_VALUE 16

CCoverManager::CCoverManager()
{
    m_covers = 0;
    m_smart_covers_storage = 0;
    m_smart_covers_actual = false;
}

CCoverManager::~CCoverManager()
{
    clear();
    xr_delete(m_covers);
    xr_delete(m_smart_covers_storage);
}

IC bool CCoverManager::edge_vertex(u32 index) const
{
    CLevelGraph::CLevelVertex* v = ai().level_graph().vertex(index);
    return ((!ai().level_graph().valid_vertex_id(v->link(0)) && (v->high_cover(0) < MIN_COVER_VALUE)) ||
        (!ai().level_graph().valid_vertex_id(v->link(1)) && (v->high_cover(1) < MIN_COVER_VALUE)) ||
        (!ai().level_graph().valid_vertex_id(v->link(2)) && (v->high_cover(2) < MIN_COVER_VALUE)) ||
        (!ai().level_graph().valid_vertex_id(v->link(3)) && (v->high_cover(3) < MIN_COVER_VALUE)) ||
        (!ai().level_graph().valid_vertex_id(v->link(0)) && (v->low_cover(0) < MIN_COVER_VALUE)) ||
        (!ai().level_graph().valid_vertex_id(v->link(1)) && (v->low_cover(1) < MIN_COVER_VALUE)) ||
        (!ai().level_graph().valid_vertex_id(v->link(2)) && (v->low_cover(2) < MIN_COVER_VALUE)) ||
        (!ai().level_graph().valid_vertex_id(v->link(3)) && (v->low_cover(3) < MIN_COVER_VALUE)));
}

IC bool CCoverManager::cover(CLevelGraph::CLevelVertex* v, u32 index0, u32 index1) const
{
    return (ai().level_graph().valid_vertex_id(v->link(index0)) &&
        ai().level_graph().valid_vertex_id(ai().level_graph().vertex(v->link(index0))->link(index1)) &&
        m_temp[ai().level_graph().vertex(v->link(index0))->link(index1)]);
}

IC bool CCoverManager::critical_point(CLevelGraph::CLevelVertex* v, u32 index, u32 index0, u32 index1) const
{
    return (!ai().level_graph().valid_vertex_id(v->link(index)) &&
        (!ai().level_graph().valid_vertex_id(v->link(index0)) || !ai().level_graph().valid_vertex_id(v->link(index1)) ||
            cover(v, index0, index) || cover(v, index1, index)));
}

IC bool CCoverManager::critical_cover(u32 index) const
{
    CLevelGraph::CLevelVertex* v = ai().level_graph().vertex(index);
    return (critical_point(v, 0, 1, 3) || critical_point(v, 2, 1, 3) || critical_point(v, 1, 0, 2) ||
        critical_point(v, 3, 0, 2));
}

void CCoverManager::compute_static_cover()
{
    clear();
    xr_delete(m_covers);
    m_covers = xr_new<CPointQuadTree>(
        ai().level_graph().header().box(), ai().level_graph().header().cell_size() * .5f, 8 * 65536, 4 * 65536);
    m_temp.resize(ai().level_graph().header().vertex_count());

    const CLevelGraph& graph = ai().level_graph();
    const u32 levelVertexCount = ai().level_graph().header().vertex_count();

    xr_parallel_for(TaskRange<u32>(0, levelVertexCount), [&](const TaskRange<u32>& range)
    {
        for (u32 i = range.begin(); i != range.end(); ++i)
        {
            const CLevelGraph::CLevelVertex& vertex = *graph.vertex(i);
            if (vertex.high_cover(0) + vertex.high_cover(1) + vertex.high_cover(2) + vertex.high_cover(3))
            {
                m_temp[i] = edge_vertex(i);
                continue;
            }

            if (vertex.low_cover(0) + vertex.low_cover(1) + vertex.low_cover(2) + vertex.low_cover(3))
            {
                m_temp[i] = edge_vertex(i);
                continue;
            }

            m_temp[i] = false;
        }
    });

    for (u32 i = 0; i < levelVertexCount; ++i)
        if (m_temp[i] && critical_cover(i))
            m_covers->insert(xr_new<CCoverPoint>(ai().level_graph().vertex_position(ai().level_graph().vertex(i)), i));

    VERIFY(!m_smart_covers_storage);
    m_smart_covers_storage = xr_new<smart_cover::storage>();
}

void CCoverManager::clear_covers(PointVector& covers)
{
    PointVector::iterator I = covers.begin();
    PointVector::iterator E = covers.end();
    for (; I != E; ++I)
    {
        if (!(*I)->m_is_smart_cover)
        {
            xr_delete(*I);
            continue;
        }

        smart_cover::cover* cover = (smart_cover::cover*)*I;
        xr_delete(cover);
    }

    covers.clear();
}

void CCoverManager::clear()
{
    if (!get_covers())
        return;

    covers().all(m_nearest);
    clear_covers(m_nearest);
    m_covers->clear();
    xr_delete(m_smart_covers_storage);
    m_smart_covers.clear();
}

namespace smart_cover
{
struct predicate
{
    object const* m_object;

    IC predicate(object const& object) : m_object(&object) {}
    IC bool operator()(CCoverPoint* const& cover) const
    {
        if (cover->m_is_smart_cover)
            return (true);

        return (!m_object->inside(cover->position()));
    }
};
} // namespace smart_cover

void CCoverManager::remove_nearby_covers(smart_cover::cover const& cover, smart_cover::object const& object) const
{
    m_nearest.clear();
    for (const smart_cover::loophole* loophole : cover.loopholes())
    {
        Fvector position = cover.fov_position(*loophole);
        m_covers->nearest(position, object.Radius() + 1.f, m_nearest);

        m_nearest.erase(
            std::remove_if(m_nearest.begin(), m_nearest.end(), smart_cover::predicate(object)), m_nearest.end());

        typedef PointVector::const_iterator const_iterator;
        const_iterator i = m_nearest.begin();
        const_iterator e = m_nearest.end();
        for (; i != e; ++i)
            m_covers->remove(*i);

        clear_covers(m_nearest);
    }
}

CCoverManager::Cover const* CCoverManager::add_smart_cover(LPCSTR table_name, smart_cover::object const& object,
    bool const& is_combat_cover, bool const& can_fire, luabind::adl::object const& loopholes) const
{
    Cover* smart_cover =
        xr_new<Cover>(object, m_smart_covers_storage->description(table_name), is_combat_cover, can_fire, loopholes);

    remove_nearby_covers(*smart_cover, object);
    m_covers->insert(smart_cover);

    m_smart_covers.push_back(smart_cover);
    m_smart_covers_actual = false;

    return (smart_cover);
}

class id_predicate_less
{
private:
    typedef smart_cover::cover Cover;

public:
    IC bool operator()(Cover* lhs, Cover* rhs)
    {
        VERIFY(lhs);
        VERIFY(rhs);
        return (lhs->get_object().cName()._get() < rhs->get_object().cName()._get());
    }
    IC bool operator()(Cover* cover, shared_str const& id)
    {
        VERIFY(cover);
        return (cover->get_object().cName()._get() < id._get());
    }
};

void CCoverManager::actualize_smart_covers() const
{
    std::sort(m_smart_covers.begin(), m_smart_covers.end(), id_predicate_less());

    m_smart_covers_actual = true;
}

CCoverManager::Cover* CCoverManager::smart_cover(shared_str const& cover_id) const
{
    if (!m_smart_covers_actual)
        actualize_smart_covers();

    SmartCovers::iterator found =
        std::lower_bound(m_smart_covers.begin(), m_smart_covers.end(), cover_id, id_predicate_less());

    VERIFY2(((found != m_smart_covers.end()) && ((*found)->id()._get() == cover_id._get())),
        make_string("smart_cover [%s] not found", cover_id.c_str()));

    return (*found);
}
