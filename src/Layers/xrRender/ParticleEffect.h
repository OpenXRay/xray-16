#ifndef ParticleEffectH
#define ParticleEffectH
#include "ParticleEffectDef.h"
#ifdef _EDITOR
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/dxParticleCustom.h"
#else // _EDITOR
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/dxParticleCustom.h"
#endif // _EDITOR

namespace PS
{
class ECORE_API CParticleEffect : public dxParticleCustom
{
    //		friend void ParticleRenderStream( LPVOID lpvParams );
    friend class CPEDef;

protected:
    float m_fElapsedLimit;

    int m_HandleEffect;
    int m_HandleActionList;

    s32 m_MemDT;

    Fvector m_InitialPosition;

public:
    CPEDef* m_Def;
    Fmatrix m_XFORM;

protected:
    DestroyCallback m_DestroyCallback;
    CollisionCallback m_CollisionCallback;

public:
    enum
    {
        flRT_Playing = (1 << 0),
        flRT_DefferedStop = (1 << 1),
        flRT_XFORM = (1 << 2),
        flRT_HUDmode = (1 << 3),
    };
    Flags8 m_RT_Flags;

protected:
    BOOL SaveActionList(IWriter& F);
    BOOL LoadActionList(IReader& F);

    void RefreshShader();

public:
    CParticleEffect();
    virtual ~CParticleEffect();

    void OnFrame(u32 dt);

    u32 RenderTO();
    virtual void Render(float LOD);

private:
    void ParticleRenderStream(FVF::LIT* pv, u32 count, PAPI::Particle* particles);

public:
    virtual void Copy(dxRender_Visual* pFrom);

    virtual void OnDeviceCreate();
    virtual void OnDeviceDestroy();

    virtual void UpdateParent(const Fmatrix& m, const Fvector& velocity, BOOL bXFORM);

    BOOL Compile(CPEDef* def);

    CPEDef* GetDefinition() { return m_Def; }
    int GetHandleEffect() { return m_HandleEffect; }
    int GetHandleActionList() { return m_HandleActionList; }
    virtual void Play();
    virtual void Stop(BOOL bDefferedStop = TRUE);
    virtual BOOL IsPlaying() { return m_RT_Flags.is(flRT_Playing); }
    virtual void SetHudMode(BOOL b) { m_RT_Flags.set(flRT_HUDmode, b); }
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

    void SetDestroyCB(DestroyCallback destroy_cb) { m_DestroyCallback = destroy_cb; }
    void SetCollisionCB(CollisionCallback collision_cb) { m_CollisionCallback = collision_cb; }
    void SetBirthDeadCB(PAPI::OnBirthParticleCB bc, PAPI::OnDeadParticleCB dc, void* owner, u32 p);

    virtual u32 ParticlesCount();
};
void OnEffectParticleBirth(void* owner, u32 param, PAPI::Particle& m, u32 idx);
void OnEffectParticleDead(void* owner, u32 param, PAPI::Particle& m, u32 idx);

extern const u32 uDT_STEP;
extern const float fDT_STEP;
}
//---------------------------------------------------------------------------
#endif
