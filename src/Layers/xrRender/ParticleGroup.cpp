#include "stdafx.h"

#include "xrParticles/psystem.h"

#ifndef _EDITOR
#include "xrServerEntities/smart_cast.h"
#endif

#include "ParticleGroup.h"
#include "PSLibrary.h"
#include "ParticleEffect.h"

using namespace PS;

//------------------------------------------------------------------------------
CPGDef::CPGDef()
{
    m_Flags.zero();
    m_fTimeLimit = 0.f;
}

CPGDef::~CPGDef()
{
    for (auto& e : m_Effects)
        xr_delete(e);
    m_Effects.clear();
}

void CPGDef::SetName(LPCSTR name) { m_Name = name; }
#ifdef _EDITOR
void CPGDef::Clone(CPGDef* source)
{
    m_Name = "<invalid_name>";
    m_Flags = source->m_Flags;
    m_fTimeLimit = source->m_fTimeLimit;

    m_Effects.resize(source->m_Effects.size(), 0);
    for (EffectIt d_it = m_Effects.begin(), s_it = source->m_Effects.begin(); s_it != source->m_Effects.end();
         s_it++, d_it++)
        *d_it = xr_new<SEffect>(**s_it);
}
#endif

//------------------------------------------------------------------------------
// I/O part
//------------------------------------------------------------------------------
BOOL CPGDef::Load(IReader& F)
{
    R_ASSERT(F.find_chunk(PGD_CHUNK_VERSION));
    u16 version = F.r_u16();

    if (version != PGD_VERSION)
    {
        Log("!Unsupported PG version. Load failed.");
        return FALSE;
    }

    R_ASSERT(F.find_chunk(PGD_CHUNK_NAME));
    F.r_stringZ(m_Name);

    F.r_chunk(PGD_CHUNK_FLAGS, &m_Flags);

    if (F.find_chunk(PGD_CHUNK_TIME_LIMIT))
        m_fTimeLimit = F.r_float();
    else
        m_fTimeLimit = 0.0f;

    bool dont_calc_timelimit = m_fTimeLimit > 0.0f;
    if (F.find_chunk(PGD_CHUNK_EFFECTS))
    {
        m_Effects.resize(F.r_u32());
        for (auto& e : m_Effects)
        {
            e = xr_new<SEffect>();
            F.r_stringZ(e->m_EffectName);
            F.r_stringZ(e->m_OnPlayChildName);
            F.r_stringZ(e->m_OnBirthChildName);
            F.r_stringZ(e->m_OnDeadChildName);
            e->m_Time0 = F.r_float();
            e->m_Time1 = F.r_float();
            e->m_Flags.assign(F.r_u32());

            if (!dont_calc_timelimit)
                m_fTimeLimit = _max(m_fTimeLimit, e->m_Time1);
        }
    }
    return TRUE;
}

BOOL CPGDef::Load2(CInifile& ini)
{
    //u16 version = ini.r_u16("_group", "version");
    m_Flags.assign(ini.r_u32("_group", "flags"));

    m_Effects.resize(ini.r_u32("_group", "effects_count"));

    u32 counter = 0;
    string256 buff;
    for (auto it = m_Effects.begin(); it != m_Effects.end(); ++it, ++counter)
    {
        *it = xr_new<SEffect>();

        xr_sprintf(buff, sizeof(buff), "effect_%04d", counter);

        (*it)->m_EffectName = ini.r_string(buff, "effect_name");
        (*it)->m_OnPlayChildName = ini.r_string(buff, "on_play_child");
        (*it)->m_OnBirthChildName = ini.r_string(buff, "on_birth_child");
        (*it)->m_OnDeadChildName = ini.r_string(buff, "on_death_child");

        (*it)->m_Time0 = ini.r_float(buff, "time0");
        (*it)->m_Time1 = ini.r_float(buff, "time1");
        (*it)->m_Flags.assign(ini.r_u32(buff, "flags"));
    }
    m_fTimeLimit = ini.r_float("_group", "timelimit");
    return TRUE;
}

void CPGDef::Save(IWriter& F)
{
    F.open_chunk(PGD_CHUNK_VERSION);
    F.w_u16(PGD_VERSION);
    F.close_chunk();

    F.open_chunk(PGD_CHUNK_NAME);
    F.w_stringZ(m_Name);
    F.close_chunk();

    F.w_chunk(PGD_CHUNK_FLAGS, &m_Flags, sizeof(m_Flags));

    F.open_chunk(PGD_CHUNK_EFFECTS);
    F.w_u32(m_Effects.size());
    for (auto& e : m_Effects)
    {
        F.w_stringZ(e->m_EffectName);
        F.w_stringZ(e->m_OnPlayChildName);
        F.w_stringZ(e->m_OnBirthChildName);
        F.w_stringZ(e->m_OnDeadChildName);
        F.w_float(e->m_Time0);
        F.w_float(e->m_Time1);
        F.w_u32(e->m_Flags.get());
    }
    F.close_chunk();

    F.open_chunk(PGD_CHUNK_TIME_LIMIT);
    F.w_float(m_fTimeLimit);
    F.close_chunk();
}

void CPGDef::Save2(CInifile& ini)
{
    ini.w_u16("_group", "version", PGD_VERSION);

    ini.w_u32("_group", "flags", m_Flags.get());

    ini.w_u32("_group", "effects_count", m_Effects.size());

    u32 counter = 0;
    string256 buff;
    for (auto it = m_Effects.begin(); it != m_Effects.end(); ++it, ++counter)
    {
        xr_sprintf(buff, sizeof(buff), "effect_%04d", counter);

        ini.w_string(buff, "effect_name", (*it)->m_EffectName.c_str());
        ini.w_string(
            buff, "on_play_child", (*it)->m_Flags.test(SEffect::flOnPlayChild) ? (*it)->m_OnPlayChildName.c_str() : "");
        ini.w_string(buff, "on_birth_child",
                     (*it)->m_Flags.test(SEffect::flOnBirthChild) ? (*it)->m_OnBirthChildName.c_str() : "");
        ini.w_string(buff, "on_death_child",
                     (*it)->m_Flags.test(SEffect::flOnDeadChild) ? (*it)->m_OnDeadChildName.c_str() : "");
        ini.w_float(buff, "time0", (*it)->m_Time0);
        ini.w_float(buff, "time1", (*it)->m_Time1);
        ini.w_u32(buff, "flags", (*it)->m_Flags.get());
    }

    ini.w_float("_group", "timelimit", m_fTimeLimit);
}
//------------------------------------------------------------------------------
// Particle Group item
//------------------------------------------------------------------------------
void CParticleGroup::SItem::Set(dxRender_Visual* e) { _effect = e; }
void CParticleGroup::SItem::Clear()
{
    VisualVec visuals;
    GetVisuals(visuals);
    for (auto& visual : visuals)
        RImplementation.model_Delete((IRenderVisual*&)visual, false);

    //	Igor: zero all pointers! Previous code didn't zero _source_ pointers,
    //	just temporary ones.
    _effect = nullptr;
    _children_related.clear();
    _children_free.clear();
}
void CParticleGroup::SItem::StartRelatedChild(CParticleEffect* emitter, LPCSTR eff_name, PAPI::Particle& m)
{
    CParticleEffect* C = static_cast<CParticleEffect*>(RImplementation.model_CreatePE(eff_name));

    C->SetHudMode(emitter->GetHudMode());

    Fmatrix M;
    M.identity();
    Fvector vel;
    vel.sub(m.pos, m.posB);
    vel.div(fDT_STEP);
    if (emitter->m_RT_Flags.is(CParticleEffect::flRT_XFORM))
    {
        M.set(emitter->m_XFORM);
        M.transform_dir(vel);
    };
    Fvector p;
    M.transform_tiny(p, m.pos);
    M.c.set(p);
    C->Play();
    C->UpdateParent(M, vel, FALSE);
    _children_related.push_back(C);
}
void CParticleGroup::SItem::StopRelatedChild(u32 idx)
{
    VERIFY(idx < _children_related.size());
    dxRender_Visual*& V = _children_related[idx];
    ((CParticleEffect*)V)->Stop(TRUE);
    _children_free.push_back(V);
    _children_related[idx] = _children_related.back();
    _children_related.pop_back();
}
void CParticleGroup::SItem::StartFreeChild(CParticleEffect* emitter, LPCSTR nm, PAPI::Particle& m)
{
    CParticleEffect* C = static_cast<CParticleEffect*>(RImplementation.model_CreatePE(nm));
    C->SetHudMode(emitter->GetHudMode());
    if (!C->IsLooped())
    {
        Fmatrix M;
        M.identity();
        Fvector vel;
        vel.sub(m.pos, m.posB);
        vel.div(fDT_STEP);
        if (emitter->m_RT_Flags.is(CParticleEffect::flRT_XFORM))
        {
            M.set(emitter->m_XFORM);
            M.transform_dir(vel);
        };
        Fvector p;
        M.transform_tiny(p, m.pos);
        M.c.set(p);
        C->Play();
        C->UpdateParent(M, vel, FALSE);
        _children_free.push_back(C);
    }
    else
    {
#ifdef _EDITOR
        Msg("! Can't use looped effect '%s' as 'On Birth' child for group.", nm);
#else
        xrDebug::Fatal(DEBUG_INFO, "Can't use looped effect '%s' as 'On Birth' child for group.", nm);
#endif
    }
}
void CParticleGroup::SItem::Play()
{
    CParticleEffect* E = static_cast<CParticleEffect*>(_effect);
    if (E)
        E->Play();
}
void CParticleGroup::SItem::Stop(BOOL def_stop)
{
    // stop all effects
    CParticleEffect* E = static_cast<CParticleEffect*>(_effect);
    if (E)
        E->Stop(def_stop);

    for (auto& p : _children_related)
        static_cast<CParticleEffect*>(p)->Stop(def_stop);
    for (auto& p : _children_free)
        static_cast<CParticleEffect*>(p)->Stop(def_stop);

    // and delete if !deffered
    if (!def_stop)
    {
        for (auto& child : _children_related)
            RImplementation.model_Delete((IRenderVisual*&)child, false);

        for (auto& child : _children_free)
            RImplementation.model_Delete((IRenderVisual*&)child, false);

        _children_related.clear();
        _children_free.clear();
    }
}
bool CParticleGroup::SItem::IsPlaying() const
{
    CParticleEffect* E = static_cast<CParticleEffect*>(_effect);
    return E ? E->IsPlaying() : false;
}

void CParticleGroup::SItem::UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL bXFORM)
{
    CParticleEffect* E = static_cast<CParticleEffect*>(_effect);
    if (E)
        E->UpdateParent(m, velocity, bXFORM);
}

//------------------------------------------------------------------------------

void OnGroupParticleBirth(void* owner, u32 param, PAPI::Particle& m, u32 idx)
{
    CParticleGroup* PG = static_cast<CParticleGroup*>(owner);
    VERIFY(PG);

    CParticleEffect* PE = static_cast<CParticleEffect*>(PG->items[param]._effect);
    PS::OnEffectParticleBirth(PE, param, m, idx);

    // if have child
    const CPGDef* PGD = PG->GetDefinition();
    VERIFY(PGD);

    const CPGDef::SEffect* eff = PGD->m_Effects[param];

    if (eff->m_Flags.is(CPGDef::SEffect::flOnBirthChild))
        PG->items[param].StartFreeChild(PE, *eff->m_OnBirthChildName, m);

    if (eff->m_Flags.is(CPGDef::SEffect::flOnPlayChild))
        PG->items[param].StartRelatedChild(PE, *eff->m_OnPlayChildName, m);
}
void OnGroupParticleDead(void* owner, u32 param, PAPI::Particle& m, u32 idx)
{
    CParticleGroup* PG = static_cast<CParticleGroup*>(owner);
    VERIFY(PG);

    CParticleEffect* PE = static_cast<CParticleEffect*>(PG->items[param]._effect);
    PS::OnEffectParticleDead(PE, param, m, idx);

    // if have child
    const CPGDef* PGD = PG->GetDefinition();
    VERIFY(PGD);

    const CPGDef::SEffect* eff = PGD->m_Effects[param];

    if (eff->m_Flags.is(CPGDef::SEffect::flOnPlayChild))
        PG->items[param].StopRelatedChild(idx);

    if (eff->m_Flags.is(CPGDef::SEffect::flOnDeadChild))
        PG->items[param].StartFreeChild(PE, *eff->m_OnDeadChildName, m);
}
//------------------------------------------------------------------------------
void CParticleGroup::SItem::OnFrame(u32 u_dt, const CPGDef::SEffect& def, Fbox& box, bool& bPlaying)
{
    CParticleEffect* E1 = static_cast<CParticleEffect*>(_effect);
    if (E1)
    {
        E1->OnFrame(u_dt);
        if (E1->IsPlaying())
        {
            bPlaying = true;
            if (E1->vis.box.is_valid())
                box.merge(E1->vis.box);
            if (def.m_Flags.is(CPGDef::SEffect::flOnPlayChild) && def.m_OnPlayChildName.size())
            {
                PAPI::Particle* particles;
                u32 p_cnt;
                PAPI::ParticleManager()->GetParticles(E1->GetHandleEffect(), particles, p_cnt);
                VERIFY(p_cnt == _children_related.size());
                if (p_cnt)
                {
                    for (u32 i = 0; i < p_cnt; i++)
                    {
                        PAPI::Particle& m = particles[i];
                        CParticleEffect* C = static_cast<CParticleEffect*>(_children_related[i]);
                        Fmatrix M;
                        M.translate(m.pos);
                        Fvector vel;
                        vel.sub(m.pos, m.posB);
                        vel.div(fDT_STEP);
                        C->UpdateParent(M, vel, FALSE);
                    }
                }
            }
        }
    }

    if (!_children_related.empty())
    {
        for (auto& p : _children_related)
        {
            CParticleEffect* E = static_cast<CParticleEffect*>(p);
            if (E)
            {
                E->OnFrame(u_dt);
                if (E->IsPlaying())
                {
                    bPlaying = true;
                    if (E->vis.box.is_valid())
                        box.merge(E->vis.box);
                }
                else
                {
                    if (def.m_Flags.is(CPGDef::SEffect::flOnPlayChildRewind))
                    {
                        E->Play();
                    }
                }
            }
        }
    }
    if (!_children_free.empty())
    {
        u32 rem_cnt = 0;
        for (auto& child : _children_free)
        {
            CParticleEffect* E = static_cast<CParticleEffect*>(child);
            if (!E)
                continue;

            E->OnFrame(u_dt);
            if (E->IsPlaying())
            {
                bPlaying = true;
                if (E->vis.box.is_valid())
                    box.merge(E->vis.box);
            }
            else
            {
                rem_cnt++;
                RImplementation.model_Delete((IRenderVisual*&)child, false);
            }
        }
        // remove if stopped
        if (rem_cnt)
        {
            const auto new_end = std::remove_if(_children_free.begin(), _children_free.end(), [](const dxRender_Visual* x)
            {
                return x == nullptr;
            });
            _children_free.erase(new_end, _children_free.end());
        }
    }
    //	Msg("C: %d CS: %d",_children.size(),_children_stopped.size());
}
void CParticleGroup::SItem::OnDeviceCreate()
{
    VisualVec visuals;
    GetVisuals(visuals);
    for (auto& visual : visuals)
        static_cast<CParticleEffect*>(visual)->OnDeviceCreate();
}
void CParticleGroup::SItem::OnDeviceDestroy()
{
    VisualVec visuals;
    GetVisuals(visuals);
    for (auto& visual : visuals)
        static_cast<CParticleEffect*>(visual)->OnDeviceDestroy();
}
u32 CParticleGroup::SItem::ParticlesCount()
{
    u32 p_count = 0;
    VisualVec visuals;
    GetVisuals(visuals);
    for (auto& visual : visuals)
        p_count += static_cast<CParticleEffect*>(visual)->ParticlesCount();
    return p_count;
}

//------------------------------------------------------------------------------
// Particle Group part
//------------------------------------------------------------------------------
CParticleGroup::CParticleGroup()
{
    m_RT_Flags.zero();
}

CParticleGroup::~CParticleGroup()
{
    // Msg ("!!! destroy PG");
    for (auto& pg : items)
        pg.Clear();
    items.clear();
}

void CParticleGroup::OnFrame(u32 u_dt)
{
    ScopeLock lock{ &render_lock };
    if (m_Def && m_RT_Flags.is(flRT_Playing))
    {
        float ct = m_CurrentTime;
        float f_dt = float(u_dt) / 1000.f;
        for (CPGDef::EffectVec::const_iterator e_it = m_Def->m_Effects.begin(); e_it != m_Def->m_Effects.end(); ++e_it)
        {
            if ((*e_it)->m_Flags.is(CPGDef::SEffect::flEnabled))
            {
                VERIFY(items.size() == m_Def->m_Effects.size());
                SItem& I = items[e_it - m_Def->m_Effects.begin()];
                if (I.IsPlaying())
                {
                    if ((ct <= (*e_it)->m_Time1) && (ct + f_dt >= (*e_it)->m_Time1))
                        I.Stop((*e_it)->m_Flags.is(CPGDef::SEffect::flDefferedStop));
                }
                else
                {
                    if (!m_RT_Flags.is(flRT_DefferedStop))
                        if ((ct <= (*e_it)->m_Time0) && (ct + f_dt >= (*e_it)->m_Time0))
                            I.Play();
                }
            }
        }
        m_CurrentTime += f_dt;
        if ((m_CurrentTime > m_Def->m_fTimeLimit) && (m_Def->m_fTimeLimit > 0.f))
            if (!m_RT_Flags.is(flRT_DefferedStop))
                Stop(true);

        bool bPlaying = false;
        Fbox box;
        box.invalidate();
        for (auto i_it = items.begin(); i_it != items.end(); ++i_it)
            i_it->OnFrame(u_dt, *m_Def->m_Effects[i_it - items.begin()], box, bPlaying);

        if (m_RT_Flags.is(flRT_DefferedStop) && !bPlaying)
        {
            m_RT_Flags.set(flRT_Playing | flRT_DefferedStop, FALSE);
        }
        if (box.is_valid())
        {
            vis.box.set(box);
            vis.box.getsphere(vis.sphere.P, vis.sphere.R);
        }
    }
    else
    {
        vis.box.set(m_InitialPosition, m_InitialPosition);
        vis.box.grow(EPS_L);
        vis.box.getsphere(vis.sphere.P, vis.sphere.R);
    }
}

void CParticleGroup::UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL bXFORM)
{
    ScopeLock lock{ &render_lock };
    m_InitialPosition = m.c;
    for (auto& item : items)
        item.UpdateParent(m, velocity, bXFORM);
}

BOOL CParticleGroup::Compile(CPGDef* def)
{
    m_Def = def;

    // destroy existing
    for (auto& item : items)
        item.Clear();
    items.clear();

    // create new
    if (m_Def)
    {
        items.resize(m_Def->m_Effects.size());
        for (CPGDef::EffectVec::const_iterator e_it = m_Def->m_Effects.begin(); e_it != m_Def->m_Effects.end(); ++e_it)
        {
            CParticleEffect* eff = (CParticleEffect*)RImplementation.model_CreatePE(*(*e_it)->m_EffectName);
            eff->SetBirthDeadCB(OnGroupParticleBirth, OnGroupParticleDead, this, u32(e_it - m_Def->m_Effects.begin()));
            items[e_it - def->m_Effects.begin()].Set(eff);
        }
    }
    return TRUE;
}

void CParticleGroup::Play()
{
    m_CurrentTime = 0;
    m_RT_Flags.set(flRT_DefferedStop, FALSE);
    m_RT_Flags.set(flRT_Playing, TRUE);
}

void CParticleGroup::Stop(BOOL bDefferedStop)
{
    ScopeLock lock{ &render_lock };
    if (bDefferedStop)
    {
        m_RT_Flags.set(flRT_DefferedStop, TRUE);
    }
    else
    {
        m_RT_Flags.set(flRT_Playing, FALSE);
    }
    for (auto& item : items)
        item.Stop(bDefferedStop);
}

void CParticleGroup::OnDeviceCreate()
{
    for (auto& item : items)
        item.OnDeviceCreate();
}

void CParticleGroup::OnDeviceDestroy()
{
    for (auto& item : items)
        item.OnDeviceDestroy();
}

u32 CParticleGroup::ParticlesCount()
{
    ScopeLock lock{ &render_lock };
    int p_count = 0;
    for (auto& item : items)
        p_count += item.ParticlesCount();
    return p_count;
}

void CParticleGroup::SetHudMode(BOOL b)
{
    ScopeLock lock{ &render_lock };
    for (auto& item : items)
    {
        CParticleEffect* E = static_cast<CParticleEffect*>(item._effect);
        E->SetHudMode(b);
    }
}

BOOL CParticleGroup::GetHudMode()
{
    ScopeLock lock{ &render_lock };
    if (items.size())
    {
        CParticleEffect* E = static_cast<CParticleEffect*>(items[0]._effect);
        return E->GetHudMode();
    }
    return false;
}
