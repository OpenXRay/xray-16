////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_animation_planner.h
//	Created 	: 04.09.2007
//	Author		: Alexander Dudin
//	Description : Smart cover planner class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_animation_planner.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "ai/stalker/ai_stalker_space.h"
#include "stalker_property_evaluators.h"
#include "smart_cover_planner_actions.h"
#include "smart_cover_loophole_planner_actions.h"
#include "Hit.h"
#include "smart_cover_planner_target_selector.h"
#include "stalker_animation_manager.h"
#include "stalker_movement_manager_smart_cover.h"
#include "movement_manager_space.h"
#include "property_storage.h"
#include "smart_cover_planner_actions.h"
#include "smart_cover_evaluators.h"
#include "smart_cover_animation_selector.h"
#include "clsid_game.h"
#include "game_object_space.h"

using smart_cover::animation_planner;
using namespace StalkerDecisionSpace;

animation_planner::animation_planner(CAI_Stalker* object, LPCSTR action_name)
    : inherited(), m_time_object_hit(0), m_last_transition_time(0), m_default_idle_interval(0),
      m_default_lookout_interval(0), m_loophole_value(1000), m_head_speed(flt_max), m_idle_min_time(0.f),
      m_idle_max_time(0.f), m_lookout_min_time(0.f), m_lookout_max_time(0.f), m_stay_idle(true), m_last_idle_time(0),
      m_last_lookout_time(0)
{
}

animation_planner::~animation_planner() {}
void animation_planner::setup(CAI_Stalker* object, CPropertyStorage* storage)
{
    inherited::setup(object);
#ifdef DEBUG
//	inherited::m_use_log	= true;
#endif // DEBUG

    add_evaluators();
    add_actions();

    object->movement().target_selector().setup(this, storage);
}

void animation_planner::target(StalkerDecisionSpace::EWorldProperties const& world_property)
{
    m_target.clear();
    m_target.add_condition(CWorldProperty(world_property, true));
    set_target_state(m_target);
}

void animation_planner::update() { inherited::update(); }
void animation_planner::initialize()
{
    typedef CAI_Stalker::HitCallback HitCallback;

    HitCallback hit_callback;
    hit_callback.bind(this, &animation_planner::hit_callback);
    object().hit_callback(hit_callback);

    m_head_speed = object().movement().m_head.speed;
    //	object().movement().m_head.speed	= PI_DIV_4;

    m_storage.set_property(eWorldPropertyLookedOut, false);
    m_storage.set_property(eWorldPropertyReadyToIdle, true);
    m_storage.set_property(eWorldPropertyReadyToLookout, false);
    m_storage.set_property(eWorldPropertyReadyToFire, false);
    m_storage.set_property(eWorldPropertyReadyToFireNoLookout, false);

    if (!target_state().conditions().empty())
        return;

    target(eWorldPropertyLookedOut);
}

void animation_planner::finalize()
{
    inherited::finalize();

    if (object().movement().target_selector().initialized())
        object().movement().target_selector().finalize();

    object().movement().m_head.speed = m_head_speed;
    m_initialized = false;
    typedef CAI_Stalker::HitCallback HitCallback;
    object().hit_callback(HitCallback());
}

void animation_planner::add_evaluators()
{
    add_evaluator(
        eWorldPropertySmartCoverEntered, new evaluators::cover_entered_evaluator(m_object, "smart cover entered"));
    add_evaluator(
        eWorldPropertySmartCoverActual, new evaluators::cover_actual_evaluator(m_object, "smart cover actual"));
    add_evaluator(
        eWorldPropertyReadyToKill, new CStalkerPropertyEvaluatorReadyToKillSmartCover(m_object, "ready to kill", 6));
    add_evaluator(eWorldPropertyLookedOut, new CStalkerPropertyEvaluatorConst(false, "looked out"));
    add_evaluator(
        eWorldPropertyLoopholeActual, new evaluators::loophole_actual_evaluator(m_object, "loophole actual", this, 0));
    add_evaluator(eWorldPropertyExitSmartCover, new CStalkerPropertyEvaluatorConst(false, "exit smart cover"));
    add_evaluator(eWorldPropertyLoopholeIdle, new CStalkerPropertyEvaluatorConst(false, "loophole idle"));
    add_evaluator(eWorldPropertyLoopholeFire, new CStalkerPropertyEvaluatorConst(false, "loophole fire"));
    add_evaluator(
        eWorldPropertyLoopholeFireNoLookout, new CStalkerPropertyEvaluatorConst(false, "loophole fire no lookout"));
    add_evaluator(eWorldPropertyReadyToIdle, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                                 eWorldPropertyReadyToIdle, true, true, "ready to idle"));
    add_evaluator(eWorldPropertyReadyToLookout, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                                    eWorldPropertyReadyToLookout, true, true, "ready to lookout"));
    add_evaluator(eWorldPropertyReadyToFire, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                                 eWorldPropertyReadyToFire, true, true, "ready to fire"));
    add_evaluator(eWorldPropertyReadyToFireNoLookout,
        new CStalkerPropertyEvaluatorMember(
            (CPropertyStorage*)0, eWorldPropertyReadyToFireNoLookout, true, true, "ready to fire_no_lookout"));
    add_evaluator(
        eWorldPropertyLoopholeExitable, new evaluators::loophole_exitable_evaluator(m_object, "loophole exitable"));
    add_evaluator(eWorldPropertyLoopholeCanExitWithAnimation,
        new evaluators::can_exit_loophole_with_animation(m_object, "can exit loophole with animation"));
}

void animation_planner::add_actions()
{
    action_base* action;

    action = new change_loophole(m_object, "change loophole");
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, false);
    add_condition(action, eWorldPropertyReadyToIdle, true);
    add_condition(action, eWorldPropertyLoopholeCanExitWithAnimation, true);
    add_effect(action, eWorldPropertyLoopholeActual, true);
    add_effect(action, eWorldPropertyLoopholeExitable, true);
    add_operator(eWorldOperatorChangeLoophole, action);

    action = new non_animated_change_loophole(m_object, "non-animated change loophole");
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, false);
    add_condition(action, eWorldPropertyReadyToIdle, true);
    add_condition(action, eWorldPropertyLoopholeCanExitWithAnimation, false);
    add_effect(action, eWorldPropertyLoopholeActual, true);
    add_effect(action, eWorldPropertyLoopholeExitable, true);
    add_operator(eWorldOperatorGoToLoophole, action);

    action = new exit(m_object, "exit cover");
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeExitable, true);
    add_condition(action, eWorldPropertyLoopholeCanExitWithAnimation, false);
    add_condition(action, eWorldPropertyReadyToIdle, true);
    add_effect(action, eWorldPropertySmartCoverActual, true);
    add_operator(eWorldOperatorExitSmartCover, action);

    action = new change_loophole(m_object, "animated exit");
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyReadyToIdle, true);
    add_condition(action, eWorldPropertyLoopholeExitable, true);
    add_condition(action, eWorldPropertyLoopholeCanExitWithAnimation, true);
    add_effect(action, eWorldPropertySmartCoverActual, true);
    add_operator(eWorldOperatorSmartCoverExit, action);

    action = new loophole_action_no_sight(m_object, "idle");
    add_condition(action, eWorldPropertySmartCoverActual, true);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, true);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertyLoopholeIdle, false);
    add_condition(action, eWorldPropertyReadyToIdle, true);
    add_effect(action, eWorldPropertyLoopholeIdle, true);
    add_operator(eWorldOperatorSmartCoverIdle, action);

    action = new loophole_lookout(m_object, "lookout");
    add_condition(action, eWorldPropertySmartCoverActual, true);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, true);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertyLookedOut, false);
    add_condition(action, eWorldPropertyReadyToLookout, true);
    add_effect(action, eWorldPropertyLookedOut, true);
    add_operator(eWorldOperatorSmartCoverLookout, action);

    action = new loophole_fire(m_object, "fire");
    add_condition(action, eWorldPropertySmartCoverActual, true);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, true);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertyLoopholeFire, false);
    add_condition(action, eWorldPropertyReadyToFire, true);
    add_effect(action, eWorldPropertyLoopholeFire, true);
    add_operator(eWorldOperatorSmartCoverFire, action);

    action = new loophole_reload(m_object, "reload");
    add_condition(action, eWorldPropertySmartCoverActual, true);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, true);
    add_condition(action, eWorldPropertyReadyToKill, false);
    add_condition(action, eWorldPropertyReadyToIdle, true);
    add_effect(action, eWorldPropertyReadyToKill, true);
    add_operator(eWorldOperatorSmartCoverReload, action);

    action = new loophole_fire(m_object, "fire_no_lookout");
    add_condition(action, eWorldPropertySmartCoverActual, true);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, true);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertyLoopholeFireNoLookout, false);
    add_condition(action, eWorldPropertyReadyToFireNoLookout, true);
    add_effect(action, eWorldPropertyLoopholeFireNoLookout, true);
    add_operator(eWorldOperatorSmartCoverFireNoLookout, action);

    action = new idle_2_lookout_transition(
        m_object, "idle_2_lookout", "idle", "lookout", eWorldPropertyReadyToIdle, eWorldPropertyReadyToLookout, this);
    add_condition(action, eWorldPropertySmartCoverActual, true);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, true);
    add_condition(action, eWorldPropertyReadyToIdle, true);
    add_condition(action, eWorldPropertyReadyToLookout, false);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_effect(action, eWorldPropertyReadyToLookout, true);
    add_effect(action, eWorldPropertyReadyToIdle, false);
    add_operator(eWorldOperatorSmartCoverIdle2Lookout, action);

    action = new lookout_2_idle_transition(
        m_object, "lookout_2_idle", "lookout", "idle", eWorldPropertyReadyToLookout, eWorldPropertyReadyToIdle, this);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyReadyToLookout, true);
    add_condition(action, eWorldPropertyReadyToIdle, false);
    add_effect(action, eWorldPropertyReadyToIdle, true);
    add_effect(action, eWorldPropertyReadyToLookout, false);
    add_operator(eWorldOperatorSmartCoverLookout2Idle, action);

    action = new idle_2_fire_transition(
        m_object, "idle_2_fire", "idle", "fire", eWorldPropertyReadyToIdle, eWorldPropertyReadyToFire, this, true);
    add_condition(action, eWorldPropertySmartCoverActual, true);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, true);
    add_condition(action, eWorldPropertyReadyToIdle, true);
    add_condition(action, eWorldPropertyReadyToFire, false);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_effect(action, eWorldPropertyReadyToFire, true);
    add_effect(action, eWorldPropertyReadyToIdle, false);
    add_operator(eWorldOperatorSmartCoverIdle2Fire, action);

    action = new fire_2_idle_transition(
        m_object, "fire_2_idle", "fire", "idle", eWorldPropertyReadyToFire, eWorldPropertyReadyToIdle, this);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyReadyToFire, true);
    add_condition(action, eWorldPropertyReadyToIdle, false);
    add_effect(action, eWorldPropertyReadyToIdle, true);
    add_effect(action, eWorldPropertyReadyToFire, false);
    add_operator(eWorldOperatorSmartCoverFire2Idle, action);

    action = new idle_2_fire_transition(m_object, "idle_2_fire_no_lookout", "idle", "fire_no_lookout",
        eWorldPropertyReadyToIdle, eWorldPropertyReadyToFireNoLookout, this, true);
    add_condition(action, eWorldPropertySmartCoverActual, true);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyLoopholeActual, true);
    add_condition(action, eWorldPropertyReadyToIdle, true);
    add_condition(action, eWorldPropertyReadyToFireNoLookout, false);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_effect(action, eWorldPropertyReadyToFireNoLookout, true);
    add_effect(action, eWorldPropertyReadyToIdle, false);
    add_operator(eWorldOperatorSmartCoverIdle2FireNoLookout, action);

    action = new fire_2_idle_transition(m_object, "fire_no_lookout_2_idle", "fire_no_lookout", "idle",
        eWorldPropertyReadyToFireNoLookout, eWorldPropertyReadyToIdle, this);
    add_condition(action, eWorldPropertySmartCoverEntered, true);
    add_condition(action, eWorldPropertyReadyToFireNoLookout, true);
    add_condition(action, eWorldPropertyReadyToIdle, false);
    add_effect(action, eWorldPropertyReadyToIdle, true);
    add_effect(action, eWorldPropertyReadyToFireNoLookout, false);
    add_operator(eWorldOperatorSmartCoverFireNoLookout2Idle, action);
}

bool animation_planner::hit_callback(SHit const* hit)
{
    m_time_object_hit = Device.dwTimeGlobal;

#ifndef MASTER_GOLD
    if (hit->who && smart_cast<CActor*>(hit->who) && psAI_Flags.test(aiIgnoreActor))
        return (false);
#endif // MASTER_GOLD

    if (!object().g_Alive())
        return (false);

    object().callback(GameObject::eHit)(m_object->lua_game_object(), hit->damage(), hit->direction(),
        smart_cast<const CGameObject*>(hit->who)->lua_game_object(), hit->boneID);

    return (false);
}

LPCSTR animation_planner::object_name() const { return ("animation_planner"); }
