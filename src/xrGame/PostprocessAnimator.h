#pragma once
#include "xrCore/PostProcess/PostProcess.hpp"
#include "xrEngine/EffectorPP.h"

class CPostprocessAnimator : public CEffectorPP, public BasicPostProcessAnimator
{
public:
    CPostprocessAnimator(int id, bool cyclic);
    CPostprocessAnimator();
    virtual void Stop(float speed) override;
    virtual void Load(LPCSTR name, bool internalFs = true) override;
    virtual bool Valid();
    virtual bool Process(SPPInfo& PPInfo);
};

class CPostprocessAnimatorLerp : public CPostprocessAnimator
{
protected:
    fastdelegate::FastDelegate0<float> m_get_factor_func;

public:
    void SetFactorFunc(fastdelegate::FastDelegate0<float> f) { m_get_factor_func = f; }
    virtual bool Process(SPPInfo& PPInfo);
};

class CPostprocessAnimatorLerpConst : public CPostprocessAnimator
{
protected:
    float m_power;

public:
    CPostprocessAnimatorLerpConst() { m_power = 1.0f; }
    void SetPower(float val) { m_power = val; }
    virtual bool Process(SPPInfo& PPInfo);
};

class CEffectorController;

class CPostprocessAnimatorControlled : public CPostprocessAnimatorLerp
{
    CEffectorController* m_controller;

public:
    virtual ~CPostprocessAnimatorControlled();
    CPostprocessAnimatorControlled(CEffectorController* c);
    virtual bool Valid();
};
