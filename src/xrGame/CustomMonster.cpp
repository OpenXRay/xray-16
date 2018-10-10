// CustomMonster.cpp: implementation of the CCustomMonster class.
//
//////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_debug.h"
#include "CustomMonster.h"
#include "ai_space.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer.h"
#include "seniority_hierarchy_holder.h"
#include "team_hierarchy_holder.h"
#include "squad_hierarchy_holder.h"
#include "group_hierarchy_holder.h"
#include "CustomZone.h"
#include "Include/xrRender/Kinematics.h"
#include "detail_path_manager.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "sound_memory_manager.h"
#include "enemy_manager.h"
#include "item_manager.h"
#include "danger_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrAICore/Navigation/ai_object_location_impl.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/game_graph.h"
#include "movement_manager.h"
#include "EntityCondition.h"
#include "sound_player.h"
#include "Level.h"
#include "level_debug.h"
#include "material_manager.h"
#include "sound_user_data_visitor.h"
#include "mt_config.h"
#include "PHMovementControl.h"
#include "xrEngine/profiler.h"
#include "date_time.h"
#include "CharacterPhysicsSupport.h"
#include "ai/monsters/snork/snork.h"
#include "ai/monsters/burer/burer.h"
#include "GamePersistent.h"
#include "Actor.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "client_spawn_manager.h"
#include "moving_object.h"
#include "level_path_manager.h"

// Lain: added
#include "xrEngine/IGame_Level.h"
#include "xrCore/_vector3d_ext.h"
#include "debug_text_tree.h"
#include "xrPhysics/IPHWorld.h"

#ifdef DEBUG
#include "debug_renderer.h"
#include "animation_movement_controller.h"
#endif // DEBUG

void SetActorVisibility(u16 who, float value);
extern int g_AI_inactive_time;

#ifndef MASTER_GOLD
Flags32 psAI_Flags = {aiObstaclesAvoiding | aiUseSmartCovers};
#endif // MASTER_GOLD

void CCustomMonster::SAnimState::Create(IKinematicsAnimated* K, LPCSTR base)
{
    char buf[128];
    fwd = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base, "_fwd"));
    back = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base, "_back"));
    ls = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base, "_ls"));
    rs = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base, "_rs"));
}

// void __stdcall CCustomMonster::TorsoSpinCallback(CBoneInstance* B)
//{
//	CCustomMonster*		M = static_cast<CCustomMonster*> (B->Callback_Param);
//
//	Fmatrix					spin;
//	spin.setXYZ				(0, M->NET_Last.o_torso.pitch, 0);
//	B->mTransform.mulB_43	(spin);
//}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCustomMonster::CCustomMonster()
    : // this is non-polymorphic call of the virtual function cast_entity_alive
      // just to remove warning C4355 if we use this instead
      Feel::Vision(cast_game_object())
{
    m_sound_user_data_visitor = 0;
    m_memory_manager = 0;
    m_movement_manager = 0;
    m_sound_player = 0;
    m_already_dead = false;
    m_invulnerable = false;
    m_moving_object = 0;
}

CCustomMonster::~CCustomMonster()
{
    xr_delete(m_sound_user_data_visitor);
    xr_delete(m_memory_manager);
    xr_delete(m_movement_manager);
    xr_delete(m_sound_player);

    // Lain: added (asking GameLevel to forget about self)
    if (g_pGameLevel)
    {
        g_pGameLevel->SoundEvent_OnDestDestroy(this);
    }

#ifdef DEBUG
    Msg("dumping client spawn manager stuff for object with id %d", ID());
    if (!GEnv.isDedicatedServer)
        Level().client_spawn_manager().dump(ID());
#endif // DEBUG
    if (!GEnv.isDedicatedServer)
        Level().client_spawn_manager().clear(ID());
}

void CCustomMonster::Load(LPCSTR section)
{
    inherited::Load(section);

    if (character_physics_support())
    {
        material().Load(section);
        character_physics_support()->movement()->Load(section);
    }

    memory().Load(section);
    movement().Load(section);
    //////////////////////////////////////////////////////////////////////////

    ///////////
    // m_PhysicMovementControl: General

    // Fbox	bb;

    //// m_PhysicMovementControl: BOX
    // Fvector	vBOX0_center= pSettings->r_fvector3	(section,"ph_box0_center"	);
    // Fvector	vBOX0_size	= pSettings->r_fvector3	(section,"ph_box0_size"		);
    // bb.set	(vBOX0_center,vBOX0_center); bb.grow(vBOX0_size);
    // m_PhysicMovementControl->SetBox		(0,bb);

    //// m_PhysicMovementControl: BOX
    // Fvector	vBOX1_center= pSettings->r_fvector3	(section,"ph_box1_center"	);
    // Fvector	vBOX1_size	= pSettings->r_fvector3	(section,"ph_box1_size"		);
    // bb.set	(vBOX1_center,vBOX1_center); bb.grow(vBOX1_size);
    // m_PhysicMovementControl->SetBox		(1,bb);

    //// m_PhysicMovementControl: Foots
    // Fvector	vFOOT_center= pSettings->r_fvector3	(section,"ph_foot_center"	);
    // Fvector	vFOOT_size	= pSettings->r_fvector3	(section,"ph_foot_size"		);
    // bb.set	(vFOOT_center,vFOOT_center); bb.grow(vFOOT_size);
    // m_PhysicMovementControl->SetFoots	(vFOOT_center,vFOOT_size);

    //// m_PhysicMovementControl: Crash speed and mass
    // float	cs_min		= pSettings->r_float	(section,"ph_crash_speed_min"	);
    // float	cs_max		= pSettings->r_float	(section,"ph_crash_speed_max"	);
    // float	mass		= pSettings->r_float	(section,"ph_mass"				);
    // m_PhysicMovementControl->SetCrashSpeeds	(cs_min,cs_max);
    // m_PhysicMovementControl->SetMass		(mass);

    // m_PhysicMovementControl: Frictions
    /*
    float af, gf, wf;
    af					= pSettings->r_float	(section,"ph_friction_air"	);
    gf					= pSettings->r_float	(section,"ph_friction_ground");
    wf					= pSettings->r_float	(section,"ph_friction_wall"	);
    m_PhysicMovementControl->SetFriction	(af,wf,gf);

    // BOX activate
    m_PhysicMovementControl->ActivateBox	(0);
    */
    ////////

    Position().y += EPS_L;

    //	m_current			= 0;

    eye_fov = pSettings->r_float(section, "eye_fov");
    eye_range = pSettings->r_float(section, "eye_range");

    // Health & Armor
    //	fArmor					= 0;

    // Msg				("! cmonster size: %d",sizeof(*this));
}

void CCustomMonster::reinit()
{
    CScriptEntity::reinit();
    CEntityAlive::reinit();

    if (character_physics_support())
        material().reinit();

    movement().reinit();
    sound().reinit();

    m_client_update_delta = 0;
    m_last_client_update_time = Device.dwTimeGlobal;

    eye_pp_stage = 0;
    m_dwLastUpdateTime = 0xffffffff;
    m_tEyeShift.set(0, 0, 0);
    m_fEyeShiftYaw = 0.f;
    NET_WasExtrapolating = FALSE;

    //////////////////////////////////////////////////////////////////////////
    // Critical Wounds
    //////////////////////////////////////////////////////////////////////////

    m_critical_wound_type = u32(-1);
    m_last_hit_time = 0;
    m_critical_wound_accumulator = 0.f;
    m_critical_wound_threshold = pSettings->r_float(cNameSect(), "critical_wound_threshold");
    m_critical_wound_decrease_quant = pSettings->r_float(cNameSect(), "critical_wound_decrease_quant");

    if (m_critical_wound_threshold >= 0)
        load_critical_wound_bones();
    //////////////////////////////////////////////////////////////////////////
    m_update_rotation_on_frame = true;
    m_movement_enabled_before_animation_controller = true;
}

void CCustomMonster::reload(LPCSTR section)
{
    sound().reload(section);
    CEntityAlive::reload(section);

    if (character_physics_support())
        material().reload(section);

    movement().reload(section);
    load_killer_clsids(section);

    m_far_plane_factor = READ_IF_EXISTS(pSettings, r_float, section, "far_plane_factor", 1.f);
    m_fog_density_factor = READ_IF_EXISTS(pSettings, r_float, section, "fog_density_factor", .05f);

    m_panic_threshold = pSettings->r_float(section, "panic_threshold");
}

void CCustomMonster::mk_orientation(Fvector& dir, Fmatrix& mR)
{
    // orient only in XZ plane
    dir.y = 0;
    float len = dir.magnitude();
    if (len > EPS_S)
    {
        // normalize
        dir.x /= len;
        dir.z /= len;
        Fvector up;
        up.set(0, 1, 0);
        mR.rotation(dir, up);
    }
}

void CCustomMonster::net_Export(NET_Packet& P) // export to server
{
    R_ASSERT(Local());

    // export last known packet
    R_ASSERT(!NET.empty());
    net_update& N = NET.back();
    P.w_float(GetfHealth());
    P.w_u32(N.dwTimeStamp);
    P.w_u8(0);
    P.w_vec3(N.p_pos);
    P.w_float /*w_angle8*/ (N.o_model);
    P.w_float /*w_angle8*/ (N.o_torso.yaw);
    P.w_float /*w_angle8*/ (N.o_torso.pitch);
    P.w_float /*w_angle8*/ (N.o_torso.roll);
    P.w_u8(u8(g_Team()));
    P.w_u8(u8(g_Squad()));
    P.w_u8(u8(g_Group()));
}

void CCustomMonster::net_Import(NET_Packet& P)
{
    R_ASSERT(Remote());
    net_update N;

    u8 flags;

    float health;
    P.r_float(health);
    SetfHealth(health);

    P.r_u32(N.dwTimeStamp);
    P.r_u8(flags);
    P.r_vec3(N.p_pos);
    P.r_float /*r_angle8*/ (N.o_model);
    P.r_float /*r_angle8*/ (N.o_torso.yaw);
    P.r_float /*r_angle8*/ (N.o_torso.pitch);
    P.r_float /*r_angle8*/ (N.o_torso.roll);

    id_Team = P.r_u8();
    id_Squad = P.r_u8();
    id_Group = P.r_u8();

    if (NET.empty() || (NET.back().dwTimeStamp < N.dwTimeStamp))
    {
        NET.push_back(N);
        NET_WasInterpolating = TRUE;
    }

    setVisible(TRUE);
    setEnabled(TRUE);
}

void CCustomMonster::shedule_Update(u32 DT)
{
    VERIFY(!g_Alive() || processing_enabled());
    // Queue shrink
    VERIFY(_valid(Position()));
    u32 dwTimeCL = Level().timeServer() - NET_Latency;
    VERIFY(!NET.empty());
    while ((NET.size() > 2) && (NET[1].dwTimeStamp < dwTimeCL))
        NET.pop_front();

    float dt = float(DT) / 1000.f;
    // *** general stuff
    if (g_Alive())
    {
        if (false && g_mt_config.test(mtAiVision))
#ifndef DEBUG
            Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CCustomMonster::Exec_Visibility));
#else // DEBUG
        {
            if (!psAI_Flags.test(aiStalker) || !!smart_cast<CActor*>(Level().CurrentEntity()))
                Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CCustomMonster::Exec_Visibility));
            else
                Exec_Visibility();
        }
#endif // DEBUG
        else
            Exec_Visibility();
        memory().update(dt);
    }
    inherited::shedule_Update(DT);

    // Queue setup
    if (dt > 3)
        return;

    m_dwCurrentTime = Device.dwTimeGlobal;

    VERIFY(_valid(Position()));
    if (Remote())
    {
    }
    else
    {
        // here is monster AI call
        m_fTimeUpdateDelta = dt;
        Level().AIStats.Think.Begin();
        if (GetScriptControl())
            ProcessScripts();
        else
        {
            if (Device.dwFrame > spawn_time() + g_AI_inactive_time)
                Think();
        }
        m_dwLastUpdateTime = Device.dwTimeGlobal;
        Level().AIStats.Think.End();

        // Look and action streams
        float temp = conditions().health();
        if (temp > 0)
        {
            Exec_Action(dt);
            VERIFY(_valid(Position()));
            // Exec_Visibility		();
            VERIFY(_valid(Position()));
            //////////////////////////////////////
            // Fvector C; float R;
            //////////////////////////////////////
            // С Олеся - ПИВО!!!! (Диме :-))))
            // m_PhysicMovementControl->GetBoundingSphere	(C,R);
            //////////////////////////////////////
            // Center(C);
            // R = Radius();
            //////////////////////////////////////
            /// #pragma todo("Oles to all AI guys: perf/logical problem: Only few objects needs 'feel_touch' why to call
            /// update for everybody?")
            ///			feel_touch_update		(C,R);

            net_update uNext;
            uNext.dwTimeStamp = Level().timeServer();
            uNext.o_model = movement().m_body.current.yaw;
            uNext.o_torso = movement().m_body.current;
            uNext.p_pos = Position();
            uNext.fHealth = GetfHealth();
            NET.push_back(uNext);
        }
        else
        {
            net_update uNext;
            uNext.dwTimeStamp = Level().timeServer();
            uNext.o_model = movement().m_body.current.yaw;
            uNext.o_torso = movement().m_body.current;
            uNext.p_pos = Position();
            uNext.fHealth = GetfHealth();
            NET.push_back(uNext);
        }
    }
}

void CCustomMonster::net_update::lerp(CCustomMonster::net_update& A, CCustomMonster::net_update& B, float f)
{
    //
    o_model = angle_lerp(A.o_model, B.o_model, f);
    o_torso.yaw = angle_lerp(A.o_torso.yaw, B.o_torso.yaw, f);
    o_torso.pitch = angle_lerp(A.o_torso.pitch, B.o_torso.pitch, f);
    p_pos.lerp(A.p_pos, B.p_pos, f);
    fHealth = A.fHealth * (1.f - f) + B.fHealth * f;
}

void CCustomMonster::update_sound_player() { sound().update(client_update_fdelta()); }
void CCustomMonster::UpdateCL()
{
    START_PROFILE("CustomMonster/client_update")
    m_client_update_delta = (u32)std::min(Device.dwTimeGlobal - m_last_client_update_time, u32(100));
    m_last_client_update_time = Device.dwTimeGlobal;

#ifdef DEBUG
    if (animation_movement())
        animation_movement()->DBG_verify_position_not_chaged();
#endif

    START_PROFILE("CustomMonster/client_update/inherited")
    inherited::UpdateCL();
    STOP_PROFILE

#ifdef DEBUG
    if (animation_movement())
        animation_movement()->DBG_verify_position_not_chaged();
#endif

    CScriptEntity::process_sound_callbacks();

    /*	//. hack just to skip 'CalculateBones'
    if (sound().need_bone_data()) {
        // we do this because we know here would be virtual function call
        IKinematics					*kinematics = smart_cast<IKinematics*>(Visual());
        VERIFY						(kinematics);
        kinematics->CalculateBones	();
    }
    */

    if (g_mt_config.test(mtSoundPlayer))
        Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CCustomMonster::update_sound_player));
    else
    {
        START_PROFILE("CustomMonster/client_update/sound_player")
        update_sound_player();
        STOP_PROFILE
    }

    START_PROFILE("CustomMonster/client_update/network extrapolation")
    if (NET.empty())
    {
        update_animation_movement_controller();
        return;
    }

    m_dwCurrentTime = Device.dwTimeGlobal;

    // distinguish interpolation/extrapolation
    u32 dwTime = Level().timeServer() - NET_Latency;
    net_update& N = NET.back();
    if ((dwTime > N.dwTimeStamp) || (NET.size() < 2))
    {
        // BAD.	extrapolation
        NET_Last = N;
    }
    else
    {
        // OK.	interpolation
        NET_WasExtrapolating = FALSE;
        // Search 2 keyframes for interpolation
        int select = -1;
        for (u32 id = 0; id < NET.size() - 1; ++id)
        {
            if ((NET[id].dwTimeStamp <= dwTime) && (dwTime <= NET[id + 1].dwTimeStamp))
                select = id;
        }
        if (select >= 0)
        {
            // Interpolate state
            net_update& A = NET[select + 0];
            net_update& B = NET[select + 1];
            u32 d1 = dwTime - A.dwTimeStamp;
            u32 d2 = B.dwTimeStamp - A.dwTimeStamp;
            //			VERIFY					(d2);
            float factor = d2 ? (float(d1) / float(d2)) : 1.f;
            Fvector l_tOldPosition = Position();
            NET_Last.lerp(A, B, factor);
            if (Local())
            {
                NET_Last.p_pos = l_tOldPosition;
            }
            else
            {
                if (!bfScriptAnimation())
                    SelectAnimation(XFORM().k, movement().detail().direction(), movement().speed());
            }

            // Signal, that last time we used interpolation
            NET_WasInterpolating = TRUE;
            NET_Time = dwTime;
        }
    }
    STOP_PROFILE

#ifdef DEBUG
    if (animation_movement())
        animation_movement()->DBG_verify_position_not_chaged();
#endif

    if (Local() && g_Alive())
    {
#pragma todo("Dima to All : this is FAKE, network is not supported here!")

        UpdatePositionAnimation();
    }

    // Use interpolated/last state
    if (g_Alive())
    {
        if (!animation_movement_controlled() && m_update_rotation_on_frame)
            XFORM().rotateY(NET_Last.o_model);
        if (!animation_movement_controlled())
            XFORM().translate_over(NET_Last.p_pos);

        if (!animation_movement_controlled() && m_update_rotation_on_frame)
        {
            Fmatrix M;
            M.setHPB(0.0f, -NET_Last.o_torso.pitch, 0.0f);
            XFORM().mulB_43(M);
        }
    }

#ifdef DEBUG
    if (animation_movement())
        animation_movement()->DBG_verify_position_not_chaged();
#endif

#ifdef DEBUG
    if (IsMyCamera())
        UpdateCamera();
#endif // DEBUG

    update_animation_movement_controller();

#ifdef DEBUG
    if (animation_movement())
        animation_movement()->DBG_verify_position_not_chaged();
#endif

    STOP_PROFILE
}

void CCustomMonster::UpdatePositionAnimation()
{
    START_PROFILE("CustomMonster/client_update/movement")
    movement().on_frame(character_physics_support()->movement(), NET_Last.p_pos);
    STOP_PROFILE

    START_PROFILE("CustomMonster/client_update/animation")
    if (!bfScriptAnimation())
        SelectAnimation(XFORM().k, movement().detail().direction(), movement().speed());
    STOP_PROFILE
}

BOOL CCustomMonster::feel_visible_isRelevant(IGameObject* O)
{
    CEntityAlive* E = smart_cast<CEntityAlive*>(O);
    if (0 == E)
        return FALSE;
    if (E->g_Team() == g_Team())
        return FALSE;
    return TRUE;
}

void CCustomMonster::eye_pp_s0()
{
    // Eye matrix
    IKinematics* V = smart_cast<IKinematics*>(Visual());
    V->CalculateBones();
    Fmatrix& mEye = V->LL_GetTransform(u16(eye_bone));
    Fmatrix X;
    X.mul_43(XFORM(), mEye);
    VERIFY(_valid(mEye));

    const MonsterSpace::SBoneRotation& rotation = head_orientation();

    VERIFY(_valid(rotation.current.yaw));
    VERIFY(_valid(m_fEyeShiftYaw));
    VERIFY(_valid(rotation.current.pitch));

    eye_matrix.setHPB(-rotation.current.yaw + m_fEyeShiftYaw, -rotation.current.pitch, 0);
    eye_matrix.c.add(X.c, m_tEyeShift);

    VERIFY(_valid(eye_matrix));
}

void CCustomMonster::update_range_fov(float& new_range, float& new_fov, float start_range, float start_fov)
{
    const float standard_far_plane = eye_range;

    float current_fog_density = GamePersistent().Environment().CurrentEnv->fog_density;
    // 0=no_fog, 1=full_fog, >1 = super-fog
    float current_far_plane = GamePersistent().Environment().CurrentEnv->far_plane;
    // 300=standart, 50=super-fog

    new_fov = start_fov;
    new_range = start_range * (std::min(m_far_plane_factor * current_far_plane, standard_far_plane) / standard_far_plane) *
        (1.f / (1.f + m_fog_density_factor * current_fog_density));
}

void CCustomMonster::eye_pp_s1()
{
    float new_range = eye_range, new_fov = eye_fov;
    if (g_Alive())
    {
#ifndef USE_STALKER_VISION_FOR_MONSTERS
        update_range_fov(new_range, new_fov,
            human_being() ? memory().visual().current_state().m_max_view_distance * eye_range : eye_range, eye_fov);
#else
        update_range_fov(
            new_range, new_fov, memory().visual().current_state().m_max_view_distance * eye_range, eye_fov);
#endif
    }
    // Standart visibility
    Level().AIStats.VisQuery.Begin();
    Fmatrix mProject, mFull, mView;
    mView.build_camera_dir(eye_matrix.c, eye_matrix.k, eye_matrix.j);
    VERIFY(_valid(eye_matrix));
    mProject.build_projection(deg2rad(new_fov), 1, 0.1f, new_range);
    mFull.mul(mProject, mView);
    feel_vision_query(mFull, eye_matrix.c);
    Level().AIStats.VisQuery.End();
}

void CCustomMonster::eye_pp_s2()
{
    // Tracing
    Level().AIStats.VisRayTests.Begin();
    u32 dwTime = Level().timeServer();
    u32 dwDT = dwTime - eye_pp_timestamp;
    eye_pp_timestamp = dwTime;
    feel_vision_update(this, eye_matrix.c, float(dwDT) / 1000.f, memory().visual().transparency_threshold());
    Level().AIStats.VisRayTests.End();
}

void CCustomMonster::Exec_Visibility()
{
    // if (0==Sector())				return;
    if (!g_Alive())
        return;

    Level().AIStats.Vis.Begin();
    switch (eye_pp_stage % 2)
    {
    case 0:
        eye_pp_s0();
        eye_pp_s1();
        break;
    case 1: eye_pp_s2(); break;
    }
    ++eye_pp_stage;
    Level().AIStats.Vis.End();
}

void CCustomMonster::UpdateCamera()
{
    float new_range = eye_range, new_fov = eye_fov;
    if (g_Alive())
        update_range_fov(
            new_range, new_fov, memory().visual().current_state().m_max_view_distance * eye_range, eye_fov);
    g_pGameLevel->Cameras().Update(eye_matrix.c, eye_matrix.k, eye_matrix.j, new_fov, .75f, new_range, 0);
}

void CCustomMonster::HitSignal(float /**perc/**/, Fvector& /**vLocalDir/**/, IGameObject* /**who/**/) {}
void CCustomMonster::Die(IGameObject* who)
{
    inherited::Die(who);
    // Level().RemoveMapLocationByID(this->ID());
    SetActorVisibility(ID(), 0.f);
}

BOOL CCustomMonster::net_Spawn(CSE_Abstract* DC)
{
    memory().reload(*cNameSect());
    memory().reinit();

    if (!movement().net_Spawn(DC) || !inherited::net_Spawn(DC) || !CScriptEntity::net_Spawn(DC))
        return (FALSE);

    ISpatial* self = smart_cast<ISpatial*>(this);
    if (self)
    {
        self->GetSpatialData().type |= STYPE_VISIBLEFORAI;
        // enable react to sound only if alive
        if (g_Alive())
            self->GetSpatialData().type |= STYPE_REACTTOSOUND;
    }

    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeMonsterAbstract* E = smart_cast<CSE_ALifeMonsterAbstract*>(e);

    eye_matrix.identity();
    movement().m_body.current.yaw = movement().m_body.target.yaw = -E->o_torso.yaw;
    movement().m_body.current.pitch = movement().m_body.target.pitch = 0;
    SetfHealth(E->get_health());
    if (!g_Alive())
    {
        set_death_time();
        //		Msg						("%6d : Object [%d][%s][%s] is spawned
        // DEAD",Device.dwTimeGlobal,ID(),*cName(),*cNameSect());
    }

    if (ai().get_level_graph() && UsedAI_Locations() && (e->ID_Parent == 0xffff))
    {
        if (ai().game_graph().valid_vertex_id(E->m_tGraphID))
            ai_location().game_vertex(E->m_tGraphID);

        if (ai().game_graph().valid_vertex_id(E->m_tNextGraphID) &&
            (ai().game_graph().vertex(E->m_tNextGraphID)->level_id() == ai().level_graph().level_id()) &&
            movement().restrictions().accessible(ai().game_graph().vertex(E->m_tNextGraphID)->level_vertex_id()))
            movement().set_game_dest_vertex(E->m_tNextGraphID);

        if (movement().restrictions().accessible(ai_location().level_vertex_id()))
            movement().set_level_dest_vertex(ai_location().level_vertex_id());
        else
        {
            Fvector dest_position;
            u32 level_vertex_id;
            level_vertex_id = movement().restrictions().accessible_nearest(
                ai().level_graph().vertex_position(ai_location().level_vertex_id()), dest_position);
            movement().set_level_dest_vertex(level_vertex_id);
            movement().detail().set_dest_position(dest_position);
        }
    }

    // Eyes
    eye_bone = smart_cast<IKinematics*>(Visual())->LL_BoneID(pSettings->r_string(cNameSect(), "bone_head"));

    // weapons
    if (Local())
    {
        net_update N;
        N.dwTimeStamp = Level().timeServer() - NET_Latency;
        N.o_model = -E->o_torso.yaw;
        N.o_torso.yaw = -E->o_torso.yaw;
        N.o_torso.pitch = 0;
        N.p_pos.set(Position());
        NET.push_back(N);

        N.dwTimeStamp += NET_Latency;
        NET.push_back(N);

        setVisible(TRUE);
        setEnabled(TRUE);
    }

    // Sheduler
    shedule.t_min = 100;
    shedule.t_max = 250; // This equaltiy is broken by Dima :-( // 30 * NET_Latency / 4;

    m_moving_object = new moving_object(this);

    return TRUE;
}

#ifdef DEBUG
void CCustomMonster::OnHUDDraw(CCustomHUD* hud) {}
#endif

void CCustomMonster::Exec_Action(float /**dt/**/) {}
// void CCustomMonster::Hit(float P, Fvector &dir,IGameObject* who, s16 element,Fvector position_in_object_space, float
// impulse, ALife::EHitType hit_type)
void CCustomMonster::Hit(SHit* pHDS)
{
    if (!invulnerable())
        inherited::Hit(pHDS);
}

void CCustomMonster::OnEvent(NET_Packet& P, u16 type) { inherited::OnEvent(P, type); }
void CCustomMonster::net_Destroy()
{
    inherited::net_Destroy();
    CScriptEntity::net_Destroy();
    sound().unload();
    movement().net_Destroy();

    Device.remove_from_seq_parallel(fastdelegate::FastDelegate0<>(this, &CCustomMonster::update_sound_player));
    Device.remove_from_seq_parallel(fastdelegate::FastDelegate0<>(this, &CCustomMonster::Exec_Visibility));

#ifdef DEBUG
    DBG().on_destroy_object(this);
#endif

    xr_delete(m_moving_object);

    SetActorVisibility(ID(), 0.0f);
}

BOOL CCustomMonster::UsedAI_Locations() { return (TRUE); }
void CCustomMonster::PitchCorrection()
{
    CLevelGraph::SContour contour;
    ai().level_graph().contour(contour, ai_location().level_vertex_id());

    Fplane P;
    P.build(contour.v1, contour.v2, contour.v3);

    Fvector position_on_plane;
    P.project(position_on_plane, Position());

    // находим проекцию точки, лежащей на векторе текущего направления
    Fvector dir_point, proj_point;
    dir_point.mad(position_on_plane, Direction(), 1.f);
    P.project(proj_point, dir_point);

    // получаем искомый вектор направления
    Fvector target_dir;
    target_dir.sub(proj_point, position_on_plane);

    float yaw, pitch;
    target_dir.getHP(yaw, pitch);

    movement().m_body.target.pitch = -pitch;
}

bool CCustomMonster::feel_touch_on_contact(IGameObject* O)
{
    CCustomZone* custom_zone = smart_cast<CCustomZone*>(O);
    if (!custom_zone)
        return (true);

    Fsphere sphere;
    sphere.P = Position();
    sphere.R = EPS_L;
    if (custom_zone->inside(sphere))
        return (true);

    return (false);
}

bool CCustomMonster::feel_touch_contact(IGameObject* O)
{
    CCustomZone* custom_zone = smart_cast<CCustomZone*>(O);
    if (!custom_zone)
        return (true);

    Fsphere sphere;
    sphere.P = Position();
    sphere.R = 0.f;
    if (custom_zone->inside(sphere))
        return (true);

    return (false);
}

void CCustomMonster::set_ready_to_save()
{
    inherited::set_ready_to_save();
    memory().enemy().set_ready_to_save();
}

void CCustomMonster::load_killer_clsids(LPCSTR section)
{
    m_killer_clsids.clear();
    LPCSTR killers = pSettings->r_string(section, "killer_clsids");
    string16 temp;
    for (u32 i = 0, n = _GetItemCount(killers); i < n; ++i)
        m_killer_clsids.push_back(TEXT2CLSID(_GetItem(killers, i, temp)));
}

bool CCustomMonster::is_special_killer(IGameObject* obj)
{
    return (
        obj && (std::find(m_killer_clsids.begin(), m_killer_clsids.end(), obj->GetClassId()) != m_killer_clsids.end()));
}

float CCustomMonster::feel_vision_mtl_transp(IGameObject* O, u32 element)
{
    return (memory().visual().feel_vision_mtl_transp(O, element));
}

void CCustomMonster::feel_sound_new(
    IGameObject* who, int type, CSound_UserDataPtr user_data, const Fvector& position, float power)
{
    // Lain: added
    if (!g_Alive())
    {
        return;
    }
    if (getDestroy())
    {
        return;
    }
    memory().sound().feel_sound_new(who, type, user_data, position, power);
}

bool CCustomMonster::useful(const CItemManager* manager, const CGameObject* object) const
{
    return (memory().item().useful(object));
}

float CCustomMonster::evaluate(const CItemManager* manager, const CGameObject* object) const
{
    return (memory().item().evaluate(object));
}

bool CCustomMonster::useful(const CEnemyManager* manager, const CEntityAlive* object) const
{
    return (memory().enemy().useful(object));
}

float CCustomMonster::evaluate(const CEnemyManager* manager, const CEntityAlive* object) const
{
    return (memory().enemy().evaluate(object));
}

bool CCustomMonster::useful(const CDangerManager* manager, const CDangerObject& object) const
{
    return (memory().danger().useful(object));
}

float CCustomMonster::evaluate(const CDangerManager* manager, const CDangerObject& object) const
{
    return (memory().danger().evaluate(object));
}

CMovementManager* CCustomMonster::create_movement_manager() { return (new CMovementManager(this)); }
CSound_UserDataVisitor* CCustomMonster::create_sound_visitor()
{
    return (m_sound_user_data_visitor = new CSound_UserDataVisitor());
}

CMemoryManager* CCustomMonster::create_memory_manager() { return (new CMemoryManager(this, create_sound_visitor())); }
const SRotation CCustomMonster::Orientation() const { return (movement().m_body.current); };
const MonsterSpace::SBoneRotation& CCustomMonster::head_orientation() const { return (movement().m_body); }
IFactoryObject* CCustomMonster::_construct()
{
    m_memory_manager = create_memory_manager();
    m_movement_manager = create_movement_manager();
    m_sound_player = new CSoundPlayer(this);

    inherited::_construct();
    CScriptEntity::_construct();

    return (this);
}

void CCustomMonster::net_Relcase(IGameObject* object)
{
    inherited::net_Relcase(object);
    memory().remove_links(object);
}

void CCustomMonster::set_fov(float new_fov)
{
    VERIFY(new_fov > 0.f);
    eye_fov = new_fov;
}

void CCustomMonster::set_range(float new_range)
{
    VERIFY(new_range > 1.f);
    eye_range = new_range;
}

void CCustomMonster::on_restrictions_change()
{
    memory().on_restrictions_change();
    movement().on_restrictions_change();
}

LPCSTR CCustomMonster::visual_name(CSE_Abstract* server_entity) { return (inherited::visual_name(server_entity)); }
void CCustomMonster::on_enemy_change(const CEntityAlive* enemy) {}
CVisualMemoryManager* CCustomMonster::visual_memory() const { return (&memory().visual()); }
void CCustomMonster::save(NET_Packet& packet)
{
    inherited::save(packet);
    if (g_Alive())
        memory().save(packet);
}

void CCustomMonster::load(IReader& packet)
{
    inherited::load(packet);
    if (g_Alive())
        memory().load(packet);
}

bool CCustomMonster::update_critical_wounded(const u16& bone_id, const float& power)
{
    // object should not be critical wounded
    VERIFY(m_critical_wound_type == u32(-1));
    // check 'multiple updates during last hit' situation
    VERIFY(Device.dwTimeGlobal >= m_last_hit_time);

    if (m_critical_wound_threshold < 0)
        return (false);

    float time_delta = m_last_hit_time ? float(Device.dwTimeGlobal - m_last_hit_time) / 1000.f : 0.f;
    m_critical_wound_accumulator += power - m_critical_wound_decrease_quant * time_delta;
    clamp(m_critical_wound_accumulator, 0.f, m_critical_wound_threshold);

#if 0 // def _DEBUG
	Msg								(
		"%6d [%s] update_critical_wounded: %f[%f] (%f,%f) [%f]",
		Device.dwTimeGlobal,
		*cName(),
		m_critical_wound_accumulator,
		power,
		m_critical_wound_threshold,
		m_critical_wound_decrease_quant,
		time_delta
	);
#endif // DEBUG

    m_last_hit_time = Device.dwTimeGlobal;
    if (m_critical_wound_accumulator < m_critical_wound_threshold)
        return (false);

    m_last_hit_time = 0;
    m_critical_wound_accumulator = 0.f;

    if (critical_wound_external_conditions_suitable())
    {
        BODY_PART::const_iterator I = m_bones_body_parts.find(bone_id);
        if (I == m_bones_body_parts.end())
            return (false);

        m_critical_wound_type = (*I).second;

        critical_wounded_state_start();

        return (true);
    }

    return (false);
}

#ifdef DEBUG

extern void dbg_draw_frustum(float FOV, float _FAR, float A, Fvector& P, Fvector& D, Fvector& U);
void draw_visiblity_rays(CCustomMonster* self, const IGameObject* object, collide::rq_results& rq_storage);

void CCustomMonster::OnRender()
{
    GEnv.DRender->OnFrameEnd();
    // RCache.OnFrameEnd				();

    {
        float const radius = .075f;
        xr_vector<u32> const& path = movement().level_path().path();
        xr_vector<u32>::const_iterator i = path.begin();
        xr_vector<u32>::const_iterator const e = path.end();
        for (; i != e; ++i)
            Level().debug_renderer().draw_aabb(
                ai().level_graph().vertex_position(*i), radius, radius, radius, color_xrgb(255, 22, 255));
    }

    for (int i = 0; i < 1; ++i)
    {
        const xr_vector<CDetailPathManager::STravelPoint>& keys =
            !i ? movement().detail().m_key_points : movement().detail().m_key_points;
        const xr_vector<DetailPathManager::STravelPathPoint>& path =
            !i ? movement().detail().path() : movement().detail().path();
        u32 color0 = !i ? color_xrgb(0, 255, 0) : color_xrgb(0, 0, 255);
        u32 color1 = !i ? color_xrgb(255, 0, 0) : color_xrgb(255, 255, 0);
        u32 color2 = !i ? color_xrgb(0, 0, 255) : color_xrgb(0, 255, 255);
        u32 color3 = !i ? color_xrgb(255, 255, 255) : color_xrgb(255, 0, 255);
        float radius0 = !i ? .1f : .15f;
        float radius1 = !i ? .2f : .3f;
        {
            for (u32 I = 1; I < path.size(); ++I)
            {
                const DetailPathManager::STravelPathPoint& N1 = path[I - 1];
                Fvector P1;
                P1.set(N1.position);
                P1.y += 0.1f;
                const DetailPathManager::STravelPathPoint& N2 = path[I];
                Fvector P2;
                P2.set(N2.position);
                P2.y += 0.1f;
                if (!fis_zero(P1.distance_to_sqr(P2), EPS_L))
                    Level().debug_renderer().draw_line(Fidentity, P1, P2, color0);
                if ((path.size() - 1) == I) // песледний box?
                    Level().debug_renderer().draw_aabb(P1, radius0, radius0, radius0, color1);
                else
                    Level().debug_renderer().draw_aabb(P1, radius0, radius0, radius0, color2);
            }

            for (u32 I = 1; I < keys.size(); ++I)
            {
                CDetailPathManager::STravelPoint temp;
                temp = keys[I - 1];
                Fvector P1;
                P1.set(temp.position.x, ai().level_graph().vertex_plane_y(temp.vertex_id), temp.position.y);
                P1.y += 0.1f;

                temp = keys[I];
                Fvector P2;
                P2.set(temp.position.x, ai().level_graph().vertex_plane_y(temp.vertex_id), temp.position.y);
                P2.y += 0.1f;

                if (!fis_zero(P1.distance_to_sqr(P2), EPS_L))
                    Level().debug_renderer().draw_line(Fidentity, P1, P2, color1);
                Level().debug_renderer().draw_aabb(P1, radius1, radius1, radius1, color3);
            }
        }
    }
    {
        u32 node = movement().level_dest_vertex_id();
        if (node == u32(-1))
            node = 0;

        Fvector P1 = ai().level_graph().vertex_position(node);
        P1.y += 1.f;
        Level().debug_renderer().draw_aabb(P1, .5f, 1.f, .5f, color_xrgb(255, 0, 0));
    }
    if (g_Alive())
    {
        if (memory().enemy().selected())
        {
            Fvector P1 = memory().memory(memory().enemy().selected()).m_object_params.m_position;
            P1.y += 1.f;
            Level().debug_renderer().draw_aabb(P1, 1.f, 1.f, 1.f, color_xrgb(0, 0, 0));
        }

        if (memory().danger().selected())
        {
            Fvector P1 = memory().danger().selected()->position();
            P1.y += 1.f;
            Level().debug_renderer().draw_aabb(P1, 1.f, 1.f, 1.f, color_xrgb(0, 0, 0));
        }
    }

    if (psAI_Flags.test(aiFrustum))
    {
        float new_range = eye_range, new_fov = eye_fov;

        if (g_Alive())
            update_range_fov(
                new_range, new_fov, memory().visual().current_state().m_max_view_distance * eye_range, eye_fov);

        dbg_draw_frustum(new_fov, new_range, 1, eye_matrix.c, eye_matrix.k, eye_matrix.j);
    }

    if (psAI_Flags.test(aiMotion))
        if (character_physics_support())
            character_physics_support()->movement()->dbg_Draw();

    if (bDebug)
        smart_cast<IKinematics*>(Visual())->DebugRender(XFORM());

#if 0
	DBG().get_text_tree().clear			();
	debug::text_tree& text_tree		=	DBG().get_text_tree().find_or_add("ActorView");

	Fvector collide_position;
	collide::rq_results	temp_rq_results;
	Fvector sizes			=	{ 0.2f, 0.2f, 0.2f };

	for ( u32 i=0; i<2; ++i )
	{
		Fvector start		=	{ -8.7, 1.6, -4.67 };
		Fvector end			=	{ -9.45, 1.3, -0.24 };

		bool use_p2			=	false;
		ai_dbg::get_var			("p2", use_p2);

		if ( use_p2 ^ i )
		{
			start.x			+=	-1.f;
			end.x			+=	-1.f;
		}

		Fvector velocity	=	end - start;
		float const jump_time	=	0.3f;
		TransferenceToThrowVel	(velocity,jump_time,physics_world()->Gravity());

		bool const result	=	trajectory_intersects_geometry	(jump_time, 
																 start,
																 end,
																 velocity,
																 collide_position,
																 this,
																 NULL,
																 temp_rq_results,
																 & m_jump_picks,
																 & m_jump_collide_tris,
																 sizes);

		text_tree.add_line(i ? "box1" : "box2", result);
	}
#endif // #if 0

    if (m_jump_picks.size() < 1)
        return;

    xr_vector<trajectory_pick>::const_iterator I = m_jump_picks.begin();
    xr_vector<trajectory_pick>::const_iterator E = m_jump_picks.end();
    for (; I != E; ++I)
    {
        trajectory_pick pick = *I;

        float const inv_nx = (pick.invert_x & 1) ? -1.f : 1.f;
        float const inv_ny = (pick.invert_y & 1) ? -1.f : 1.f;
        float const inv_nz = (pick.invert_z & 1) ? -1.f : 1.f;

        float const inv_x = (pick.invert_x & 2) ? -1.f : 1.f;
        float const inv_y = (pick.invert_y & 2) ? -1.f : 1.f;
        float const inv_z = (pick.invert_z & 2) ? -1.f : 1.f;

        Fvector const traj_start = pick.center - pick.z_axis * pick.sizes.z * 0.5f * inv_z;
        Fvector const traj_end = pick.center + pick.z_axis * pick.sizes.z * 0.5f * inv_z;

        Fvector const z_offs[] = {
            (pick.x_axis * pick.sizes.x * 0.5f) + (pick.y_axis * pick.sizes.y * 0.5f),
            (-pick.x_axis * pick.sizes.x * 0.5f) + (pick.y_axis * pick.sizes.y * 0.5f),
            (pick.x_axis * pick.sizes.x * 0.5f) - (pick.y_axis * pick.sizes.y * 0.5f),
            (-pick.x_axis * pick.sizes.x * 0.5f) - (pick.y_axis * pick.sizes.y * 0.5f),
        };

        Fvector const z_normal = -pick.z_axis * 0.1 * inv_nz;
        Level().debug_renderer().draw_line(Fidentity, traj_start, traj_start + z_normal, color_xrgb(128, 255, 128));
        Level().debug_renderer().draw_line(Fidentity, traj_end, traj_end - z_normal, color_xrgb(128, 255, 128));

        for (u32 i = 0; i < sizeof(z_offs) / sizeof(z_offs[0]); ++i)
            Level().debug_renderer().draw_line(
                Fidentity, traj_start + z_offs[i], traj_end + z_offs[i], color_xrgb(255, 255, 128));

        Fvector const hor_start = pick.center - pick.x_axis * pick.sizes.x * 0.5f * inv_x;
        Fvector const hor_end = pick.center + pick.x_axis * pick.sizes.x * 0.5f * inv_x;

        Fvector const x_offs[] = {
            (pick.y_axis * pick.sizes.y * 0.5f) + (pick.z_axis * pick.sizes.z * 0.5f),
            (-pick.y_axis * pick.sizes.y * 0.5f) + (pick.z_axis * pick.sizes.z * 0.5f),
            (pick.y_axis * pick.sizes.y * 0.5f) - (pick.z_axis * pick.sizes.z * 0.5f),
            (-pick.y_axis * pick.sizes.y * 0.5f) - (pick.z_axis * pick.sizes.z * 0.5f),
        };

        Fvector const x_normal = -pick.x_axis * 0.1 * inv_nx;
        Level().debug_renderer().draw_line(Fidentity, hor_start, hor_start + x_normal, color_xrgb(128, 255, 128));
        Level().debug_renderer().draw_line(Fidentity, hor_end, hor_end - x_normal, color_xrgb(128, 255, 128));

        for (u32 i = 0; i < sizeof(x_offs) / sizeof(x_offs[0]); ++i)
            Level().debug_renderer().draw_line(
                Fidentity, hor_start + x_offs[i], hor_end + x_offs[i], color_xrgb(255, 255, 128));

        Fvector const ver_start = pick.center - pick.y_axis * pick.sizes.y * 0.5f * inv_y;
        Fvector const ver_end = pick.center + pick.y_axis * pick.sizes.y * 0.5f * inv_y;

        Fvector const y_offs[] = {
            (pick.x_axis * pick.sizes.x * 0.5f) + (pick.z_axis * pick.sizes.z * 0.5f),
            (-pick.x_axis * pick.sizes.x * 0.5f) + (pick.z_axis * pick.sizes.z * 0.5f),
            (pick.x_axis * pick.sizes.x * 0.5f) - (pick.z_axis * pick.sizes.z * 0.5f),
            (-pick.x_axis * pick.sizes.x * 0.5f) - (pick.z_axis * pick.sizes.z * 0.5f),
        };

        Fvector const y_normal = -pick.y_axis * 0.1 * inv_ny;
        Level().debug_renderer().draw_line(Fidentity, ver_start, ver_start + y_normal, color_xrgb(128, 255, 128));
        Level().debug_renderer().draw_line(Fidentity, ver_end, ver_end - y_normal, color_xrgb(128, 255, 128));

        for (u32 i = 0; i < sizeof(y_offs) / sizeof(y_offs[0]); ++i)
            Level().debug_renderer().draw_line(
                Fidentity, ver_start + y_offs[i], ver_end + y_offs[i], color_xrgb(255, 255, 128));

        Level().debug_renderer().draw_line(Fidentity, traj_start, traj_end, color_xrgb(255, 0, 0));
    }

    for (u32 i = 0; i < m_jump_collide_tris.size(); i += 3)
    {
        Fvector const v1 = m_jump_collide_tris[i];
        Fvector const v2 = m_jump_collide_tris[i + 1];
        Fvector const v3 = m_jump_collide_tris[i + 2];

        Fmatrix unit;
        unit.identity();

        Level().debug_renderer().draw_line(unit, v1, v2, color_xrgb(255, 255, 255));
        Level().debug_renderer().draw_line(unit, v1, v3, color_xrgb(255, 255, 255));
        Level().debug_renderer().draw_line(unit, v2, v3, color_xrgb(255, 255, 255));
    }
}
#endif // DEBUG

void CCustomMonster::spatial_move()
{
    inherited::spatial_move();

    get_moving_object()->on_object_move();
}

Fvector CCustomMonster::predict_position(const float& time_to_check) const
{
    return (movement().predict_position(time_to_check));
}

Fvector CCustomMonster::target_position() const { return (movement().target_position()); }
void CCustomMonster::create_anim_mov_ctrl(CBlend* b, Fmatrix* start_pose, bool local_animation)
{
    bool already_initialized = animation_movement_controlled();
    inherited::create_anim_mov_ctrl(b, start_pose, local_animation);

    if (already_initialized)
        return;

    m_movement_enabled_before_animation_controller = movement().enabled();
    movement().enable_movement(false);
}

void CCustomMonster::destroy_anim_mov_ctrl()
{
    inherited::destroy_anim_mov_ctrl();

    movement().enable_movement(m_movement_enabled_before_animation_controller);

    float roll;
    XFORM().getHPB(movement().m_body.current.yaw, movement().m_body.current.pitch, roll);

    movement().m_body.current.yaw *= -1.f;
    movement().m_body.current.pitch *= -1.f;

    movement().m_body.target.yaw = movement().m_body.current.yaw;
    movement().m_body.target.pitch = movement().m_body.current.pitch;

    NET_Last.o_model = movement().m_body.current.yaw;
    NET_Last.o_torso.pitch = movement().m_body.current.pitch;
}

void CCustomMonster::ForceTransform(const Fmatrix& m)
{
    character_physics_support()->ForceTransform(m);
    const float block_damage_time_seconds = 2.f;
    if (!IsGameTypeSingle())
        character_physics_support()->movement()->BlockDamageSet(u64(block_damage_time_seconds / fixed_step));
}

Fvector CCustomMonster::spatial_sector_point()
{
    // if ( g_Alive() )
    //	return						inherited::spatial_sector_point( );

    // if ( !animation_movement() )
    return inherited::spatial_sector_point().add(Fvector().set(0.f, Radius() * .5f, 0.f));

    // IKinematics* const kinematics	= smart_cast<IKinematics*>(Visual());
    // VERIFY							(kinematics);
    // u16 const root_bone_id			= kinematics->LL_BoneID("bip01_spine");

    // Fmatrix local;
    // kinematics->Bone_GetAnimPos		( local, root_bone_id, u8(-1), false );

    // Fmatrix result;
    // result.mul_43					( XFORM(), local );
    // return							result.c;
}
