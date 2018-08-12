#include "StdAfx.h"
#include "corpse_cover.h"
#include "cover_point.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"

void CMonsterCorpseCoverEvaluator::evaluate_cover(const CCoverPoint* cover_point, float weight)
{
    float my_distance = m_start_position.distance_to(cover_point->position());

    if (my_distance <= m_min_distance)
        return;

    if (my_distance >= m_max_distance)
        return;

    Fvector direction;
    float y, p;
    direction.sub(m_start_position, cover_point->position());
    direction.getHP(y, p);

    float high_cover_value = ai().level_graph().high_cover_in_direction(y, cover_point->level_vertex_id());
    float low_cover_value = ai().level_graph().low_cover_in_direction(y, cover_point->level_vertex_id());
    float cover_value = std::min(high_cover_value, low_cover_value);
    if (cover_value >= 2.f * m_best_value)
        return;

    m_selected = cover_point;
    m_best_value = cover_value;
}

void CMonsterCorpseCoverEvaluator::evaluate_smart_cover(smart_cover::cover const* smart_cover, float const& weight) {}
