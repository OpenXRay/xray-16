#pragma once

#include "CycleConstStorage.h"
#include <ode/common.h>
#include "xrCore/_vector3d.h"
#include "xrCore/_quaternion.h"

class CPHInterpolation
{
public:
    CPHInterpolation();
    void SetBody(dBodyID body);
    static const u16 PH_INTERPOLATION_POINTS = 2;
    void InterpolatePosition(Fvector& pos);
    void InterpolateRotation(Fmatrix& rot);
    void UpdatePositions();
    void UpdateRotations();
    void ResetPositions();
    void ResetRotations();
    void GetRotation(Fquaternion& q, u16 num);
    void GetPosition(Fvector& p, u16 num);
    void SetRotation(const Fquaternion& q, u16 num);
    void SetPosition(const Fvector& p, u16 num);

private:
    dBodyID m_body;
    CCycleConstStorage<Fvector, PH_INTERPOLATION_POINTS> qPositions;
    CCycleConstStorage<Fquaternion, PH_INTERPOLATION_POINTS> qRotations;
};
