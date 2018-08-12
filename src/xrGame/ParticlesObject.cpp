#include "StdAfx.h"

#include "ParticlesObject.h"
#include "xrEngine/defines.h"
#include "Include/xrRender/RenderVisual.h"
#include "Include/xrRender/ParticleCustom.h"
#include "xrEngine/Render.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"

const Fvector zero_vel = {0.f, 0.f, 0.f};

CParticlesObject::CParticlesObject(LPCSTR p_name, BOOL bAutoRemove, bool destroy_on_game_load)
    : inherited(destroy_on_game_load)
{
    Init(p_name, 0, bAutoRemove);
}

void CParticlesObject::Init(LPCSTR p_name, IRender_Sector* S, BOOL bAutoRemove)
{
    m_bLooped = false;
    m_bStopping = false;
    m_bAutoRemove = bAutoRemove;
    float time_limit = 0.0f;

    if (!GEnv.isDedicatedServer)
    {
        // create visual
        renderable.visual = GEnv.Render->model_CreateParticles(p_name);
        VERIFY(renderable.visual);
        IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
        VERIFY(V);
        time_limit = V->GetTimeLimit();
    }
    else
    {
        time_limit = 1.0f;
    }

    if (time_limit > 0.f)
    {
        m_iLifeTime = iFloor(time_limit * 1000.f);
    }
    else
    {
        if (bAutoRemove)
        {
            R_ASSERT3(!m_bAutoRemove, "Can't set auto-remove flag for looped particle system.", p_name);
        }
        else
        {
            m_iLifeTime = 0;
            m_bLooped = true;
        }
    }

    // spatial
    spatial.type = 0;
    spatial.sector = S;

    // sheduled
    shedule.t_min = 20;
    shedule.t_max = 50;
    shedule_register();

    dwLastTime = Device.dwTimeGlobal;
    mt_dt = 0;
}

//----------------------------------------------------
CParticlesObject::~CParticlesObject()
{
    VERIFY(0 == mt_dt);

    //	we do not need this since CPS_Instance does it
    //	shedule_unregister		();
}

void CParticlesObject::UpdateSpatial()
{
    if (GEnv.isDedicatedServer)
        return;

    // spatial	(+ workaround occasional bug inside particle-system)
    vis_data& vis = renderable.visual->getVisData();
    if (_valid(vis.sphere))
    {
        Fvector P;
        float R;
        renderable.xform.transform_tiny(P, vis.sphere.P);
        R = vis.sphere.R;
        if (0 == spatial.type)
        {
            // First 'valid' update - register
            spatial.type = STYPE_RENDERABLE;
            spatial.sphere.set(P, R);
            spatial_register();
        }
        else
        {
            BOOL bMove = FALSE;
            if (!P.similar(spatial.sphere.P, EPS_L * 10.f))
                bMove = TRUE;
            if (!fsimilar(R, spatial.sphere.R, 0.15f))
                bMove = TRUE;
            if (bMove)
            {
                spatial.sphere.set(P, R);
                spatial_move();
            }
        }
    }
}

const shared_str CParticlesObject::Name()
{
    if (GEnv.isDedicatedServer)
        return "";

    IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
    VERIFY(V);
    return (V) ? V->Name() : "";
}

//----------------------------------------------------
void CParticlesObject::Play(bool bHudMode)
{
    if (GEnv.isDedicatedServer)
        return;

    IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
    VERIFY(V);
    if (bHudMode)
        V->SetHudMode(bHudMode);

    V->Play();
    dwLastTime = Device.dwTimeGlobal - 33ul;
    mt_dt = 0;
    PerformAllTheWork(0);
    m_bStopping = false;
}

void CParticlesObject::play_at_pos(const Fvector& pos, BOOL xform)
{
    if (GEnv.isDedicatedServer)
        return;

    IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
    VERIFY(V);
    Fmatrix m;
    m.translate(pos);
    V->UpdateParent(m, zero_vel, xform);
    V->Play();
    dwLastTime = Device.dwTimeGlobal - 33ul;
    mt_dt = 0;
    PerformAllTheWork(0);
    m_bStopping = false;
}

void CParticlesObject::Stop(BOOL bDefferedStop)
{
    if (GEnv.isDedicatedServer)
        return;

    IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
    VERIFY(V);
    V->Stop(bDefferedStop);
    m_bStopping = true;
}

void CParticlesObject::shedule_Update(u32 _dt)
{
    inherited::shedule_Update(_dt);

    if (GEnv.isDedicatedServer)
        return;

    // Update
    if (m_bDead)
        return;
    u32 dt = Device.dwTimeGlobal - dwLastTime;
    if (dt)
    {
        if (0)
        { //.psDeviceFlags.test(mtParticles))	{    //. AlexMX comment this line// NO UNCOMMENT - DON'T WORK PROPERLY
            mt_dt = dt;
            fastdelegate::FastDelegate0<> delegate(this, &CParticlesObject::PerformAllTheWork_mt);
            Device.seqParallel.push_back(delegate);
        }
        else
        {
            mt_dt = 0;
            IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
            VERIFY(V);
            V->OnFrame(dt);
        }
        dwLastTime = Device.dwTimeGlobal;
    }
    UpdateSpatial();
}

void CParticlesObject::PerformAllTheWork(u32 _dt)
{
    if (GEnv.isDedicatedServer)
        return;

    // Update
    u32 dt = Device.dwTimeGlobal - dwLastTime;
    if (dt)
    {
        IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
        VERIFY(V);
        V->OnFrame(dt);
        dwLastTime = Device.dwTimeGlobal;
    }
    UpdateSpatial();
}

void CParticlesObject::PerformAllTheWork_mt()
{
    if (GEnv.isDedicatedServer)
        return;

    if (0 == mt_dt)
        return; //???
    IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
    VERIFY(V);
    V->OnFrame(mt_dt);
    mt_dt = 0;
}

void CParticlesObject::SetXFORM(const Fmatrix& m)
{
    if (GEnv.isDedicatedServer)
        return;

    IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
    VERIFY(V);
    V->UpdateParent(m, zero_vel, TRUE);
    renderable.xform.set(m);
    UpdateSpatial();
}

void CParticlesObject::UpdateParent(const Fmatrix& m, const Fvector& vel)
{
    if (GEnv.isDedicatedServer)
        return;

    IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
    VERIFY(V);
    V->UpdateParent(m, vel, FALSE);
    UpdateSpatial();
}

Fvector& CParticlesObject::Position()
{
    if (GEnv.isDedicatedServer)
    {
        static Fvector _pos = Fvector().set(0, 0, 0);
        return _pos;
    }
    vis_data& vis = renderable.visual->getVisData();
    return vis.sphere.P;
}

float CParticlesObject::shedule_Scale()
{
    if (GEnv.isDedicatedServer)
        return 5.0f;

    return Device.vCameraPosition.distance_to(Position()) / 200.f;
}

void CParticlesObject::renderable_Render()
{
    VERIFY(renderable.visual);
    u32 dt = Device.dwTimeGlobal - dwLastTime;
    if (dt)
    {
        IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
        VERIFY(V);
        V->OnFrame(dt);
        dwLastTime = Device.dwTimeGlobal;
    }

    GEnv.Render->set_Transform(&renderable.xform);
    GEnv.Render->add_Visual(renderable.visual);
}

bool CParticlesObject::IsAutoRemove()
{
    if (m_bAutoRemove)
        return true;
    else
        return false;
}
void CParticlesObject::SetAutoRemove(bool auto_remove)
{
    VERIFY(m_bStopping || !IsLooped());
    m_bAutoRemove = auto_remove;
}

//играются ли партиклы, отличается от PSI_Alive, тем что после
//остановки Stop партиклы могут еще доигрывать анимацию IsPlaying = true
bool CParticlesObject::IsPlaying()
{
    if (GEnv.isDedicatedServer)
        return false;

    IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual);
    VERIFY(V);
    return !!V->IsPlaying();
}
