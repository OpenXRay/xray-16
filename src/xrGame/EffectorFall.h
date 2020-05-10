#pragma once

#include "xrEngine/Effector.h"

// приседание после падения
class CEffectorFall : public CEffectorCam
{
    float fPower;
    float fPhase;

public:
    CEffectorFall(float power, float life_time = 1);
    virtual bool ProcessCam(SCamEffectorInfo& info);
};

class CEffectorDOF : public CEffectorCam
{
    float m_fPhase;

public:
    CEffectorDOF(const Fvector4& dof);
    virtual bool ProcessCam(SCamEffectorInfo& info);
};
