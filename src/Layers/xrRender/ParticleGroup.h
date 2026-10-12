#pragma once

#include "Layers/xrRender/dxParticleCustom.h"
#include "Layers/xrRender/ParticleGroupDef.h"

namespace xray::render::RENDER_NAMESPACE
{
namespace PS
{
class CParticleEffect;

class ECORE_API CParticleGroup : public dxParticleCustom
{
    const CPGDef* m_Def{};
    float m_CurrentTime{};
    Fvector m_InitialPosition{};
    Lock render_lock{};

public:
    using VisualVec = xr_vector<dxRender_Visual*>;
    struct SItem
    {
        dxRender_Visual* _effect;
        VisualVec _children_related;
        VisualVec _children_free;

        void Set(dxRender_Visual* e);
        void Clear();

        u32 GetVisuals(xr_vector<dxRender_Visual*>& visuals)
        {
            visuals.reserve(_children_related.size() + _children_free.size() + 1);
            if (_effect)
                visuals.push_back(_effect);
            visuals.insert(visuals.end(), _children_related.begin(), _children_related.end());
            visuals.insert(visuals.end(), _children_free.begin(), _children_free.end());
            return visuals.size();
        }

        void OnDeviceCreate();
        void OnDeviceDestroy();

        void StartRelatedChild(CParticleEffect* emitter, LPCSTR eff_name, PAPI::Particle& m);
        void StopRelatedChild(u32 idx);
        void StartFreeChild(CParticleEffect* emitter, LPCSTR eff_name, PAPI::Particle& m);

        void UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL bXFORM);
        void OnFrame(u32 u_dt, const CPGDef::SEffect& def, Fbox& box, bool& bPlaying);

        u32 ParticlesCount();
        bool IsPlaying() const;
        void Play();
        void Stop(BOOL def_stop);
    };
    using SItemVec = xr_vector<SItem>;
    SItemVec items;

public:
    enum
    {
        flRT_Playing = (1 << 0),
        flRT_DefferedStop = (1 << 1),
    };
    Flags8 m_RT_Flags;

public:
    CParticleGroup();
    virtual ~CParticleGroup();
    virtual void OnFrame(u32 dt);

    virtual void Copy(dxRender_Visual* /*pFrom*/) { FATAL("Can't duplicate particle system - NOT IMPLEMENTED"); }
    virtual void OnDeviceCreate();
    virtual void OnDeviceDestroy();

    virtual void UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL bXFORM);

    BOOL Compile(CPGDef* def);

    const CPGDef* GetDefinition() { return m_Def; }
    virtual void Play();
    virtual void Stop(BOOL bDefferedStop = TRUE);
    virtual BOOL IsPlaying() { return m_RT_Flags.is(flRT_Playing); }
    virtual void SetHudMode(BOOL b);
    virtual BOOL GetHudMode();

    virtual float GetTimeLimit()
    {
        VERIFY(m_Def);
        return m_Def->m_fTimeLimit;
    }

    virtual const shared_str Name()
    {
        VERIFY(m_Def);
        return m_Def->m_Name;
    }

    virtual u32 ParticlesCount();
};
}
//----------------------------------------------------
#define PGD_VERSION 0x0003
#define PGD_CHUNK_VERSION 0x0001
#define PGD_CHUNK_NAME 0x0002
#define PGD_CHUNK_FLAGS 0x0003
#define PGD_CHUNK_EFFECTS 0x0004 // obsolete
#define PGD_CHUNK_TIME_LIMIT 0x0005
#define PGD_CHUNK_EFFECTS2 0x0007
} // namespace xray::render::RENDER_NAMESPACE
