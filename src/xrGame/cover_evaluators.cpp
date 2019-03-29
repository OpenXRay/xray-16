////////////////////////////////////////////////////////////////////////////
//	Module 		: cover_evaluators.h
//	Created 	: 24.04.2004
//  Modified 	: 24.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Cover evaluators
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "cover_evaluators.h"
#include "cover_point.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/game_graph.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "smart_cover.h"
#include "smart_cover_loophole.h"
#include "ai_debug.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_movement_manager_smart_cover.h"

float g_smart_cover_factor = 1.f;

CCoverEvaluatorBase::CCoverEvaluatorBase(CRestrictedObject* object)
    : m_best_loophole_value(0), m_loophole(nullptr), m_can_use_smart_covers(false)
{
    m_inertia_time = 0;
    m_last_update = 0;
    m_inertia_time = 0;
    m_best_value = flt_max;
    m_initialized = false;
    m_start_position.set(flt_max, flt_max, flt_max);
    m_selected = 0;
    m_previous_selected = 0;
    m_object = object;
    m_stalker = smart_cast<CAI_Stalker*>(&object->object());
    m_actuality = true;
    m_last_radius = flt_max;
    m_use_smart_covers_only = false;
}

bool CCoverEvaluatorBase::inertia(Fvector const& position, float radius)
{
    //	m_actuality				= m_actuality && fsimilar(m_last_radius,radius);
    //	m_actuality				= m_actuality && ((m_last_radius + EPS_L) >= radius);
    const bool radius_criteria = ((m_last_radius + EPS_L) >= radius);
    const bool time_criteria = (Device.dwTimeGlobal < m_last_update + m_inertia_time);

    m_last_radius = radius;

    if (time_criteria && radius_criteria)
        return true;

    if (!m_stalker)
        return false;

    smart_cover::cover const* cover = m_stalker->get_current_smart_cover();
    if (!cover)
        return false;

    smart_cover::loophole const* loophole = m_stalker->get_current_loophole();
    
    if (!loophole || !cover->is_position_in_danger_fov(*loophole, position))
        return false;

    if (!m_stalker->can_fire_right_now())
        return false;

    return true;
}

void CCoverEvaluatorBase::evaluate(CCoverPoint const* cover_point, float weight)
{
    if (!cover_point->m_is_smart_cover)
    {
        evaluate_cover(cover_point, weight);
        return;
    }

#ifndef MASTER_GOLD
    if (!psAI_Flags.test(aiUseSmartCovers))
        return;
#endif // #ifndef MASTER_GOLD

    smart_cover::cover const* tmp = static_cast<smart_cover::cover const*>(cover_point);
    if (tmp->is_combat_cover())
        evaluate_smart_cover(tmp, weight);
}

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorCloseToEnemy
//////////////////////////////////////////////////////////////////////////

void CCoverEvaluatorCloseToEnemy::evaluate_cover(const CCoverPoint* cover_point, float weight)
{
    float enemy_distance = m_enemy_position.distance_to(cover_point->position());
    // float					my_distance		= m_start_position.distance_to(cover_point->position());

    if ((enemy_distance <= m_min_distance) && (m_current_distance > enemy_distance))
        return;

    if ((enemy_distance >= m_max_distance) && (m_current_distance < enemy_distance))
        return;

    if (enemy_distance >= m_current_distance + m_deviation)
        return;

    // Fvector				direction;
    // float					y,p;
    // direction.sub			(m_enemy_position,cover_point->position());
    // direction.getHP		(y,p);
    // y						= angle_normalize(y);
    // float					cover_value = ai().level_graph().cover_in_direction(y,cover_point->level_vertex_id());
    // if (cover_value >= m_best_value)
    //	return;

    if (enemy_distance >= m_best_value)
        return;

    m_selected = cover_point;
    m_best_value = enemy_distance;
    // m_best_distance		= my_distance;
}

void CCoverEvaluatorCloseToEnemy::evaluate_smart_cover(smart_cover::cover const* smart_cover, float const& weight) {}
//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorFarFromEnemy
//////////////////////////////////////////////////////////////////////////

void CCoverEvaluatorFarFromEnemy::evaluate_cover(const CCoverPoint* cover_point, float weight)
{
    float enemy_distance = m_enemy_position.distance_to(cover_point->position());
    //	float					my_distance		= m_start_position.distance_to(cover_point->position());

    if ((enemy_distance <= m_min_distance) && (m_current_distance > enemy_distance))
        return;

    if ((enemy_distance >= m_max_distance) && (m_current_distance < enemy_distance))
        return;

    if (enemy_distance <= m_current_distance - m_deviation)
        return;

    //	Fvector					direction;
    //	float					y,p;
    //	direction.sub			(m_enemy_position,cover_point->position());
    //	direction.getHP			(y,p);
    //	y						= angle_normalize(y);
    //	float					cover_value = ai().level_graph().cover_in_direction(y,cover_point->level_vertex_id());
    if (enemy_distance <= -m_best_value)
        return;

    m_selected = cover_point;
    m_best_value = -enemy_distance;
}

void CCoverEvaluatorFarFromEnemy::evaluate_smart_cover(smart_cover::cover const* smart_cover, float const& weight) {}
//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorBest
//////////////////////////////////////////////////////////////////////////

bool CCoverEvaluatorBest::threat_on_the_way(Fvector const& cover_position) const
{
    Fvector const start_to_cover = Fvector().sub(cover_position, m_start_position);
    float const start_to_cover_magnitude = start_to_cover.magnitude();
    if (start_to_cover_magnitude < EPS_L)
        return (false);

    Fvector const start_to_threat = Fvector().sub(m_enemy_position, m_start_position);
    Fvector const start_to_cover_direction = Fvector(start_to_cover).normalize();
    float const projection = start_to_cover_direction.dotproduct(start_to_threat);
    float const start_to_threat_magnitude = start_to_threat.magnitude();
    float const cos_alpha = projection / start_to_threat_magnitude;
    float const angle = acosf(cos_alpha);
    if (angle >= PI_DIV_6)
        return (false);

    if (projection > start_to_cover_magnitude * 1.5f)
        return (false);

    return (true);
}

void CCoverEvaluatorBest::evaluate_cover(const CCoverPoint* cover_point, float weight)
{
    if (fis_zero(weight))
        return;

    if (m_use_smart_covers_only && !cover_point->m_is_smart_cover)
        return;

    float enemy_distance = m_enemy_position.distance_to(cover_point->position());

    if ((enemy_distance <= m_min_distance) && (m_current_distance > enemy_distance))
        return;

    if ((enemy_distance >= m_max_distance) && (m_current_distance < enemy_distance))
        return;

    if (threat_on_the_way(cover_point->position()))
        return;

    Fvector direction;
    float y, p;
    direction.sub(m_enemy_position, cover_point->position());
    direction.getHP(y, p);
    y = angle_normalize(y);

    float high_cover_value = ai().level_graph().high_cover_in_direction(y, cover_point->level_vertex_id());
    float low_cover_value = ai().level_graph().low_cover_in_direction(y, cover_point->level_vertex_id());
    float cover_value = std::min(high_cover_value, low_cover_value);
    float value = cover_value;
    if (ai().level_graph().neighbour_in_direction(direction, cover_point->level_vertex_id()))
        value += 10.f;

    value /= weight;

    if (value > m_best_value)
        return;

    if ((value == m_best_value) && (cover_point > m_selected))
        return;

    m_selected = cover_point;
    m_best_value = value;
    m_loophole = 0;
    //	Msg						("Loophole is NULL CCoverEvaluatorBest::evaluate_cover");
}

void CCoverEvaluatorBest::evaluate_smart_cover(smart_cover::cover const* smart_cover, float const& weight)
{
#if 0
	return;
#else
    if (!m_can_use_smart_covers)
        return;

    float value;
    smart_cover::loophole* best_loophole = smart_cover->best_loophole(
        m_enemy_position, value, false, m_stalker->movement().current_params().cover() == smart_cover);
    if (!best_loophole)
        return;

    value *= g_smart_cover_factor;
    if (value >= m_best_value)
        return;

    m_best_value = value / weight;
    m_selected = smart_cover;
    m_loophole = best_loophole;
//	Msg								("Loophole is NULL CCoverEvaluatorBest::evaluate_cover");
#endif // #if 1
}

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorAngle
//////////////////////////////////////////////////////////////////////////

void CCoverEvaluatorAngle::initialize(const Fvector& start_position, bool fake_call)
{
    inherited::initialize(start_position, fake_call);
    m_best_alpha = -1.f;
    m_direction.sub(m_start_position, m_enemy_position);
    m_direction.normalize_safe();
    float best_value = -1.f;
    float m_best_angle = 0.f;
    for (float alpha = 0.f, step = PI_MUL_2 / 360.f; alpha < PI_MUL_2; alpha += step)
    {
        float high_value = ai().level_graph().compute_high_square(alpha, PI_DIV_2, m_level_vertex_id);
        float low_value = ai().level_graph().compute_low_square(alpha, PI_DIV_2, m_level_vertex_id);
        float value = _max(high_value, low_value);
        if (value > best_value)
        {
            best_value = value;
            m_best_angle = alpha;
        }
    }
    m_best_direction.setHP(m_best_angle, 0.f);
}

void CCoverEvaluatorAngle::evaluate_cover(const CCoverPoint* cover_point, float weight)
{
    float enemy_distance = m_enemy_position.distance_to(cover_point->position());

    if ((enemy_distance <= m_min_distance) && (m_current_distance > enemy_distance))
        return;

    if ((enemy_distance >= m_max_distance) && (m_current_distance < enemy_distance))
        return;

    Fvector direction;
    direction.sub(cover_point->position(), m_enemy_position);
    direction.normalize_safe();
    float cos_a = direction.dotproduct(m_best_direction);
    if (cos_a < m_best_alpha)
        return;

    m_selected = cover_point;
    m_best_alpha = cos_a;
}

void CCoverEvaluatorAngle::evaluate_smart_cover(smart_cover::cover const* smart_cover, float const& weight) {}
//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorSafe
//////////////////////////////////////////////////////////////////////////

void CCoverEvaluatorSafe::evaluate_cover(const CCoverPoint* cover_point, float weight)
{
    if (m_start_position.distance_to(cover_point->position()) <= m_min_distance)
        return;

    float high_cover_value = ai().level_graph().vertex_high_cover(cover_point->level_vertex_id());
    float low_cover_value = ai().level_graph().vertex_low_cover(cover_point->level_vertex_id());
    float cover_value = std::min(high_cover_value, low_cover_value);
    if (cover_value >= m_best_value)
        return;

    m_selected = cover_point;
    m_best_value = cover_value;
}

void CCoverEvaluatorSafe::evaluate_smart_cover(smart_cover::cover const* smart_cover, float const& weight) {}
//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorAmbush
//////////////////////////////////////////////////////////////////////////

void CCoverEvaluatorAmbush::setup(const Fvector& my_position, const Fvector& enemy_position, float min_enemy_distance)
{
    inherited::setup();

    //	m_actuality				= m_actuality && m_my_position.similar(my_position);
    m_my_position = my_position;

    m_actuality = m_actuality && m_enemy_position.similar(enemy_position, 5.f);
    m_enemy_position = enemy_position;

    m_actuality = m_actuality && fsimilar(m_min_enemy_distance, min_enemy_distance);
    m_min_enemy_distance = min_enemy_distance;
}

void CCoverEvaluatorAmbush::evaluate_cover(const CCoverPoint* cover_point, float weight)
{
    //	float					enemy_distance = m_enemy_position.distance_to(cover_point->position());
    float my_distance = m_my_position.distance_to(cover_point->position());

    if (my_distance <= m_min_enemy_distance)
        return;

    Fvector direction;
    float y, p;

    direction.sub(m_enemy_position, cover_point->position());
    direction.getHP(y, p);
    y = angle_normalize(y);
    float high_cover_from_enemy = ai().level_graph().high_cover_in_direction(y, cover_point->level_vertex_id());
    float low_cover_from_enemy = ai().level_graph().low_cover_in_direction(y, cover_point->level_vertex_id());
    float cover_from_enemy = std::min(high_cover_from_enemy, low_cover_from_enemy);

    direction.sub(m_my_position, cover_point->position());
    direction.getHP(y, p);
    y = angle_normalize(y);
    float high_cover_from_myself = ai().level_graph().high_cover_in_direction(y, cover_point->level_vertex_id());
    float low_cover_from_myself = ai().level_graph().low_cover_in_direction(y, cover_point->level_vertex_id());
    float cover_from_myself = std::min(high_cover_from_myself, low_cover_from_myself);

    float value = cover_from_enemy / cover_from_myself;
    if (value >= m_best_value)
        return;

    m_selected = cover_point;
    m_best_value = value;
}

void CCoverEvaluatorAmbush::evaluate_smart_cover(smart_cover::cover const* smart_cover, float const& weight) {}
