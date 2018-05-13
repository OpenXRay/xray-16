#pragma once
#include "Include/xrRender/KinematicsAnimated.h"
#include "Actor.h"
#include "xrEngine/CameraBase.h"
#include "HUDManager.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateBloodsuckerVampireExecuteAbstract CStateBloodsuckerVampireExecute<_Object>

#define VAMPIRE_TIME_HOLD 4000
#define VAMPIRE_HIT_IMPULSE 40.f
#define VAMPIRE_MIN_DIST 0.5f
#define VAMPIRE_MAX_DIST 1.f

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::initialize()
{
    inherited::initialize();

    this->object->CControlledActor::install();

    look_head();

    m_action = eActionPrepare;
    time_vampire_started = 0;

    this->object->set_visibility_state(CAI_Bloodsucker::full_visibility);

    this->object->m_hits_before_vampire = 0;
    this->object->m_sufficient_hits_before_vampire_random = -1 + (rand() % 3);

    HUD().SetRenderable(false);
    NET_Packet P;
    Actor()->u_EventGen(P, GEG_PLAYER_WEAPON_HIDE_STATE, Actor()->ID());
    P.w_u16(INV_STATE_BLOCK_ALL);
    P.w_u8(u8(true));
    Actor()->u_EventSend(P);

    Actor()->set_inventory_disabled(true);

    m_effector_activated = false;
}

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::execute()
{
    if (!this->object->CControlledActor::is_turning() && !m_effector_activated)
    {
        this->object->ActivateVampireEffector();
        m_effector_activated = true;
    }

    look_head();

    switch (m_action)
    {
    case eActionPrepare:
        execute_vampire_prepare();
        m_action = eActionContinue;
        break;

    case eActionContinue: execute_vampire_continue(); break;

    case eActionFire:
        execute_vampire_hit();
        m_action = eActionWaitTripleEnd;
        break;

    case eActionWaitTripleEnd:
        if (!this->object->com_man().ta_is_active())
        {
            m_action = eActionCompleted;
        }

    case eActionCompleted: break;
    }

    this->object->dir().face_target(this->object->EnemyMan.get_enemy());

    Fvector const enemy_to_self = this->object->EnemyMan.get_enemy()->Position() - this->object->Position();
    float const dist_to_enemy = magnitude(enemy_to_self);
    float const vampire_dist = this->object->get_vampire_distance();

    if (angle_between_vectors(this->object->Direction(), enemy_to_self) < deg2rad(20.f) && dist_to_enemy > vampire_dist)
    {
        this->object->set_action(ACT_RUN);
        this->object->anim().accel_activate(eAT_Aggressive);
        this->object->anim().accel_set_braking(false);

        u32 const target_vertex = this->object->EnemyMan.get_enemy()->ai_location().level_vertex_id();
        Fvector const target_pos = ai().level_graph().vertex_position(target_vertex);

        this->object->path().set_target_point(target_pos, target_vertex);
        this->object->path().set_rebuild_time(100);
        this->object->path().set_use_covers(false);
        this->object->path().set_distance_to_end(vampire_dist);
    }
    else
    {
        this->object->set_action(ACT_STAND_IDLE);
    }
}

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::show_hud()
{
    HUD().SetRenderable(true);
    NET_Packet P;

    Actor()->u_EventGen(P, GEG_PLAYER_WEAPON_HIDE_STATE, Actor()->ID());
    P.w_u16(INV_STATE_BLOCK_ALL);
    P.w_u8(u8(false));
    Actor()->u_EventSend(P);
}

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::cleanup()
{
    Actor()->set_inventory_disabled(false);

    if (this->object->com_man().ta_is_active())
        this->object->com_man().ta_deactivate();

    if (this->object->CControlledActor::is_controlling())
        this->object->CControlledActor::release();

    show_hud();
}

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::finalize()
{
    inherited::finalize();
    cleanup();
}

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::critical_finalize()
{
    inherited::critical_finalize();
    cleanup();
}

TEMPLATE_SPECIALIZATION
bool CStateBloodsuckerVampireExecuteAbstract::check_start_conditions()
{
    const CEntityAlive* enemy = this->object->EnemyMan.get_enemy();

    // проверить дистанцию
    // 	float dist		= object->MeleeChecker.distance_to_enemy	(enemy);
    // 	if ((dist > VAMPIRE_MAX_DIST) || (dist < VAMPIRE_MIN_DIST))	return false;

    if (!this->object->done_enough_hits_before_vampire())
        return false;

    u32 const vertex_id = ai().level_graph().check_position_in_direction(
        this->object->ai_location().level_vertex_id(), this->object->Position(), enemy->Position());
    if (!ai().level_graph().valid_vertex_id(vertex_id))
        return false;

    if (!this->object->MeleeChecker.can_start_melee(enemy))
        return false;

    // проверить направление на врага
    if (!this->object->control().direction().is_face_target(enemy, PI_DIV_2))
        return false;

    if (!this->object->WantVampire())
        return false;

    // является ли враг актером
    if (!smart_cast<CActor const*>(enemy))
        return false;

    if (this->object->CControlledActor::is_controlling())
        return false;

    const CActor* actor = smart_cast<const CActor*>(enemy);

    VERIFY(actor);

    if (actor->input_external_handler_installed())
        return false;

    return true;
}

TEMPLATE_SPECIALIZATION
bool CStateBloodsuckerVampireExecuteAbstract::check_completion() { return (m_action == eActionCompleted); }
//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::execute_vampire_prepare()
{
    this->object->com_man().ta_activate(this->object->anim_triple_vampire);
    time_vampire_started = Device.dwTimeGlobal;

    this->object->sound().play(CAI_Bloodsucker::eVampireGrasp);
}

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::execute_vampire_continue()
{
    const CEntityAlive* enemy = this->object->EnemyMan.get_enemy();

    // if (object->Position().distance_to(Actor()->Position()) > 2.f) {
    if (!this->object->MeleeChecker.can_start_melee(enemy))
    {
        this->object->com_man().ta_deactivate();
        m_action = eActionCompleted;
        return;
    }

    this->object->sound().play(CAI_Bloodsucker::eVampireSucking);

    // проверить на грави удар
    if (time_vampire_started + VAMPIRE_TIME_HOLD < Device.dwTimeGlobal)
    {
        m_action = eActionFire;
    }
}

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::execute_vampire_hit()
{
    this->object->com_man().ta_pointbreak();
    this->object->sound().play(CAI_Bloodsucker::eVampireHit);
    this->object->SatisfyVampire();
}

//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
void CStateBloodsuckerVampireExecuteAbstract::look_head()
{
    IKinematics* pK = smart_cast<IKinematics*>(this->object->Visual());
    Fmatrix bone_transform = pK->LL_GetTransform(pK->LL_BoneID("bip01_head"));

    Fmatrix global_transform;
    global_transform.mul_43(this->object->XFORM(), bone_transform);

    this->object->CControlledActor::look_point(global_transform.c);
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateBloodsuckerVampireExecuteAbstract
