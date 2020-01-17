////////////////////////////////////////////////////////////////////////////
//	Module 		: cover_manager.h
//	Created 	: 24.03.2004
//  Modified 	: 24.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Cover manager class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrEngine/profiler.h"
#include "quadtree.h"

class CCoverPoint;

namespace LevelGraph
{
class CLevelVertex;
};

namespace smart_cover
{
class cover;
class storage;
class object;
};

namespace luabind
{
namespace adl
{
class object;
}
}

class CCoverManager
{
public:
    typedef CQuadTree<CCoverPoint> CPointQuadTree;
    typedef xr_vector<CCoverPoint*> PointVector;
    typedef smart_cover::storage Storage;
    typedef smart_cover::cover Cover;
    typedef xr_vector<Cover*> SmartCovers;

protected:
    CPointQuadTree* m_covers;
    xr_vector<bool> m_temp;
    mutable PointVector m_nearest;

private:
    Storage* m_smart_covers_storage;
    mutable SmartCovers m_smart_covers;
    mutable bool m_smart_covers_actual;

protected:
    IC bool edge_vertex(u32 index) const;
    IC bool cover(LevelGraph::CLevelVertex* v, u32 index0, u32 index1) const;
    IC bool critical_point(LevelGraph::CLevelVertex* v, u32 index, u32 index0, u32 index1) const;
    IC bool critical_cover(u32 index) const;

private:
    template <typename _evaluator_type, typename _restrictor_type>
    IC bool inertia(
        Fvector const& position, float radius, _evaluator_type& evaluator, const _restrictor_type& restrictor) const;

    static void clear_covers(PointVector& covers);
    void remove_nearby_covers(smart_cover::cover const& cover, smart_cover::object const& object) const;
    void actualize_smart_covers() const;

public:
    CCoverManager();
    virtual ~CCoverManager();
    void compute_static_cover();
    IC CPointQuadTree& covers() const;
    IC CPointQuadTree* get_covers();
    IC Storage* smart_covers_storage() const;
    void clear();
    template <typename _evaluator_type, typename _restrictor_type>
    IC const CCoverPoint* best_cover(
        const Fvector& position, float radius, _evaluator_type& evaluator, const _restrictor_type& restrictor) const;
    template <typename _evaluator_type>
    IC const CCoverPoint* best_cover(const Fvector& position, float radius, _evaluator_type& evaluator) const;
    IC bool operator()(const CCoverPoint*) const;
    IC float weight(const CCoverPoint*) const;
    IC void finalize(const CCoverPoint*) const;
    Cover const* add_smart_cover(LPCSTR table_name, smart_cover::object const& object, bool const& is_combat_cover,
        bool const& can_fire, luabind::adl::object const& loopholes) const;
    Cover* smart_cover(shared_str const& cover_id) const;
};

#include "cover_manager_inline.h"
