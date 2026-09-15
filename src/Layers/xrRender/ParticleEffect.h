#pragma once

#include "ParticleEffectDef.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/dxParticleCustom.h"
#ifndef _EDITOR
#include "GpuParticleManager.h"
#endif

namespace xray::render::fg
{
namespace PS
{
class ECORE_API CParticleEffect : public dxParticleCustom
{
    friend class CPEDef;

#ifdef _EDITOR
    float m_fElapsedLimit;
    int m_HandleEffect;
    int m_HandleActionList;
    s32 m_MemDT;
    DestroyCallback m_DestroyCallback;
    CollisionCallback m_CollisionCallback;
#else
    GpuParticleHandle m_GpuHandle{};
    u32 m_SnapshotSerial{};
    u32 m_ParticleCount{};
    Fvector m_ParentVelocity{};
    void RefreshSnapshot();
#endif
    Fvector m_InitialPosition;
    void RefreshShader();

public:
    CPEDef* m_Def;
    Fmatrix m_XFORM;

    enum
    {
        flRT_Playing = (1 << 0),
        flRT_DefferedStop = (1 << 1),
        flRT_XFORM = (1 << 2),
        flRT_HUDmode = (1 << 3),
    };
    Flags8 m_RT_Flags;

    CParticleEffect();
    virtual ~CParticleEffect();
    void OnFrame(u32 dt);
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void OnDeviceCreate();
    virtual void OnDeviceDestroy();
    virtual void UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL bXFORM);
    BOOL Compile(CPEDef* def);
    CPEDef* GetDefinition() const { return m_Def; }
    virtual void Play();
    virtual void Stop(BOOL bDefferedStop = TRUE);
    virtual BOOL IsPlaying() { return m_RT_Flags.is(flRT_Playing); }
    virtual void SetHudMode(BOOL b);
    virtual BOOL GetHudMode() { return m_RT_Flags.is(flRT_HUDmode); }
    virtual float GetTimeLimit()
    {
        VERIFY(m_Def);
        return m_Def->m_Flags.is(CPEDef::dfTimeLimit) ? m_Def->m_fTimeLimit : -1.f;
    }
    virtual const shared_str Name()
    {
        VERIFY(m_Def);
        return m_Def->m_Name;
    }
#ifdef _EDITOR
    int GetHandleEffect() const { return m_HandleEffect; }
    int GetHandleActionList() { return m_HandleActionList; }
    void SetDestroyCB(DestroyCallback destroy_cb) { m_DestroyCallback = destroy_cb; }
    void SetCollisionCB(CollisionCallback collision_cb) { m_CollisionCallback = collision_cb; }
    void SetBirthDeadCB(PAPI::OnBirthParticleCB bc, PAPI::OnDeadParticleCB dc, void* owner, u32 p) const;
    void Render(float LOD, bool useFastGeo);
#else
    GpuParticleHandle GetGpuHandle() const { return m_GpuHandle; }
    void ConfigureChildren(const char* birth, const char* play, const char* death, u32 groupFlags);
#endif
    virtual u32 ParticlesCount();
};
#ifdef _EDITOR
void OnEffectParticleBirth(void* owner, u32 param, PAPI::Particle& m, u32 idx);
void OnEffectParticleDead(void* owner, u32 param, PAPI::Particle& m, u32 idx);
extern const u32 uDT_STEP;
extern const float fDT_STEP;
#endif
}
}
