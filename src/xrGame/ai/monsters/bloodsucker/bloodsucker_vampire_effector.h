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
    static constexpr float DELTA_ANGLE_X = 10.f * PI / 180.f;
    static constexpr float DELTA_ANGLE_Y = DELTA_ANGLE_X;
    static constexpr float DELTA_ANGLE_Z = DELTA_ANGLE_X;
    static constexpr float ANGLE_SPEED = 0.2f;
    static constexpr float BEST_DISTANCE = 0.3f;

private:
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
