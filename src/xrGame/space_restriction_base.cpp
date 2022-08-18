////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_base.cpp
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction base
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "space_restriction_base.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"

bool CSpaceRestrictionBase::inside(u32 level_vertex_id, bool partially_inside)
{
    constexpr auto DEFAULT_RADIUS = EPS_L;

    return (inside(level_vertex_id, partially_inside, DEFAULT_RADIUS));
}

bool CSpaceRestrictionBase::inside(u32 level_vertex_id, bool partially_inside, float radius)
{
    constexpr auto construct_position = [](u32 level_vertex_id, float x, float z) -> Fvector
    {
        return { x, ai().level_graph().vertex_plane_y(level_vertex_id, x, z), z };
    };

    const float offset = ai().level_graph().header().cell_size() * .5f - EPS_L;
    const auto& [x, y, z] = ai().level_graph().vertex_position(level_vertex_id);

    if (partially_inside)
    {
        return inside(Fsphere{ construct_position(level_vertex_id, x + offset, z + offset), radius })
            || inside(Fsphere{ construct_position(level_vertex_id, x + offset, z - offset), radius })
            || inside(Fsphere{ construct_position(level_vertex_id, x - offset, z + offset), radius })
            || inside(Fsphere{ construct_position(level_vertex_id, x - offset, z - offset), radius })
            || inside(Fsphere{ { x, y, z }, radius });
    }
    return inside(Fsphere{ construct_position(level_vertex_id, x + offset, z + offset), radius })
        && inside(Fsphere{ construct_position(level_vertex_id, x + offset, z - offset), radius })
        && inside(Fsphere{ construct_position(level_vertex_id, x - offset, z + offset), radius })
        && inside(Fsphere{ construct_position(level_vertex_id, x - offset, z - offset), radius })
        && inside(Fsphere{ { x, y, z }, radius });
}

void CSpaceRestrictionBase::process_borders()
{
    std::sort(m_border.begin(), m_border.end());
    m_border.erase(std::unique(m_border.begin(), m_border.end()), m_border.end());
    std::sort(m_border.begin(), m_border.end(), [](u32 v0, u32 v1)
    {
        return ai().level_graph().vertex(v0)->position().xz() < ai().level_graph().vertex(v1)->position().xz();
    });
}
