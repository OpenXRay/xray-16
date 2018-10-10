////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_crow.cpp
//	Created 	: 13.05.2002
//  Modified 	: 13.05.2002
//	Author		: Dmitriy Iassenev
//	Description : AI Behaviour for monster "Crow"
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrPhysics/phvalide.h"
#include "ai_crow.h"
#include "Level.h"
#include "Include/xrRender/RenderVisual.h"
#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Actor.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "game_object_space.h"
#include "script_game_object.h"
#include "Hit.h"
#ifdef DEBUG

#endif

void CAI_Crow::SAnim::Load(IKinematicsAnimated* visual, LPCSTR prefix)
{
    const MotionID& M = visual->ID_Cycle_Safe(prefix);
    if (M)
        m_Animations.push_back(M);
    for (int i = 0; (i < MAX_ANIM_COUNT) && (m_Animations.size() < MAX_ANIM_COUNT); ++i)
    {
        string128 sh_anim;
        xr_sprintf(sh_anim, "%s_%d", prefix, i);
        const MotionID& M = visual->ID_Cycle_Safe(sh_anim);
        if (M)
            m_Animations.push_back(M);
    }
    R_ASSERT(m_Animations.size());
}

void CAI_Crow::SSound::Load(LPCSTR prefix)
{
    string_path fn;
    if (FS.exist(fn, "$game_sounds$", prefix, ".ogg"))
    {
        m_Sounds.push_back(ref_sound());
        GEnv.Sound->create(m_Sounds.back(), prefix, st_Effect, sg_SourceType);
    }
    for (int i = 0; (i < MAX_SND_COUNT) && (m_Sounds.size() < MAX_SND_COUNT); ++i)
    {
        string64 name;
        xr_sprintf(name, "%s_%d", prefix, i);
        if (FS.exist(fn, "$game_sounds$", name, ".ogg"))
        {
            m_Sounds.push_back(ref_sound());
            GEnv.Sound->create(m_Sounds.back(), name, st_Effect, sg_SourceType);
        }
    }
    R_ASSERT(m_Sounds.size());
}

void CAI_Crow::SSound::SetPosition(const Fvector& pos)
{
    for (int i = 0; i < (int)m_Sounds.size(); ++i)
        if (m_Sounds[i]._feedback())
            m_Sounds[i].set_position(pos);
}

void CAI_Crow::SSound::Unload()
{
    for (int i = 0; i < (int)m_Sounds.size(); ++i)
        GEnv.Sound->destroy(m_Sounds[i]);
}

void cb_OnHitEndPlaying(CBlend* B) { ((CAI_Crow*)B->CallbackParam)->OnHitEndPlaying(B); }
void CAI_Crow::OnHitEndPlaying(CBlend* /**B/**/) { bPlayDeathIdle = true; }
CAI_Crow::CAI_Crow() { init(); }
CAI_Crow::~CAI_Crow()
{
    // removing all data no more being neded
    m_Sounds.m_idle.Unload();
}

void CAI_Crow::init()
{
    st_current = eUndef;
    st_target = eFlyIdle;
    vGoalDir.set(10.0f * (Random.randF() - Random.randF()), 10.0f * (Random.randF() - Random.randF()),
        10.0f * (Random.randF() - Random.randF()));
    vCurrentDir.set(0, 0, 1);
    vHPB.set(0, 0, 0);
    fDHeading = 0;
    fGoalChangeDelta = 10.f;
    fGoalChangeTime = 0.f;
    fSpeed = 5.f;
    fASpeed = 0.2f;
    fMinHeight = 40.f;
    vVarGoal.set(10.f, 10.f, 100.f);
    fIdleSoundDelta = 10.f;
    fIdleSoundTime = fIdleSoundDelta;
    bPlayDeathIdle = false;
    o_workload_frame = 0;
    o_workload_rframe = 0;
}

void CAI_Crow::Load(LPCSTR section)
{
    inherited::Load(section);
    //////////////////////////////////////////////////////////////////////////
    ISpatial* self = smart_cast<ISpatial*>(this);
    if (self)
    {
        self->GetSpatialData().type &= ~STYPE_VISIBLEFORAI;
        self->GetSpatialData().type &= ~STYPE_REACTTOSOUND;
    }
    //////////////////////////////////////////////////////////////////////////

    // sounds
    m_Sounds.m_idle.Load("monsters" DELIMITER "crow" DELIMITER "idle");
    // play defaut

    fSpeed = pSettings->r_float(section, "speed");
    fASpeed = pSettings->r_float(section, "angular_speed");
    fGoalChangeDelta = pSettings->r_float(section, "goal_change_delta");
    fMinHeight = pSettings->r_float(section, "min_height");
    vVarGoal = pSettings->r_fvector3(section, "goal_variability");
    fIdleSoundDelta = pSettings->r_float(section, "idle_sound_delta");
    fIdleSoundTime = fIdleSoundDelta + fIdleSoundDelta * Random.randF(-.5f, .5f);
    VERIFY2(valid_pos(Position()), dbg_valide_pos_string(Position(), this, "CAI_Crow::Load( LPCSTR section )"));
}

BOOL CAI_Crow::net_Spawn(CSE_Abstract* DC)
{
    BOOL R = inherited::net_Spawn(DC);
    setVisible(TRUE);
    setEnabled(TRUE);

    // animations
    IKinematicsAnimated* M = smart_cast<IKinematicsAnimated*>(Visual());
    R_ASSERT(M);
    m_Anims.m_death.Load(M, "death");
    m_Anims.m_death_dead.Load(M, "death_drop");
    m_Anims.m_death_idle.Load(M, "death_idle");
    m_Anims.m_fly.Load(M, "fly_fwd");
    m_Anims.m_idle.Load(M, "fly_idle");

    o_workload_frame = 0;
    o_workload_rframe = 0;

    if (GetfHealth() > 0)
    {
        st_current = ECrowStates::eFlyIdle;
        st_target = ECrowStates::eFlyIdle;

        // disable UpdateCL, enable only on HIT
        processing_deactivate();
    }
    else
    {
        st_current = ECrowStates::eDeathFall;
        st_target = ECrowStates::eDeathDead;

        // Crow is already dead, need to enable physics
        processing_activate();
        CreateSkeleton();
    }

    VERIFY2(valid_pos(Position()), dbg_valide_pos_string(Position(), this, "CAI_Crow::net_Spawn"));
    return R;
}

void CAI_Crow::net_Destroy()
{
    inherited::net_Destroy();

    m_Anims.m_death.m_Animations.clear();
    m_Anims.m_death_dead.m_Animations.clear();
    m_Anims.m_death_idle.m_Animations.clear();
    m_Anims.m_fly.m_Animations.clear();
    m_Anims.m_idle.m_Animations.clear();
}

// crow update
void CAI_Crow::switch2_FlyUp() { smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(m_Anims.m_fly.GetRandom()); }
void CAI_Crow::switch2_FlyIdle() { smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(m_Anims.m_idle.GetRandom()); }
void CAI_Crow::switch2_DeathDead()
{
    // AI need to pickup this
    ISpatial* self = smart_cast<ISpatial*>(this);
    if (self)
        self->GetSpatialData().type |= STYPE_VISIBLEFORAI;
    //
    smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(m_Anims.m_death_dead.GetRandom());
}
void CAI_Crow::switch2_DeathFall()
{
    Fvector V;
    V.mul(XFORM().k, fSpeed);
    //	m_PhysicMovementControl->SetVelocity(V);
    smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(m_Anims.m_death.GetRandom(), TRUE, cb_OnHitEndPlaying, this);
}

void CAI_Crow::state_Flying(float fdt)
{
    // Update position and orientation of the planes
    float fAT = fASpeed * fdt;
    Fvector& vDirection = XFORM().k;

    // Tweak orientation based on last position and goal
    Fvector vOffset;
    vOffset.sub(vGoalDir, Position());

    // First, tweak the pitch
    if (vOffset.y > 1.0)
    { // We're too low
        vHPB.y += fAT;
        if (vHPB.y > 0.8f)
            vHPB.y = 0.8f;
    }
    else if (vOffset.y < -1.0)
    { // We're too high
        vHPB.y -= fAT;
        if (vHPB.y < -0.8f)
            vHPB.y = -0.8f;
    }
    else // Add damping
        vHPB.y *= 0.95f;

    // Now figure out yaw changes
    vOffset.y = 0.0f;
    vDirection.y = 0.0f;

    vDirection.normalize();
    vOffset.normalize();

    float fDot = vDirection.dotproduct(vOffset);
    fDot = (1.0f - fDot) / 2.0f * fAT * 10.0f;

    vOffset.crossproduct(vOffset, vDirection);

    if (vOffset.y > 0.01f)
        fDHeading = (fDHeading * 9.0f + fDot) * 0.1f;
    else if (vOffset.y < 0.01f)
        fDHeading = (fDHeading * 9.0f - fDot) * 0.1f;

    vHPB.x += fDHeading;
    vHPB.z = -fDHeading * 9.0f;

    // Update position
    vOldPosition.set(Position());
    XFORM().setHPB(vHPB.x, vHPB.y, vHPB.z);
    Position().mad(vOldPosition, vDirection, fSpeed * fdt);
    VERIFY2(valid_pos(Position()), dbg_valide_pos_string(Position(), this, "state_Flying		(float fdt)"));
}

static Fvector vV = {0, 0, 0};
void CAI_Crow::state_DeathFall()
{
    Fvector tAcceleration;
    tAcceleration.set(0, -10.f, 0);
    if (m_pPhysicsShell)
    {
        Fvector velocity;
        m_pPhysicsShell->get_LinearVel(velocity);
        if (velocity.y > -0.001f)
            st_target = eDeathDead;
    }
    else
        st_target = eDeathDead;

    if (bPlayDeathIdle)
    {
        smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(m_Anims.m_death_idle.GetRandom());
        bPlayDeathIdle = false;
    }
    VERIFY2(valid_pos(Position()), dbg_valide_pos_string(Position(), this, "CAI_Crow::state_DeathFall()"));
}

void CAI_Crow::Die(IGameObject* who)
{
    inherited::Die(who);
    processing_activate(); // enable UpdateCL for dead crows - especially for physics support
    // and do it especially before Creating physics shell or it definitely throws processing enable/disable calls:
    // underflow
    CreateSkeleton();

    const CGameObject* who_object = smart_cast<const CGameObject*>(who);
    callback(GameObject::eDeath)(lua_game_object(), who_object ? who_object->lua_game_object() : 0);
};
void CAI_Crow::UpdateWorkload(float fdt)
{
    if (o_workload_frame == Device.dwFrame)
        return;
    o_workload_frame = Device.dwFrame;
    switch (st_current)
    {
    case eFlyIdle:
    case eFlyUp: state_Flying(fdt); break;
    case eDeathFall: state_DeathFall(); break;
    }
}
void CAI_Crow::UpdateCL()
{
    inherited::UpdateCL();
    VERIFY2(valid_pos(Position()), dbg_valide_pos_string(Position(), this, " CAI_Crow::UpdateCL		()"));
    if (m_pPhysicsShell)
    {
        m_pPhysicsShell->Update();
        XFORM().set(m_pPhysicsShell->mXFORM);
    }
}
void CAI_Crow::renderable_Render()
{
    UpdateWorkload(Device.fTimeDelta * (Device.dwFrame - o_workload_frame));
    inherited::renderable_Render();
    o_workload_rframe = Device.dwFrame;
}
void CAI_Crow::shedule_Update(u32 DT)
{
    float fDT = float(DT) / 1000.F;
    spatial.type &= ~STYPE_VISIBLEFORAI;

    inherited::shedule_Update(DT);

    if (st_target != st_current)
    {
        switch (st_target)
        {
        case eFlyUp: switch2_FlyUp(); break;
        case eFlyIdle: switch2_FlyIdle(); break;
        case eDeathFall: switch2_DeathFall(); break;
        case eDeathDead: switch2_DeathDead(); break;
        }
        st_current = st_target;
    }

    switch (st_current)
    {
    case eFlyIdle:
        if (Position().y > vOldPosition.y)
            st_target = eFlyUp;
        break;
    case eFlyUp:
        if (Position().y <= vOldPosition.y)
            st_target = eFlyIdle;
        break;
    case eDeathFall: state_DeathFall(); break;
    }
    if ((eDeathFall != st_current) && (eDeathDead != st_current))
    {
        // At random times, change the direction (goal) of the plane
        if (fGoalChangeTime <= 0)
        {
            fGoalChangeTime += fGoalChangeDelta + fGoalChangeDelta * Random.randF(-0.5f, 0.5f);
            Fvector vP = Actor()->Position();
            vP.y += +fMinHeight;
            vGoalDir.x = vP.x + vVarGoal.x * Random.randF(-0.5f, 0.5f);
            vGoalDir.y = vP.y + vVarGoal.y * Random.randF(-0.5f, 0.5f);
            vGoalDir.z = vP.z + vVarGoal.z * Random.randF(-0.5f, 0.5f);
        }
        fGoalChangeTime -= fDT;
        // sounds
        if (fIdleSoundTime <= 0)
        {
            fIdleSoundTime = fIdleSoundDelta + fIdleSoundDelta * Random.randF(-0.5f, 0.5f);
            // if (st_current==eFlyIdle)
            GEnv.Sound->play_at_pos(m_Sounds.m_idle.GetRandom(), H_Root(), Position());
        }
        fIdleSoundTime -= fDT;
    }
    m_Sounds.m_idle.SetPosition(Position());

    // work
    if (o_workload_rframe >= (Device.dwFrame - 2))
        ;
    else
        UpdateWorkload(fDT);
    VERIFY2(valid_pos(Position()), dbg_valide_pos_string(Position(), this, " CAI_Crow::shedule_Update		(u32 DT)"));
}

// Core events
void CAI_Crow::net_Export(NET_Packet& P) // export to server
{
    R_ASSERT(Local());

    u8 flags = 0;
    P.w_float(GetfHealth());

    P.w_u32(Level().timeServer());
    P.w_u8(flags);

    P.w_vec3(Position());

    float yaw, pitch, bank;
    XFORM().getHPB(yaw, pitch, bank);

    P.w_float(yaw);

    P.w_float(yaw);
    P.w_float(pitch);
    P.w_float(0);

    P.w_u8(u8(g_Team()));
    P.w_u8(u8(g_Squad()));
    P.w_u8(u8(g_Group()));
}
//---------------------------------------------------------------------
void CAI_Crow::net_Import(NET_Packet& P)
{
    R_ASSERT(Remote());

    float health;
    P.r_float(health);
    SetfHealth(health);

    P.r_u32();
    P.r_u8();

    P.r_vec3(Position());

    float yaw, pitch, bank = 0, roll = 0;

    P.r_float(yaw);
    P.r_float(yaw);
    P.r_float(pitch);
    P.r_float(roll);

    id_Team = P.r_u8();
    id_Squad = P.r_u8();
    id_Group = P.r_u8();

    XFORM().setHPB(yaw, pitch, bank);
    VERIFY2(valid_pos(Position()), dbg_valide_pos_string(Position(), this, " CAI_Crow::net_Import	(NET_Packet& P)"));
}
//---------------------------------------------------------------------
void CAI_Crow::HitSignal(float /**HitAmount/**/, Fvector& /**local_dir/**/, IGameObject* who, s16 /**element/**/)
{
    // bool				first_time = !!g_Alive();
    //	bool				first_time = !PPhysicsShell();
    SetfHealth(0);
    // set_death_time		()	;
    if (eDeathDead != st_current)
    {
        //		if (first_time)	Die			(who);
        st_target = eDeathFall;
    }
    else
        smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(m_Anims.m_death_dead.GetRandom());
}
//---------------------------------------------------------------------
void CAI_Crow::HitImpulse(float /**amount/**/, Fvector& /**vWorldDir/**/, Fvector& /**vLocalDir/**/) {}
//---------------------------------------------------------------------
void CAI_Crow::CreateSkeleton()
{
    m_pPhysicsShell = P_build_Shell(this, false, (BONE_P_MAP*)0); // P_build_SimpleShell(this,0.3f,false);
    m_pPhysicsShell->SetMaterial(smart_cast<IKinematics*>(
        Visual())->LL_GetData(smart_cast<IKinematics*>(Visual())->LL_GetBoneRoot())
                                     .game_mtl_idx);
}

// void CAI_Crow::Hit	(float P, Fvector &dir, IGameObject* who, s16 element,Fvector p_in_object_space, float impulse,
// ALife::EHitType hit_type)
void CAI_Crow::Hit(SHit* pHDS)
{
    //	inherited::Hit	(P,dir,who,element,p_in_object_space,impulse/100.f, hit_type);
    SHit HDS = *pHDS;
    HDS.impulse /= 100.f;
    inherited::Hit(&HDS);

    const CGameObject* who_object = smart_cast<const CGameObject*>(pHDS->who);
    callback(GameObject::eHit)(lua_game_object(), who_object ? who_object->lua_game_object() : 0);
}

BOOL CAI_Crow::UsedAI_Locations() { return (FALSE); }
void CAI_Crow::create_physic_shell()
{
    // do not delete!!!
}
