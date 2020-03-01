////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_manager.cpp
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Sight manager
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "sight_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_movement_manager_smart_cover.h"
#include "xrEngine/profiler.h"
#include "aimers_weapon.h"
#include "aimers_bone.h"
#include "stalker_animation_manager.h"
#include "Weapon.h"

using MonsterSpace::SBoneRotation;

//												head  shoulder  spine
static Fvector const s_danger_factors = {.0f, .50f, .50f};
static Fvector const s_free_factors = {.25f, .25f, .50f};
static float const s_factor_lerp_speed = 1.f;

//#define SIGHT_DEBUG

CSightManager::CSightManager(CAI_Stalker* object)
    : inherited(object), m_enabled(true), m_turning_in_place(false), m_aiming_type(aiming_none)
{
}

void CSightManager::Load(LPCSTR section) {}
void CSightManager::reinit()
{
    inherited::reinit();
    m_enabled = true;
    m_turning_in_place = false;

    VERIFY(_valid(s_free_factors));
    m_current.m_head.m_factor = s_free_factors.x;
    m_current.m_shoulder.m_factor = s_free_factors.y;
    m_current.m_spine.m_factor = s_free_factors.z;
}

void CSightManager::reload(LPCSTR section)
{
    m_max_left_angle = deg2rad(READ_IF_EXISTS(pSettings, r_float, section, "max_left_torso_angle", 90.f));
    m_max_right_angle = deg2rad(READ_IF_EXISTS(pSettings, r_float, section, "max_right_torso_angle", 60.f));
}

void CSightManager::vfValidateAngleDependency(float x1, float& x2, float x3)
{
    float const _x2 = angle_normalize_signed(x2 - x1);
    float const _x3 = angle_normalize_signed(x3 - x1);
    if (_x2 * _x3 >= 0.f)
        return;

    if (_abs(_x2) + _abs(_x3) <= PI)
        return;

    x2 = x3;
}

#ifdef DEBUG
BOOL g_ai_dbg_sight = 0;
#endif // #ifdef DEBUG

float g_ai_aim_min_speed = PI_DIV_8 / 2.f;
float g_ai_aim_min_angle = PI_DIV_8 / 2.f;
float g_ai_aim_max_angle = PI_DIV_4;
BOOL g_ai_aim_use_smooth_aim = 1;

static inline float select_speed(
    float const distance, float const speed, float const min_speed, float const min_distance, float const max_distance)
{
    VERIFY(max_distance > min_distance);

#ifdef DEBUG
    if (!g_ai_aim_use_smooth_aim)
        return speed;
#endif // #ifdef DEBUG

    if (speed <= min_speed)
        return speed;

    if (distance < min_distance)
        return min_speed;

    if (distance >= max_distance)
        return speed;

    float const factor = (distance - min_distance) / (max_distance - min_distance);
    return min_speed + factor * (speed - min_speed);
}

void CSightManager::Exec_Look(float time_delta)
{
    START_PROFILE("Sight Manager")

    SBoneRotation& body = object().movement().m_body;
    SBoneRotation& head = object().movement().m_head;

    if (object().animation_movement_controlled())
        body.target = body.current;

    // normalizing torso angles
    body.current.yaw = angle_normalize_signed(body.current.yaw);
    body.current.pitch = angle_normalize_signed(body.current.pitch);
    body.target.yaw = angle_normalize_signed(body.target.yaw);
    body.target.pitch = angle_normalize_signed(body.target.pitch);

    // normalizing head angles
    head.current.yaw = angle_normalize_signed(head.current.yaw);
    head.current.pitch = angle_normalize_signed(head.current.pitch);
    head.target.yaw = angle_normalize_signed(head.target.yaw);
    head.target.pitch = angle_normalize_signed(head.target.pitch);

    float body_speed = body.speed;
    if (current_action().change_body_speed())
        body_speed = current_action().body_speed();

    float head_speed = head.speed;
    if (current_action().change_head_speed())
        head_speed = current_action().head_speed();

#ifdef SIGHT_DEBUG
    if (object().cName() == "level_prefix_stalker")
    {
        Msg("[%6d][%s] BEFORE BODY [%f] -> [%f]", Device.dwTimeGlobal, object().cName().c_str(),
            object().movement().m_body.current.yaw, object().movement().m_body.target.yaw);
        Msg("[%6d][%s] BEFORE HEAD [%f] -> [%f]", Device.dwTimeGlobal, object().cName().c_str(),
            object().movement().m_head.current.yaw, object().movement().m_head.target.yaw);
    }
#endif // #ifdef SIGHT_DEBUG

// static CStatGraph* s_stats_graph	= 0;
// if ( !s_stats_graph ) {
//	s_stats_graph					= new CStatGraph();
//	s_stats_graph->SetRect			(0, 1024-68, 1280, 68, 0xff000000, 0xff000000);
//	s_stats_graph->SetMinMax		(-PI, PI, 1000);
//	s_stats_graph->SetStyle			(CStatGraph::stBarLine);
//	s_stats_graph->AppendSubGraph	(CStatGraph::stCurve);
//	s_stats_graph->AppendSubGraph	(CStatGraph::stCurve);
//}

// s_stats_graph->AppendItem			( angle_normalize_signed(head.current.yaw),   0xff00ff00, 0 );
// s_stats_graph->AppendItem			( angle_normalize_signed(head.current.pitch), 0xffff0000, 1 );

#ifdef DEBUG
    if (g_ai_dbg_sight)
        Msg("%6d [%s] before body[%f]->[%f], head[%f]->[%f]", Device.dwTimeGlobal, object().cName().c_str(),
            body.current.yaw, body.target.yaw, head.current.yaw, head.target.yaw);
#endif // #ifdef DEBUG
    vfValidateAngleDependency(body.current.yaw, body.target.yaw, head.current.yaw);
#ifdef DEBUG
    if (g_ai_dbg_sight)
        Msg("%6d [%s] after  body[%f]->[%f], head[%f]->[%f]", Device.dwTimeGlobal, object().cName().c_str(),
            body.current.yaw, body.target.yaw, head.current.yaw, head.target.yaw);
#endif // #ifdef DEBUG

    m_object->angle_lerp_bounds(body.current.yaw, body.target.yaw,
        select_speed(angle_difference(body.current.yaw, body.target.yaw), body_speed, g_ai_aim_min_speed,
            g_ai_aim_min_angle, g_ai_aim_max_angle),
        time_delta);
    m_object->angle_lerp_bounds(body.current.pitch, body.target.pitch,
        select_speed(angle_difference(body.current.pitch, body.target.pitch), body_speed, g_ai_aim_min_speed,
            g_ai_aim_min_angle, g_ai_aim_max_angle),
        time_delta);

    m_object->angle_lerp_bounds(head.current.yaw, head.target.yaw,
        select_speed(angle_difference(head.current.yaw, head.target.yaw), head_speed, g_ai_aim_min_speed,
            g_ai_aim_min_angle, g_ai_aim_max_angle),
        time_delta);
    m_object->angle_lerp_bounds(head.current.pitch, head.target.pitch,
        select_speed(angle_difference(head.current.pitch, head.target.pitch), head_speed, g_ai_aim_min_speed,
            g_ai_aim_min_angle, g_ai_aim_max_angle),
        time_delta);

#ifdef DEBUG
    if (g_ai_dbg_sight)
        Msg("%6d [%s] after2 body[%f]->[%f], head[%f]->[%f]", Device.dwTimeGlobal, object().cName().c_str(),
            body.current.yaw, body.target.yaw, head.current.yaw, head.target.yaw);
#endif // #ifdef DEBUG

#ifdef SIGHT_DEBUG
    // normalizing torso angles
    body.current.yaw = angle_normalize_signed(body.current.yaw);
    body.current.pitch = angle_normalize_signed(body.current.pitch);

    // normalizing head angles
    head.current.yaw = angle_normalize_signed(head.current.yaw);
    head.current.pitch = angle_normalize_signed(head.current.pitch);

    if (object().cName() == "level_prefix_stalker")
    {
        Msg("[%6d][%s] AFTER  BODY [%f] -> [%f]", Device.dwTimeGlobal, object().cName().c_str(),
            object().movement().m_body.current.yaw, object().movement().m_body.target.yaw);
        Msg("[%6d][%s] AFTER  HEAD [%f][%f] -> [%f][%f]", Device.dwTimeGlobal, object().cName().c_str(),
            object().movement().m_head.current.yaw, object().movement().m_head.current.pitch,
            object().movement().m_head.target.yaw, object().movement().m_head.target.pitch);
    }
#endif // #ifdef SIGHT_DEBUG

    if (enabled())
    {
        compute_aiming(time_delta, head_speed);
        current_action().on_frame();
    }

#ifdef DEBUG
    if (g_ai_dbg_sight)
        Msg("%6d [%s] after3 body[%f]->[%f], head[%f]->[%f]", Device.dwTimeGlobal, object().cName().c_str(),
            body.current.yaw, body.target.yaw, head.current.yaw, head.target.yaw);
#endif // #ifdef DEBUG

    if (object().animation_movement_controlled())
        return;

    Fmatrix& m = m_object->XFORM();
    float h = -body.current.yaw;
    float _sh = _sin(h), _ch = _cos(h);
    m.i.set(_ch, 0.f, _sh);
    m._14_ = 0.f;
    m.j.set(0.f, 1.f, 0.f);
    m._24_ = 0.f;
    m.k.set(-_sh, 0.f, _ch);
    m._34_ = 0.f;

    STOP_PROFILE
}

void CSightManager::setup(const CSightAction& sight_action)
{
    if (m_actions.size() > 1)
        clear();

    if (!m_actions.empty() && (*(*m_actions.begin()).second == sight_action))
        return;

    clear();
    add_action(0, new CSightControlAction(1.f, u32(-1), sight_action));
}

void CSightManager::update()
{
    START_PROFILE("Sight Manager")

    if (!enabled())
        return;

    if (!fis_zero(object().movement().speed()))
    {
        m_turning_in_place = false;
        inherited::update();
        return;
    }

    if (!m_turning_in_place)
    {
        if (angle_difference(object().movement().m_body.current.yaw, object().movement().m_head.current.yaw) >
            (left_angle(-object().movement().m_head.current.yaw, -object().movement().m_body.current.yaw) ?
                    m_max_left_angle :
                    m_max_right_angle))
        {
            m_turning_in_place = true;
            //			Msg				("%6d started turning in place",Device.dwTimeGlobal);
            object().movement().m_body.target.yaw = object().movement().m_head.current.yaw;
        }
        else
            object().movement().m_body.target.yaw = object().movement().m_body.current.yaw;

        inherited::update();
        return;
    }

    if (angle_difference(object().movement().m_body.current.yaw, object().movement().m_head.target.yaw) > EPS_L)
    {
        //		object().movement().m_body.target.yaw	= object().movement().m_head.current.yaw;
        object().movement().m_body.target.yaw = object().movement().m_head.target.yaw;
    }
    else
    {
        m_turning_in_place = false;
        //		Msg					("%6d stopped turning in place",Device.dwTimeGlobal);
        object().movement().m_body.target.yaw = object().movement().m_body.current.yaw;
    }

    inherited::update();

    STOP_PROFILE
}

void CSightManager::remove_links(IGameObject* object)
{
    setup_actions::iterator I = actions().begin();
    setup_actions::iterator E = actions().end();
    for (; I != E; ++I)
        (*I).second->remove_links(object);
}

Fvector CSightManager::object_position() const
{
    CGameObject const* object = &current_action().object();
    Fvector look_pos;
    object->Center(look_pos);

    const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>(object);
    if (!entity_alive || entity_alive->g_Alive())
    {
        look_pos.x = object->Position().x;
        look_pos.z = object->Position().z;
    }

    Fvector my_position = m_object->eye_matrix.c;
    Fvector target = look_pos;
    if (!aim_target(my_position, target, object))
        target = look_pos;

    return (target);
}

// CActor*			Actor()	;

Fvector CSightManager::aiming_position() const
{
    Fvector result;

#if 0
    Fmatrix								player_head;
    IKinematics* actor_kinematics		= smart_cast<IKinematics*>(Actor()->Visual());
    actor_kinematics->Bone_GetAnimPos	(player_head, actor_kinematics->LL_BoneID("bip01_head"), 1, false);
    player_head.mulA_43					(Actor()->XFORM());
    return								( player_head.c );
#endif // #if 0

#ifdef DEBUG
    result.set(flt_max, flt_max, flt_max);
#endif // #ifdef DEBUG

    float const fake_distance = 10000.f;

    using namespace SightManager;

    switch (current_action().sight_type())
    {
    case eSightTypeCurrentDirection:
    {
        VERIFY2(_valid(object().Position()), make_string("[%f][%f][%f]", VPUSH(object().Position())));
        VERIFY2(_valid(-object().movement().m_head.current.yaw),
            make_string("%f", -object().movement().m_head.current.yaw));
        VERIFY2(_valid(-object().movement().m_head.current.yaw),
            make_string("%f", -object().movement().m_head.current.pitch));
        VERIFY(_valid(
            Fvector().setHP(-object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch)));
        result.mad(object().Position(),
            Fvector().setHP(-object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch),
            fake_distance);

        VERIFY2(result.magnitude() < 100000.f,
            make_string("[%f][%f][%f] [%f][%f] [%f]", VPUSH(object().Position()),
                -object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch, fake_distance));
        VERIFY(_valid(result));
        break;
    }
    case eSightTypePathDirection:
    {
        result.mad(object().Position(),
            Fvector().setHP(-object().movement().m_head.target.yaw, -object().movement().m_head.target.pitch),
            fake_distance);
        VERIFY2(result.magnitude() < 100000.f,
            make_string("[%f][%f][%f] [%f][%f] [%f]", VPUSH(object().Position()),
                -object().movement().m_head.target.yaw, -object().movement().m_head.target.pitch, fake_distance));
        VERIFY(_valid(result));
        break;
    }
    case eSightTypeDirection:
    {
        VERIFY(_valid(current_action().vector3d()));
        result.mad(object().Position(), current_action().vector3d(), fake_distance);
        VERIFY2(result.magnitude() < 100000.f, make_string("[%f][%f][%f] [%f][%f][%f] [%f]", VPUSH(object().Position()),
                                                   VPUSH(current_action().vector3d()), fake_distance));
        VERIFY(_valid(result));
        break;
    }
    case eSightTypePosition:
    case eSightTypeFirePosition:
    {
        result = current_action().vector3d();
        VERIFY2(result.magnitude() < 100000.f, make_string("[%f][%f][%f]", VPUSH(current_action().vector3d())));
        VERIFY(_valid(current_action().vector3d()));
        break;
    }
    case eSightTypeObject:
    {
        result = object_position();
        VERIFY2(result.magnitude() < 100000.f, make_string("[%f][%f][%f]", VPUSH(result)));
        VERIFY(_valid(result));
        break;
    }
    case eSightTypeFireObject:
    {
        switch (current_action().state_fire_object())
        {
        case 0:
        {
            result = current_action().vector3d(); // object_position();
            VERIFY2(result.magnitude() < 100000.f, make_string("[%f][%f][%f]", VPUSH(result)));
            VERIFY(_valid(result));
            break;
        }
        case 1:
        {
            result = current_action().vector3d();
            VERIFY2(result.magnitude() < 100000.f, make_string("[%f][%f][%f]", VPUSH(result)));
            VERIFY(_valid(result));
            break;
        }
        default: NODEFAULT;
        }
        break;
    }
    case eSightTypeCover:
    {
        result.mad(object().Position(),
            Fvector().setHP(-object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch),
            fake_distance);
        VERIFY2(result.magnitude() < 100000.f,
            make_string("[%f][%f][%f] [%f][%f] [%f]", VPUSH(object().Position()),
                -object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch, fake_distance));
        VERIFY(_valid(result));
        break;
    }
    case eSightTypeSearch:
    {
        result.mad(object().Position(),
            Fvector().setHP(-object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch),
            fake_distance);
        VERIFY2(result.magnitude() < 100000.f,
            make_string("[%f][%f][%f] [%f][%f] [%f]", VPUSH(object().Position()),
                -object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch, fake_distance));
        VERIFY(_valid(result));
        break;
    }
    case eSightTypeLookOver:
    {
        result.mad(object().Position(),
            Fvector().setHP(-object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch),
            fake_distance);
        VERIFY2(result.magnitude() < 100000.f,
            make_string("[%f][%f][%f] [%f][%f] [%f]", VPUSH(object().Position()),
                -object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch, fake_distance));
        VERIFY(_valid(result));
        break;
    }
    case eSightTypeCoverLookOver:
    {
        result.mad(object().Position(),
            Fvector().setHP(-object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch),
            fake_distance);
        VERIFY2(result.magnitude() < 100000.f,
            make_string("[%f][%f][%f] [%f][%f] [%f]", VPUSH(object().Position()),
                -object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch, fake_distance));
        VERIFY(_valid(result));
        break;
    }
    case eSightTypeAnimationDirection:
    {
        result.mad(object().Position(),
            Fvector().setHP(-object().movement().m_body.current.yaw, -object().movement().m_body.current.pitch),
            fake_distance);
        VERIFY2(result.magnitude() < 100000.f,
            make_string("[%f][%f][%f] [%f][%f] [%f]", VPUSH(object().Position()),
                -object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch, fake_distance));
        VERIFY(_valid(result));
        break;
    }
    default: NODEFAULT;
    }

    VERIFY2(result.magnitude() < 100000.f, make_string("[%f][%f][%f] [%f][%f] [%f]", VPUSH(result)));
    VERIFY(_valid(result));
    return (result);
}

static inline float lerp(float low, float high, float value)
{
    float result;
    if (low > high)
    {
        result = low - value;
        std::swap(low, high);
    }
    else
        result = low + value;
    ;

    if (result >= high)
        return (high);

    if (result <= low)
        return (low);

    return (result);
}

void CSightManager::process_action(float const time_delta)
{
    VERIFY(_valid(time_delta));
    VERIFY(_valid(s_factor_lerp_speed));

    //	if ( current_action().sight_type() == SightManager::eSightTypeAnimationDirection ) {
    //		m_current.m_spine.m_rotation			= Fidentity;
    //		m_current.m_shoulder.m_rotation			= Fidentity;
    //		m_current.m_head.m_rotation				= Fidentity;
    //		return;
    //	}

    SBoneRotation const& head = object().movement().m_head;
    SBoneRotation const& body = object().movement().m_body;

    Fvector const& factors = current_action().use_torso_look() ? s_danger_factors : s_free_factors;
    VERIFY(_valid(factors));
    //	if ( object().cName() == "level_prefix_stalker" ) {
    //		Msg							("[%6d][%6d] [%f] + [%f] = [%f] ([%f])",  Device.dwFrame, Device.dwTimeGlobal,
    // m_current.m_head.m_factor,		s_factor_lerp_speed*time_delta,		lerp ( m_current.m_head.m_factor,
    // factors.x,
    // s_factor_lerp_speed*time_delta ), factors.x );
    //		Msg							("[%6d][%6d] [%f] + [%f] = [%f] ([%f])",  Device.dwFrame, Device.dwTimeGlobal,
    // m_current.m_shoulder.m_factor,	s_factor_lerp_speed*time_delta,		lerp ( m_current.m_shoulder.m_factor,
    // factors.y, s_factor_lerp_speed*time_delta ), factors.y );
    //		Msg							("[%6d][%6d] [%f] + [%f] = [%f] ([%f])",  Device.dwFrame, Device.dwTimeGlobal,
    // m_current.m_spine.m_factor,	s_factor_lerp_speed*time_delta,		lerp ( m_current.m_spine.m_factor,
    // factors.z,
    // s_factor_lerp_speed*time_delta ), factors.z );
    //	}

    VERIFY(_valid(m_current.m_head.m_factor));
    m_current.m_head.m_factor = lerp(m_current.m_head.m_factor, factors.x, s_factor_lerp_speed * time_delta);
    VERIFY(_valid(m_current.m_head.m_factor));

    VERIFY(_valid(m_current.m_shoulder.m_factor));
    m_current.m_shoulder.m_factor = lerp(m_current.m_shoulder.m_factor, factors.y, s_factor_lerp_speed * time_delta);
    VERIFY(_valid(m_current.m_shoulder.m_factor));

    VERIFY(_valid(m_current.m_spine.m_factor));
    m_current.m_spine.m_factor = lerp(m_current.m_spine.m_factor, factors.z, s_factor_lerp_speed * time_delta);
    VERIFY(_valid(m_current.m_spine.m_factor));

    Fvector const angles = Fvector().set(angle_normalize_signed(-(head.current.pitch - body.current.pitch)),
        angle_normalize_signed(-(head.current.yaw - body.current.yaw)),
        angle_normalize_signed(head.current.roll - body.current.roll));

    m_current.m_head.m_rotation.setXYZ(Fvector(angles).mul(m_current.m_head.m_factor));
    m_current.m_shoulder.m_rotation.setXYZ(Fvector(angles).mul(m_current.m_shoulder.m_factor));
    m_current.m_spine.m_rotation.setXYZ(Fvector(angles).mul(m_current.m_spine.m_factor));
}

void CSightManager::compute_aiming(float const time_delta, float const angular_speed)
{
    switch (m_aiming_type)
    {
    case aiming_none:
    {
        process_action(time_delta);
        return;
    }
    case aiming_weapon:
    {
        if (!enabled())
            return;

        if (current_action().sight_type() == SightManager::eSightTypeAnimationDirection)
        {
            m_target.m_spine.m_rotation = Fidentity;
            m_target.m_shoulder.m_rotation = Fidentity;
            m_target.m_head.m_rotation = Fidentity;
            break;
        }

        if (!object().best_weapon())
        {
            m_target.m_spine.m_rotation = Fidentity;
            m_target.m_shoulder.m_rotation = Fidentity;
            m_target.m_head.m_rotation = Fidentity;
            break;
        }

        VERIFY(m_animation_id.size());
        VERIFY(m_animation_frame != animation_frame_none);

        bool forward_blend_callbacks = object().animation().forward_blend_callbacks();
        bool backward_blend_callbacks = object().animation().backward_blend_callbacks();
        object().animation().remove_bone_callbacks();
        VERIFY(object().best_weapon());
        VERIFY(smart_cast<CWeapon const*>(object().best_weapon()));
        VERIFY(_valid(aiming_position()));
        aimers::weapon aimer(&object(), m_animation_id.c_str(), m_animation_frame == animation_frame_start,
            aiming_position(), pSettings->r_string(object().cNameSect().c_str(), "bone_spin"),
            pSettings->r_string(object().cNameSect().c_str(), "bone_shoulder"),
            pSettings->r_string(object().cNameSect().c_str(), "weapon_bone0"),
            pSettings->r_string(object().cNameSect().c_str(), "weapon_bone2"),
            *smart_cast<CWeapon const*>(object().best_weapon()));
        if (forward_blend_callbacks)
            object().animation().assign_bone_blend_callbacks(true);
        else
        {
            if (backward_blend_callbacks)
                object().animation().assign_bone_blend_callbacks(false);
            else
                object().animation().assign_bone_callbacks();
        }

        m_target.m_spine.m_rotation = aimer.get_bone(0);
        m_target.m_shoulder.m_rotation = aimer.get_bone(1);
        m_target.m_head.m_rotation = Fidentity;

        if (!forward_blend_callbacks && !backward_blend_callbacks)
        {
            if (!fis_zero(time_delta))
                slerp_rotations(time_delta, angular_speed);

            break;
        }

        m_current.m_head.m_rotation = m_target.m_head.m_rotation;
        m_current.m_shoulder.m_rotation = m_target.m_shoulder.m_rotation;
        m_current.m_spine.m_rotation = m_target.m_spine.m_rotation;

        break;
    }
    case aiming_head:
    {
        if (!enabled())
            return;

        if (current_action().sight_type() == SightManager::eSightTypeAnimationDirection)
        {
            m_target.m_spine.m_rotation = Fidentity;
            m_target.m_shoulder.m_rotation = Fidentity;
            m_target.m_head.m_rotation = Fidentity;
            break;
        }

        VERIFY(m_animation_id.size());
        VERIFY(m_animation_frame != animation_frame_none);
        LPCSTR bones[] = {
            pSettings->r_string(object().cNameSect().c_str(), "bone_spin"),
            pSettings->r_string(object().cNameSect().c_str(), "bone_shoulder"),
            pSettings->r_string(object().cNameSect().c_str(), "bone_head"),
        };

        bool forward_blend_callbacks = object().animation().forward_blend_callbacks();
        bool backward_blend_callbacks = object().animation().backward_blend_callbacks();
        object().animation().remove_bone_callbacks();
        VERIFY(_valid(aiming_position()));
        aimers::bone<3> aimer(
            &object(), m_animation_id.c_str(), m_animation_frame == animation_frame_start, aiming_position(), bones);
        if (forward_blend_callbacks)
            object().animation().assign_bone_blend_callbacks(true);
        else
        {
            if (backward_blend_callbacks)
                object().animation().assign_bone_blend_callbacks(false);
            else
                object().animation().assign_bone_callbacks();
        }

        m_target.m_spine.m_rotation = aimer.get_bone(0);
        m_target.m_shoulder.m_rotation = aimer.get_bone(1);
        m_target.m_head.m_rotation = aimer.get_bone(2);

        if (!forward_blend_callbacks && !backward_blend_callbacks)
        {
            if (!fis_zero(time_delta))
            {
#ifdef DEBUG
                Msg("!animation movement controller wasn't created");
#endif // #ifdef DEBUG
                if (m_object->animation_movement())
                    slerp_rotations(time_delta, m_object->animation_movement()->IsBlending() ? .1f : angular_speed);
            }

            break;
        }

        m_current.m_head.m_rotation = m_target.m_head.m_rotation;
        m_current.m_shoulder.m_rotation = m_target.m_shoulder.m_rotation;
        m_current.m_spine.m_rotation = m_target.m_spine.m_rotation;

        break;
    }
    default: NODEFAULT;
    }
}

static void slerp_rotations(float const time_delta, float const angular_speed, Fmatrix& current, Fmatrix const& target)
{
    VERIFY(!fis_zero(time_delta));
    VERIFY(!fis_zero(angular_speed));

    Fquaternion left;
    left.set(current);

    Fquaternion right;
    right.set(target);

    Fquaternion left_inversed;
    left_inversed.inverse(left);

    Fquaternion difference;
    difference.mul(right, left_inversed);

    Fvector axe;
    float angle;
    difference.get_axis_angle(axe, angle);

    if (fis_zero(angle))
    {
        current = target;
        return;
    }

    float const test_angle = clampr(_abs(angle), EPS_L, PI);
    float speed = angular_speed;
    float const test_speed = PI / 36.f;
    float const min_speed = PI / 180.f;
    if (angular_speed > test_speed)
    {
        if (test_angle < test_speed)
        {
            speed = test_angle;
            if ((speed < angular_speed) && (speed < min_speed))
                speed = min_speed;
        }
    }

    float const factor = clampr(time_delta / (angle / speed), 0.f, 1.f);
    if (fsimilar(1.f, factor))
    {
        current = target;
        return;
    }

    Fquaternion result;
    result.slerp(left, right, factor);

    current.rotation(result);
}

void CSightManager::slerp_rotations(float const time_delta, float const angular_speed)
{
    ::slerp_rotations(time_delta, angular_speed, m_current.m_spine.m_rotation, m_target.m_spine.m_rotation);
    ::slerp_rotations(time_delta, angular_speed, m_current.m_shoulder.m_rotation, m_target.m_shoulder.m_rotation);
    ::slerp_rotations(time_delta, angular_speed, m_current.m_head.m_rotation, m_target.m_head.m_rotation);
}

void CSightManager::adjust_orientation()
{
    m_current.m_spine.m_rotation = Fidentity;
    m_current.m_shoulder.m_rotation = Fidentity;
    m_current.m_head.m_rotation = Fidentity;

    m_target.m_spine.m_rotation = Fidentity;
    m_target.m_shoulder.m_rotation = Fidentity;
    m_target.m_head.m_rotation = Fidentity;

    SBoneRotation& body = object().movement().m_body;
    object().XFORM().getXYZ(body.current.pitch, body.current.yaw, body.current.roll);
    body.current.pitch *= -1.f;
    body.current.yaw *= -1.f;
    body.current.roll = 0.f;
    body.target = body.current;

    SBoneRotation& head = object().movement().m_head;
    head.current = body.current;
    head.target = head.current;
}

void CSightManager::enable(bool const value)
{
    if (value == m_enabled)
        return;

    m_enabled = value;
    //	Msg					("[%d][%s] sight_enabled[%c]", Device.dwTimeGlobal, object().cName().c_str(), value ? '+' :
    //'-');

    if (!m_enabled)
        return;

    if (!object().animation_movement())
        return;

    adjust_orientation();
}
