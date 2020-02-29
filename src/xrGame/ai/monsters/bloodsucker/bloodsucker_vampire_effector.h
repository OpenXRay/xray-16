#pragma once
#include "xrEngine/EffectorPP.h"
#include "CameraEffector.h"
#include "xrEngine/CameraManager.h"

class CVampirePPEffector : public CEffectorPP
{
    typedef CEffectorPP inherited;

    SPPInfo state; // current state
    float m_total; // total PP time

public:
    CVampirePPEffector(const SPPInfo& ppi, float life_time);
    virtual bool Process(SPPInfo& pp);
};

//////////////////////////////////////////////////////////////////////////
// Vampire Camera Effector
//////////////////////////////////////////////////////////////////////////
class CVampireCameraEffector : public CEffectorCam
{
    typedef CEffectorCam inherited;

    float m_time_total;
    Fvector dangle_target;
    Fvector dangle_current;

    float m_dist;
    Fvector m_direction;

public:
    CVampireCameraEffector(float time, const Fvector& src, const Fvector& tgt);
    virtual bool ProcessCam(SCamEffectorInfo& info);
};
