#include "StdAfx.h"
#include "Artefact.h"
#include "xrPhysics/PhysicsShell.h"
#include "PhysicsShellHolder.h"
#include "game_cl_base.h"

#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"

#include "Inventory.h"
#include "Level.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrPhysics/IPHWorld.h"
#include "restriction_space.h"
#include "xrEngine/IGame_Persistent.h"

#include "artefact_activation.h"

#include "ai_space.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path_storage.h"

#define FASTMODE_DISTANCE (50.f) // distance to camera from sphere, when zone switches to fast update sequence

#define CHOOSE_MAX(x, inst_x, y, inst_y, z, inst_z) \
    if (x > y)                                      \
        if (x > z)                                  \
        {                                           \
            inst_x;                                 \
        }                                           \
        else                                        \
        {                                           \
            inst_z;                                 \
        }                                           \
    else if (y > z)                                 \
    {                                               \
        inst_y;                                     \
    }                                               \
    else                                            \
    {                                               \
        inst_z;                                     \
    }

CArtefact::CArtefact()
{
    shedule.t_min = 20;
    shedule.t_max = 50;
    m_sParticlesName = NULL;
    m_pTrailLight = NULL;
    m_activationObj = NULL;
    m_detectorObj = NULL;
    m_additional_weight = 0.0f;
}

CArtefact::~CArtefact() {}
void CArtefact::Load(LPCSTR section)
{
    inherited::Load(section);

    if (pSettings->line_exist(section, "particles"))
        m_sParticlesName = pSettings->r_string(section, "particles");

    m_bLightsEnabled = !!pSettings->r_bool(section, "lights_enabled");
    if (m_bLightsEnabled)
    {
        sscanf(pSettings->r_string(section, "trail_light_color"), "%f,%f,%f", &m_TrailLightColor.r,
            &m_TrailLightColor.g, &m_TrailLightColor.b);
        m_fTrailLightRange = pSettings->r_float(section, "trail_light_range");
    }

    m_fHealthRestoreSpeed = pSettings->r_float(section, "health_restore_speed");
    m_fRadiationRestoreSpeed = pSettings->r_float(section, "radiation_restore_speed");
    m_fSatietyRestoreSpeed = pSettings->r_float(section, "satiety_restore_speed");
    m_fPowerRestoreSpeed = pSettings->r_float(section, "power_restore_speed");
    m_fBleedingRestoreSpeed = pSettings->r_float(section, "bleeding_restore_speed");

    if (pSettings->section_exist(pSettings->r_string(section, "hit_absorbation_sect")))
    {
        m_ArtefactHitImmunities.LoadImmunities(pSettings->r_string(section, "hit_absorbation_sect"), pSettings);
    }
    m_bCanSpawnZone = !!pSettings->line_exist("artefact_spawn_zones", section);
    m_af_rank = pSettings->r_u8(section, "af_rank");
    m_additional_weight = pSettings->r_float(section, "additional_inventory_weight");
}

BOOL CArtefact::net_Spawn(CSE_Abstract* DC)
{
    if (pSettings->r_bool(cNameSect(), "can_be_controlled"))
        m_detectorObj = new SArtefactDetectorsSupport(this);

    BOOL result = inherited::net_Spawn(DC);
    SwitchAfParticles(true);

    StartLights();
    m_CarringBoneID = u16(-1);
    IKinematicsAnimated* K = smart_cast<IKinematicsAnimated*>(Visual());
    if (K)
        K->PlayCycle("idle");

    o_fastmode = FALSE; // start initially with fast-mode enabled
    o_render_frame = 0;
    SetState(eHidden);

    return result;
}

void CArtefact::net_Destroy()
{
    inherited::net_Destroy();

    StopLights();
    if (m_pTrailLight)
        m_pTrailLight.destroy();

    CPHUpdateObject::Deactivate();
    xr_delete(m_activationObj);
    xr_delete(m_detectorObj);
}

void CArtefact::OnH_A_Chield()
{
    inherited::OnH_A_Chield();

    StopLights();
    if (IsGameTypeSingle())
    {
        SwitchAfParticles(false);
    }
    else
    {
        IKinematics* K = smart_cast<IKinematics*>(H_Parent()->Visual());
        if (K)
            m_CarringBoneID = K->LL_BoneID("bip01_head");
        else
            m_CarringBoneID = u16(-1);
    }
    if (m_detectorObj)
    {
        m_detectorObj->m_currPatrolPath = NULL;
        m_detectorObj->m_currPatrolVertex = NULL;
    }
}

void CArtefact::OnH_B_Independent(bool just_before_destroy)
{
    VERIFY(!physics_world()->Processing());
    inherited::OnH_B_Independent(just_before_destroy);

    StartLights();
    SwitchAfParticles(true);
}

void CArtefact::SwitchAfParticles(bool bOn)
{
    if (m_sParticlesName.size() == 0)
        return;

    if (bOn)
    {
        Fvector dir;
        dir.set(0, 1, 0);
        CParticlesPlayer::StartParticles(m_sParticlesName, dir, ID(), -1, false);
    }
    else
    {
        CParticlesPlayer::StopParticles(m_sParticlesName, BI_NONE, true);
    }
}

// called only in "fast-mode"
void CArtefact::UpdateCL()
{
    inherited::UpdateCL();

    if (o_fastmode || m_activationObj)
        UpdateWorkload(Device.dwTimeDelta);
}

void CArtefact::Interpolate()
{
    if (OnServer())
        return;

    net_updateInvData* p = NetSync();
    while (p->NET_IItem.size() > 1) // in real no interpolation, just get latest state
    {
        p->NET_IItem.pop_front();
    }
    inherited::Interpolate();

    if (p->NET_IItem.size())
    {
        p->NET_IItem.clear(); // same as p->NET_IItem.pop_front();
    }
}

void CArtefact::UpdateWorkload(u32 dt)
{
    VERIFY(!physics_world()->Processing());
    // particles - velocity
    Fvector vel = {0, 0, 0};
    if (H_Parent())
    {
        CPhysicsShellHolder* pPhysicsShellHolder = smart_cast<CPhysicsShellHolder*>(H_Parent());
        if (pPhysicsShellHolder)
            pPhysicsShellHolder->PHGetLinearVell(vel);
    }
    CParticlesPlayer::SetParentVel(vel);

    //
    UpdateLights();
    if (m_activationObj && m_activationObj->IsInProgress())
    {
        CPHUpdateObject::Activate();
        m_activationObj->UpdateActivation();
        return;
    }

    // custom-logic
    if (!CAttachableItem::enabled())
        UpdateCLChild();
}

void CArtefact::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    //////////////////////////////////////////////////////////////////////////
    // check "fast-mode" border
    if (H_Parent())
        o_switch_2_slow();
    else
    {
        Fvector center;
        Center(center);
        BOOL rendering = (Device.dwFrame == o_render_frame);
        float cam_distance = Device.vCameraPosition.distance_to(center) - Radius();
        if (rendering || (cam_distance < FASTMODE_DISTANCE))
            o_switch_2_fast();
        else
            o_switch_2_slow();
    }
    if (!o_fastmode)
        UpdateWorkload(dt);

    if (!H_Parent() && m_detectorObj)
    {
        m_detectorObj->UpdateOnFrame();
    }
}

void CArtefact::create_physic_shell()
{
    m_pPhysicsShell = P_build_Shell(this, false);
    m_pPhysicsShell->Deactivate();
}

void CArtefact::StartLights()
{
    VERIFY(!physics_world()->Processing());
    if (!m_bLightsEnabled)
        return;

    VERIFY(m_pTrailLight == NULL);
    m_pTrailLight = GEnv.Render->light_create();
    bool const b_light_shadow = !!pSettings->r_bool(cNameSect(), "idle_light_shadow");

    m_pTrailLight->set_shadow(b_light_shadow);

    m_pTrailLight->set_color(m_TrailLightColor);
    m_pTrailLight->set_range(m_fTrailLightRange);
    m_pTrailLight->set_position(Position());
    m_pTrailLight->set_active(true);
}

void CArtefact::StopLights()
{
    VERIFY(!physics_world()->Processing());
    if (!m_bLightsEnabled || !m_pTrailLight)
        return;

    m_pTrailLight->set_active(false);
    m_pTrailLight.destroy();
}

void CArtefact::UpdateLights()
{
    VERIFY(!physics_world()->Processing());
    if (!m_bLightsEnabled || !m_pTrailLight || !m_pTrailLight->get_active())
        return;
    m_pTrailLight->set_position(Position());
}

void CArtefact::ActivateArtefact()
{
    VERIFY(m_bCanSpawnZone);
    VERIFY(H_Parent());
    CreateArtefactActivation();
    if (!m_activationObj)
        return;
    m_activationObj->Start();
}

void CArtefact::PhDataUpdate(float step)
{
    if (m_activationObj && m_activationObj->IsInProgress())
        m_activationObj->PhDataUpdate(step);
}

bool CArtefact::CanTake() const
{
    if (!inherited::CanTake())
        return false;

    if (m_activationObj && m_activationObj->IsInProgress())
        return false;

    return true;
}

void CArtefact::Hide() { SwitchState(eHiding); }
void CArtefact::Show() { SwitchState(eShowing); }
void CArtefact::MoveTo(Fvector const& position)
{
    if (!PPhysicsShell())
        return;

    Fmatrix M = XFORM();
    M.translate(position);
    ForceTransform(M);
    // m_bInInterpolation = false;
}

#include "InventoryOwner.h"
#include "entity_alive.h"
void CArtefact::UpdateXForm()
{
    if (Device.dwFrame != dwXF_Frame)
    {
        dwXF_Frame = Device.dwFrame;

        if (0 == H_Parent())
            return;

        // Get access to entity and its visual
        CEntityAlive* E = smart_cast<CEntityAlive*>(H_Parent());

        if (!E)
            return;

        const CInventoryOwner* parent = smart_cast<const CInventoryOwner*>(E);
        if (parent && parent->use_simplified_visual())
            return;

        VERIFY(E);
        IKinematics* V = smart_cast<IKinematics*>(E->Visual());
        VERIFY(V);
        if (CAttachableItem::enabled())
            return;

        // Get matrices
        int boneL = -1, boneR = -1, boneR2 = -1;
        E->g_WeaponBones(boneL, boneR, boneR2);
        if (boneR == -1)
            return;

        boneL = boneR2;

        V->CalculateBones();
        Fmatrix& mL = V->LL_GetTransform(u16(boneL));
        Fmatrix& mR = V->LL_GetTransform(u16(boneR));

        // Calculate
        Fmatrix mRes;
        Fvector R, D, N;
        D.sub(mL.c, mR.c);
        D.normalize_safe();
        R.crossproduct(mR.j, D);
        R.normalize_safe();
        N.crossproduct(D, R);
        N.normalize_safe();
        mRes.set(R, N, D, mR.c);
        mRes.mulA_43(E->XFORM());
        //		UpdatePosition		(mRes);
        XFORM().mul(mRes, offset());
    }
}
#include "xr_level_controller.h"
bool CArtefact::Action(u16 cmd, u32 flags)
{
    switch (cmd)
    {
    case kWPN_FIRE:
    {
        if (flags & CMD_START && m_bCanSpawnZone)
        {
            SwitchState(eActivating);
            return true;
        }
        if (flags & CMD_STOP && m_bCanSpawnZone && GetState() == eActivating)
        {
            SwitchState(eIdle);
            return true;
        }
    }
    break;
    default: break;
    }
    return inherited::Action(cmd, flags);
}

void CArtefact::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);
    switch (S)
    {
    case eShowing: { PlayHUDMotion("anm_show", FALSE, this, S);
    }
    break;
    case eHiding:
    {
        if (oldState != eHiding)
            PlayHUDMotion("anm_hide", FALSE, this, S);
    }
    break;
    case eActivating: { PlayHUDMotion("anm_activate", FALSE, this, S);
    }
    break;
    case eIdle: { PlayAnimIdle();
    }
    break;
    };
}

void CArtefact::PlayAnimIdle() { PlayHUDMotion("anm_idle", FALSE, NULL, eIdle); }
void CArtefact::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eHiding: { SwitchState(eHidden);
    }
    break;
    case eShowing: { SwitchState(eIdle);
    }
    break;
    case eActivating:
    {
        if (Local())
        {
            SwitchState(eHiding);
            NET_Packet P;
            u_EventGen(P, GEG_PLAYER_ACTIVATEARTEFACT, H_Parent()->ID());
            P.w_u16(ID());
            u_EventSend(P);
        }
    }
    break;
    };
}

void CArtefact::FollowByPath(LPCSTR path_name, int start_idx, Fvector magic_force)
{
    if (m_detectorObj)
        m_detectorObj->FollowByPath(path_name, start_idx, magic_force);
}

bool CArtefact::CanBeInvisible() { return (m_detectorObj != NULL); }
void CArtefact::SwitchVisibility(bool b)
{
    if (m_detectorObj)
        m_detectorObj->SetVisible(b);
}

void CArtefact::StopActivation()
{
    // VERIFY2(m_activationObj, "activation object not initialized");
    if (!m_activationObj)
        return;
    m_activationObj->Stop();
}

void CArtefact::ForceTransform(const Fmatrix& m)
{
    VERIFY(PPhysicsShell());
    XFORM().set(m);
    PPhysicsShell()->SetGlTransformDynamic(m); // XFORM().set(m);
}

void CArtefact::CreateArtefactActivation()
{
    if (m_activationObj)
    {
        return;
    }
    m_activationObj = new SArtefactActivation(this, H_Parent()->ID());
}

SArtefactDetectorsSupport::SArtefactDetectorsSupport(CArtefact* A)
    : m_parent(A), m_currPatrolPath(NULL), m_currPatrolVertex(NULL), m_switchVisTime(0)
{
}

SArtefactDetectorsSupport::~SArtefactDetectorsSupport() { m_sound.destroy(); }
void SArtefactDetectorsSupport::SetVisible(bool b)
{
    m_switchVisTime = Device.dwTimeGlobal;
    if (b == !!m_parent->getVisible())
        return;

    if (b)
        m_parent->StartLights();
    else
        m_parent->StopLights();

    if (b)
    {
        LPCSTR curr =
            pSettings->r_string(m_parent->cNameSect().c_str(), (b) ? "det_show_particles" : "det_hide_particles");

        IKinematics* K = smart_cast<IKinematics*>(m_parent->Visual());
        R_ASSERT2(K, m_parent->cNameSect().c_str());
        LPCSTR bone = pSettings->r_string(m_parent->cNameSect().c_str(), "particles_bone");
        u16 bone_id = K->LL_BoneID(bone);
        R_ASSERT2(bone_id != BI_NONE, bone);

        m_parent->CParticlesPlayer::StartParticles(curr, bone_id, Fvector().set(0, 1, 0), m_parent->ID());

        curr = pSettings->r_string(m_parent->cNameSect().c_str(), (b) ? "det_show_snd" : "det_hide_snd");
        m_sound.create(curr, st_Effect, sg_SourceType);
        m_sound.play_at_pos(0, m_parent->Position(), 0);
    }

    m_parent->setVisible(b);
    m_parent->SwitchAfParticles(b);
}

void SArtefactDetectorsSupport::Blink()
{
    LPCSTR curr = pSettings->r_string(m_parent->cNameSect().c_str(), "det_show_particles");

    IKinematics* K = smart_cast<IKinematics*>(m_parent->Visual());
    R_ASSERT2(K, m_parent->cNameSect().c_str());
    LPCSTR bone = pSettings->r_string(m_parent->cNameSect().c_str(), "particles_bone");
    u16 bone_id = K->LL_BoneID(bone);
    R_ASSERT2(bone_id != BI_NONE, bone);

    m_parent->CParticlesPlayer::StartParticles(curr, bone_id, Fvector().set(0, 1, 0), m_parent->ID(), 1000, true);
}

void SArtefactDetectorsSupport::UpdateOnFrame()
{
    if (m_currPatrolPath && !m_parent->getVisible())
    {
        if (m_parent->Position().distance_to(m_destPoint) < 2.0f)
        {
            CPatrolPath::const_iterator b, e;
            m_currPatrolPath->begin(m_currPatrolVertex, b, e);
            if (b != e)
            {
                std::advance(b, ::Random.randI(s32(e - b)));
                m_currPatrolVertex = m_currPatrolPath->vertex((*b).vertex_id());
                m_destPoint = m_currPatrolVertex->data().position();
            }
        }
        float cos_et = _cos(deg2rad(45.f));
        Fvector dir;
        dir.sub(m_destPoint, m_parent->Position()).normalize_safe();

        Fvector v;
        m_parent->PHGetLinearVell(v);
        float cosa = v.dotproduct(dir);
        if (v.square_magnitude() < (0.7f * 0.7f) || (cosa < cos_et))
        {
            Fvector power = dir;
            power.y += 1.0f;
            power.mul(m_path_moving_force);
            m_parent->m_pPhysicsShell->applyGravityAccel(power);
        }
    }

    if (m_parent->getVisible() && m_parent->GetAfRank() != 0 && m_switchVisTime + 5000 < Device.dwTimeGlobal)
        SetVisible(false);

    u32 dwDt = 2 * 3600 * 1000 / 10; // 2 hour of game time
    if (!m_parent->getVisible() && m_switchVisTime + dwDt < Device.dwTimeGlobal)
    {
        m_switchVisTime = Device.dwTimeGlobal;
        if (m_parent->Position().distance_to(Device.vCameraPosition) > 40.0f)
            Blink();
    }
}

void SArtefactDetectorsSupport::FollowByPath(LPCSTR path_name, int start_idx, Fvector force)
{
    m_currPatrolPath = ai().patrol_paths().path(path_name, true);
    if (m_currPatrolPath)
    {
        m_currPatrolVertex = m_currPatrolPath->vertex(start_idx);
        m_destPoint = m_currPatrolVertex->data().position();
        m_path_moving_force = force;
    }
}

void CArtefact::OnActiveItem()
{
    SwitchState(eShowing);
    inherited::OnActiveItem();
    SetState(eIdle);
    SetNextState(eIdle);
}

void CArtefact::OnHiddenItem()
{
    if (IsGameTypeSingle())
        SwitchState(eHiding);
    else
        SwitchState(eHidden);

    inherited::OnHiddenItem();
    SetState(eHidden);
    SetNextState(eHidden);
}
