#include "stdafx.h"

#include "particle_manager.h"
#include "particle_effect.h"
#include "particle_actions_collection.h"

using namespace PAPI;

// system
CParticleManager PM;
PARTICLES_API IParticleManager* PAPI::ParticleManager() { return &PM; }
//
CParticleManager::CParticleManager() {}
CParticleManager::~CParticleManager() {}

ParticleEffect* CParticleManager::GetEffectPtr(int effect_id)
{
    R_ASSERT(effect_id >= 0 && effect_id < (int)effect_vec.size());
    return effect_vec[effect_id];
}

ParticleActions* CParticleManager::GetActionListPtr(int a_list_num)
{
    R_ASSERT(a_list_num >= 0 && a_list_num < (int)m_alist_vec.size());
    return m_alist_vec[a_list_num];
}

// create
int CParticleManager::CreateEffect(u32 max_particles)
{
    int eff_id = -1;
    for (int i = 0; i < (int)effect_vec.size(); i++)
        if (!effect_vec[i])
        {
            eff_id = i;
            break;
        }

    if (eff_id < 0)
    {
        // Couldn't find a big enough gap. Reallocate.
        eff_id = effect_vec.size();
        effect_vec.push_back(nullptr);
    }

    effect_vec[eff_id] = new ParticleEffect(max_particles);

    return eff_id;
}

void CParticleManager::DestroyEffect(int effect_id)
{
    R_ASSERT(effect_id >= 0 && effect_id < (int)effect_vec.size());
    xr_delete(effect_vec[effect_id]);
}

int CParticleManager::CreateActionList()
{
    int list_id = -1;
    for (u32 i = 0; i < m_alist_vec.size(); ++i)
        if (!m_alist_vec[i])
        {
            list_id = i;
            break;
        }

    if (list_id < 0)
    {
        // Couldn't find a big enough gap. Reallocate.
        list_id = m_alist_vec.size();
        m_alist_vec.push_back(nullptr);
    }

    m_alist_vec[list_id] = new ParticleActions();

    return list_id;
}

void CParticleManager::DestroyActionList(int alist_id)
{
    R_ASSERT(alist_id >= 0 && alist_id < (int)m_alist_vec.size());
    xr_delete(m_alist_vec[alist_id]);
}

// control
void CParticleManager::PlayEffect(int effect_id, int alist_id)
{
    // effect
    //ParticleEffect* pe = GetEffectPtr(effect_id);
    // Execute the specified action list.
    ParticleActions* pa = GetActionListPtr(alist_id);
    VERIFY(pa);
    if (pa == nullptr)
        return; // ERROR
    pa->lock();
    // Step through all the actions in the action list.
    for (auto& it : *pa)
    {
        VERIFY(it);
        switch (it->type)
        {
        case PASourceID: static_cast<PASource*>(it)->m_Flags.set(PASource::flSilent, false);
            break;
        case PAExplosionID: static_cast<PAExplosion*>(it)->age = 0.f;
            break;
        case PATurbulenceID: static_cast<PATurbulence*>(it)->age = 0.f;
            break;
        }
    }
    pa->unlock();
}

void CParticleManager::StopEffect(int effect_id, int alist_id, bool deffered)
{
    // Execute the specified action list.
    ParticleActions* pa = GetActionListPtr(alist_id);
    VERIFY(pa);
    if (pa == nullptr)
        return; // ERROR
    pa->lock();

    // Step through all the actions in the action list.
    for (auto& it : *pa)
    {
        switch (it->type)
        {
        case PASourceID: static_cast<PASource*>(it)->m_Flags.set(PASource::flSilent, true);
            break;
        }
    }
    if (!deffered)
    {
        // effect
        ParticleEffect* pe = GetEffectPtr(effect_id);
        pe->p_count = 0;
    }
    pa->unlock();
}

// update&render
void CParticleManager::Update(int effect_id, int alist_id, float dt)
{
    ParticleEffect* pe = GetEffectPtr(effect_id);
    ParticleActions* pa = GetActionListPtr(alist_id);

    VERIFY(pa);
    VERIFY(pe);

    pa->lock();

    // Step through all the actions in the action list.
    float kill_old_time = 1.0f;
    for (auto& it : *pa)
    {
        VERIFY(it);
        it->Execute(pe, dt, kill_old_time);
    }
    pa->unlock();
}

void CParticleManager::Render(int effect_id)
{
    //    ParticleEffect* pe	= GetEffectPtr(effect_id);
}

void CParticleManager::Transform(int alist_id, const Fmatrix& full, const Fvector& vel)
{
    // Execute the specified action list.
    ParticleActions* pa = GetActionListPtr(alist_id);
    VERIFY(pa);

    if (pa == nullptr)
        return; // ERROR
    pa->lock();

    Fmatrix mT;
    mT.translate(full.c);

    // Step through all the actions in the action list.
    for (auto& it : *pa)
    {
        bool r = it->m_Flags.is(ParticleAction::ALLOW_ROTATE);
        const Fmatrix& m = r ? full : mT;
        it->Transform(m);
        switch (it->type)
        {
        case PASourceID:
            static_cast<PASource*>(it)->parent_vel =
                pVector(vel.x, vel.y, vel.z) * static_cast<PASource*>(it)->parent_motion;
            break;
        }
    }
    pa->unlock();
}

// effect
void CParticleManager::RemoveParticle(int effect_id, u32 p_id)
{
    ParticleEffect* pe = GetEffectPtr(effect_id);
    pe->Remove(p_id);
}

void CParticleManager::SetMaxParticles(int effect_id, u32 max_particles)
{
    ParticleEffect* pe = GetEffectPtr(effect_id);
    pe->Resize(max_particles);
}

void CParticleManager::SetCallback(int effect_id, OnBirthParticleCB b, OnDeadParticleCB d, void* owner, u32 param)
{
    ParticleEffect* pe = GetEffectPtr(effect_id);
    pe->b_cb = b;
    pe->d_cb = d;
    pe->owner = owner;
    pe->param = param;
}

void CParticleManager::GetParticles(int effect_id, Particle*& particles, u32& cnt)
{
    ParticleEffect* pe = GetEffectPtr(effect_id);
    particles = pe->particles;
    cnt = pe->p_count;
}

u32 CParticleManager::GetParticlesCount(int effect_id)
{
    ParticleEffect* pe = GetEffectPtr(effect_id);
    return pe->p_count;
}

// action
ParticleAction* CParticleManager::CreateAction(PActionEnum type)
{
    ParticleAction* pa = nullptr;
    switch (type)
    {
    case PAAvoidID: pa = new PAAvoid();
        break;
    case PABounceID: pa = new PABounce();
        break;
    case PACopyVertexBID: pa = new PACopyVertexB();
        break;
    case PADampingID: pa = new PADamping();
        break;
    case PAExplosionID: pa = new PAExplosion();
        break;
    case PAFollowID: pa = new PAFollow();
        break;
    case PAGravitateID: pa = new PAGravitate();
        break;
    case PAGravityID: pa = new PAGravity();
        break;
    case PAJetID: pa = new PAJet();
        break;
    case PAKillOldID: pa = new PAKillOld();
        break;
    case PAMatchVelocityID: pa = new PAMatchVelocity();
        break;
    case PAMoveID: pa = new PAMove();
        break;
    case PAOrbitLineID: pa = new PAOrbitLine();
        break;
    case PAOrbitPointID: pa = new PAOrbitPoint();
        break;
    case PARandomAccelID: pa = new PARandomAccel();
        break;
    case PARandomDisplaceID: pa = new PARandomDisplace();
        break;
    case PARandomVelocityID: pa = new PARandomVelocity();
        break;
    case PARestoreID: pa = new PARestore();
        break;
    case PASinkID: pa = new PASink();
        break;
    case PASinkVelocityID: pa = new PASinkVelocity();
        break;
    case PASourceID: pa = new PASource();
        break;
    case PASpeedLimitID: pa = new PASpeedLimit();
        break;
    case PATargetColorID: pa = new PATargetColor();
        break;
    case PATargetSizeID: pa = new PATargetSize();
        break;
    case PATargetRotateID: pa = new PATargetRotate();
        break;
    case PATargetRotateDID: pa = new PATargetRotate();
        break;
    case PATargetVelocityID: pa = new PATargetVelocity();
        break;
    case PATargetVelocityDID: pa = new PATargetVelocity();
        break;
    case PAVortexID: pa = new PAVortex();
        break;
    case PATurbulenceID: pa = new PATurbulence();
        break;
    case PAScatterID: pa = new PAScatter();
        break;
    default: NODEFAULT;
    }
    pa->type = type;
    return pa;
}

size_t CParticleManager::LoadActions(int alist_id, IReader& R)
{
    // Execute the specified action list.
    ParticleActions* pa = GetActionListPtr(alist_id);
    VERIFY(pa);
    pa->clear();
    if (R.length())
    {
        u32 cnt = R.r_u32();
        for (u32 k = 0; k < cnt; k++)
        {
            ParticleAction* act = CreateAction(PActionEnum(R.r_u32()));
            act->Load(R);
            pa->append(act);
        }
    }
    return pa->size();
}

void CParticleManager::SaveActions(int alist_id, IWriter& W)
{
    // Execute the specified action list.
    ParticleActions* pa = GetActionListPtr(alist_id);
    VERIFY(pa);
    pa->lock();
    W.w_u32(pa->size());
    for (auto& it : *pa)
        it->Save(W);
    pa->unlock();
}
