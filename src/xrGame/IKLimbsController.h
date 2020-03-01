#pragma once

#include "ik/IKLimb.h"
#include "pose_extrapolation.h"
#include "ik_object_shift.h"
class IKinematicsAnimated;
class CGameObject;
class CBlend;
struct SIKCrlCalcData;

class CIKLimbsController
{
private:
    static const u16 max_size = 4;

public:
    CIKLimbsController();
    void Create(CGameObject* O);
    void Destroy(CGameObject* O);

public:
    void PlayLegs(CBlend* b);
    void Update();
    float Shift() { return _object_shift.shift(); }
private:
    void Calculate();
    void LimbCalculate(SCalculateData& cd);
    void ShiftObject(const SCalculateData cd[max_size]);
    float StaticObjectShift(const SCalculateData cd[max_size]);
    float LegLengthShiftLimit(float current_shift, const SCalculateData cd[max_size]);
    bool PredictObjectShift(const SCalculateData cd[max_size]);
    void ObjectShift(float static_shift, const SCalculateData cd[max_size]);
    void LimbUpdate(CIKLimb& L);
    void LimbSetup();

private:
    static void __stdcall IKVisualCallback(IKinematics* K);

private:
    CBlend* m_legs_blend;
    CGameObject* m_object;
    xr_vector<CIKLimb> _bone_chains;
    object_shift _object_shift;
    extrapolation::points _pose_extrapolation;

#ifdef DEBUG
    LPCSTR anim_name;
    LPCSTR anim_set_name;
#endif
};
