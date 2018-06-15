#include "stdafx.h"
#include "monster_cover_manager.h"
#include "BaseMonster/base_monster.h"
#include "cover_evaluators.h"
#include "cover_point.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/game_graph.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "Level.h"
#include "level_debug.h"
#include "cover_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "ai_monster_squad.h"
#include "ai_monster_squad_manager.h"
#include <functional>

//////////////////////////////////////////////////////////////////////////
// CControllerCoverEvaluator
class CCoverEvaluator : public CCoverEvaluatorBase
{
    typedef CCoverEvaluatorBase inherited;

    Fvector m_dest_position;
    float m_min_distance;
    float m_max_distance;
    float m_current_distance;
    float m_deviation;
    float m_best_distance;

    CBaseMonster* m_object;

public:
    CCoverEvaluator(CRestrictedObject* object);

    // setup by cover_manager
    void initialize(const Fvector& start_position);

    // manual setup
    void setup(CBaseMonster* object, const Fvector& position, float min_pos_distance, float max_pos_distance,
        float deviation = 0.f);

    virtual void evaluate_cover(const CCoverPoint* cover_point, float weight);
    virtual void evaluate_smart_cover(smart_cover::cover const* smart_cover, float const& weight);
};

//////////////////////////////////////////////////////////////////////////
// CControllerCoverPredicate
class CCoverPredicate
{
public:
    // setup internals here
    CCoverPredicate();
    // called from cover_manager for every cover (for suitable cover)
    bool operator()(const CCoverPoint* cover) const { return true; }
    // must return a value that is transfered to cover evaluator
    float weight(const CCoverPoint* cover) const { return 1.f; }
    void finalize(const CCoverPoint* cover) const {}
};

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluator Implementation
//////////////////////////////////////////////////////////////////////////

CCoverEvaluator::CCoverEvaluator(CRestrictedObject* object) : inherited(object), m_object(nullptr)
{
    m_dest_position.set(flt_max, flt_max, flt_max);
    m_deviation = flt_max;
    m_min_distance = flt_max;
    m_max_distance = flt_max;
    m_best_distance = flt_max;
    m_current_distance = flt_max;
}

void CCoverEvaluator::setup(
    CBaseMonster* object, const Fvector& position, float min_pos_distance, float max_pos_distance, float deviation)
{
    inherited::setup();

    m_object = object;

    m_dest_position = position;

    m_actuality = m_actuality && fsimilar(m_deviation, deviation);
    m_deviation = deviation;

    m_actuality = m_actuality && fsimilar(m_min_distance, min_pos_distance);
    m_min_distance = min_pos_distance;

    m_actuality = m_actuality && fsimilar(m_max_distance, max_pos_distance);
    m_max_distance = max_pos_distance;
}

void CCoverEvaluator::initialize(const Fvector& start_position)
{
    inherited::initialize(start_position);
    m_current_distance = m_start_position.distance_to(m_dest_position);
#ifdef DEBUG
    DBG().level_info(this).clear();
#endif
}

void CCoverEvaluator::evaluate_cover(const CCoverPoint* cover_point, float weight)
{
#ifdef DEBUG
// DBG().level_info(this).add_item(cover_point->position(), color_xrgb(0,255,0));
#endif
    CMonsterSquad* squad = monster_squad().get_squad(m_object);
    if (squad->is_locked_cover(cover_point->level_vertex_id()))
        return;

    if (fis_zero(weight))
        return;

    float dest_distance = m_dest_position.distance_to(cover_point->position());

    if ((dest_distance <= m_min_distance) && (m_current_distance > dest_distance))
        return;

    if ((dest_distance >= m_max_distance) && (m_current_distance < dest_distance))
        return;

    Fvector direction;
    float y, p;
    direction.sub(m_dest_position, cover_point->position());
    direction.getHP(y, p);

    float high_cover_value = ai().level_graph().high_cover_in_direction(y, cover_point->level_vertex_id());
    float low_cover_value = ai().level_graph().low_cover_in_direction(y, cover_point->level_vertex_id());
    float cover_value = std::min(high_cover_value, low_cover_value);
    float value = cover_value;
    if (ai().level_graph().neighbour_in_direction(direction, cover_point->level_vertex_id()))
        value += 10.f;

    value /= weight;

    if (value >= m_best_value)
        return;

    m_selected = cover_point;
    m_best_value = value;
}

void CCoverEvaluator::evaluate_smart_cover(smart_cover::cover const* smart_cover, float const& weight) {}
//=============================================================================
// Cover Manager
//=============================================================================

CMonsterCoverManager::CMonsterCoverManager(CBaseMonster* monster) : m_object(monster) { m_ce_best = 0; }
CMonsterCoverManager::~CMonsterCoverManager() { xr_delete(m_ce_best); }
void CMonsterCoverManager::load()
{
    m_ce_best = new CCoverEvaluator(&(m_object->control().path_builder().restrictions()));
}

const CCoverPoint* CMonsterCoverManager::find_cover(
    const Fvector& position, float min_pos_distance, float max_pos_distance, float deviation)
{
    m_ce_best->setup(m_object, position, min_pos_distance, max_pos_distance, deviation);
    const CCoverPoint* point = ai().cover_manager().best_cover(m_object->Position(), 30.f, *m_ce_best);

    return point;
}

// найти лучший ковер относительно "position"
const CCoverPoint* CMonsterCoverManager::find_cover(
    const Fvector& src_pos, const Fvector& dest_pos, float min_pos_distance, float max_pos_distance, float deviation)
{
    m_ce_best->setup(m_object, dest_pos, min_pos_distance, max_pos_distance, deviation);
    const CCoverPoint* point = ai().cover_manager().best_cover(src_pos, 30.f, *m_ce_best);
    return point;
}

//////////////////////////////////////////////////////////////////////////
// Find Less Cover Direction (e.g. look at the most open place)
//////////////////////////////////////////////////////////////////////////

#define ANGLE_DISP PI_DIV_2
#define ANGLE_DISP_STEP deg(10)
#define TRACE_STATIC_DIST 3.f

void CMonsterCoverManager::less_cover_direction(Fvector& dir)
{
    float angle = ai().level_graph().vertex_high_cover_angle(
        m_object->ai_location().level_vertex_id(), deg(10), std::greater<float>());

    collide::rq_result l_rq;

    float angle_from = angle_normalize(angle - ANGLE_DISP);
    float angle_to = angle_normalize(angle + ANGLE_DISP);

    Fvector trace_from;
    m_object->Center(trace_from);
    Fvector direction;

    // trace discretely left
    for (float ang = angle; angle_difference(ang, angle) < ANGLE_DISP; ang = angle_normalize(ang - ANGLE_DISP_STEP))
    {
        direction.setHP(ang, 0.f);

        if (Level().ObjectSpace.RayPick(trace_from, direction, TRACE_STATIC_DIST, collide::rqtStatic, l_rq, m_object))
        {
            if ((l_rq.range < TRACE_STATIC_DIST))
            {
                angle_from = ang;
                break;
            }
        }
    }

    // trace discretely right
    for (float ang = angle; angle_difference(ang, angle) < ANGLE_DISP; ang = angle_normalize(ang + ANGLE_DISP_STEP))
    {
        direction.setHP(ang, 0.f);

        if (Level().ObjectSpace.RayPick(trace_from, direction, TRACE_STATIC_DIST, collide::rqtStatic, l_rq, m_object))
        {
            if ((l_rq.range < TRACE_STATIC_DIST))
            {
                angle_to = ang;
                break;
            }
        }
    }

    angle = angle_normalize(angle_from + angle_difference(angle_from, angle_to) / 2);
    dir.setHP(angle, 0.f);
}
//////////////////////////////////////////////////////////////////////////
