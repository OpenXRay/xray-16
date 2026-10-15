#pragma once

#include "PHInterpolation.h"
#include "xrPhysicsCore/IPhysicsCore.h"
#include "xrCore/_matrix33.h"

class PHDynamicData
{
public:
    Fvector pos;
    Fmatrix33 R;
    Fmatrix BoneTransform;

private:
    CharacterVirtualHandle body = INVALID_CHARACTER_VIRTUAL_HANDLE;
    CPHInterpolation* p_parent_body_interpolation = nullptr;
    CPHInterpolation body_interpolation;

    PhysicsShapeHandle geom = nullptr;
    PhysicsShapeHandle transform = nullptr;

    // PHDynamicData* Childs;
    // xr_vector<PHDynamicData>  Childs;
    unsigned int numOfChilds = 0;
    Fmatrix ZeroTransform;

public:
    PHDynamicData();
    PHDynamicData(unsigned int numOfchilds, CharacterVirtualHandle body);

    inline void UpdateInterpolation()
    {
        body_interpolation.UpdatePositions();
        body_interpolation.UpdateRotations();
    }

    void UpdateInterpolationRecursive();
    void InterpolateTransform(Fmatrix& transform);
    void InterpolateTransformVsParent(Fmatrix& transform);

    void Destroy();
    void Create(unsigned int numOfchilds, CharacterVirtualHandle Body);
    void CalculateData(void);

    PHDynamicData* GetChild(unsigned int ChildNum);
    bool SetChild(unsigned int ChildNum, unsigned int numOfchilds, CharacterVirtualHandle body);

    void SetAsZero();
    void SetAsZeroRecursive();
    void SetZeroTransform(Fmatrix& aTransform);

    void GetWorldMX(Fmatrix& aTransform)
    {
        if (body != INVALID_CHARACTER_VIRTUAL_HANDLE)
        {
            GetPhysicsCore()->GetBodyTransform(body, aTransform);
        }
        else
        {
            aTransform.identity();
        }
    }

    void GetTGeomWorldMX(Fmatrix& aTransform)
    {
        GetWorldMX(aTransform);
    }

private:
    void CalculateR_N_PosOfChilds(CharacterVirtualHandle parent);

public:
    bool SetGeom(PhysicsShapeHandle ageom);
    bool SetTransform(PhysicsShapeHandle ageom);
};
