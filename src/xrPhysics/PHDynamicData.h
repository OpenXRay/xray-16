// PHDynamicData.h: interface for the PHDynamicData class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "PHInterpolation.h"
#include "MathUtilsOde.h"
#include "xrCore/_matrix33.h"

class PHDynamicData
{
public:
    dVector3 pos;
    dMatrix3 R;
    Fmatrix BoneTransform;

private:
    dBodyID body;
    CPHInterpolation* p_parent_body_interpolation;
    CPHInterpolation body_interpolation;
    dGeomID geom;
    dGeomID transform;
    // PHDynamicData* Childs;
    // xr_vector<PHDynamicData>  Childs;
    unsigned int numOfChilds;
    Fmatrix ZeroTransform;

public:
    inline void UpdateInterpolation()
    {
        body_interpolation.UpdatePositions();
        body_interpolation.UpdateRotations();
    }
    void UpdateInterpolationRecursive();
    void InterpolateTransform(Fmatrix& transform);
    void InterpolateTransformVsParent(Fmatrix& transform);
    //	PHDynamicData& operator [] (unsigned int i) {return Childs[i];};
    void Destroy();
    void Create(unsigned int numOfchilds, dBodyID Body);
    void CalculateData(void);
    PHDynamicData* GetChild(unsigned int ChildNum);
    bool SetChild(unsigned int ChildNum, unsigned int numOfchilds, dBodyID body);
    void SetAsZero();
    void SetAsZeroRecursive();
    void SetZeroTransform(Fmatrix& aTransform);
    PHDynamicData(unsigned int numOfchilds, dBodyID body);
    PHDynamicData();
    //	virtual ~PHDynamicData();
    void GetWorldMX(Fmatrix& aTransform)
    {
        dMatrix3 R;
        dQtoR(dBodyGetQuaternion(body), R);
        DMXPStoFMX(R, dBodyGetPosition(body), aTransform);
    }
    void GetTGeomWorldMX(Fmatrix& aTransform)
    {
        if (!transform)
            return;
        Fmatrix NormTransform, Transform;
        dVector3 P0 = {0, 0, 0, -1};
        Fvector Translate, Translate1;
        // compute_final_tx(geom);
        // dQtoR(dBodyGetQuaternion(body),R);
        DMXPStoFMX(dBodyGetRotation(body), P0, NormTransform);
        DMXPStoFMX(dGeomGetRotation(dGeomTransformGetGeom(transform)), P0, Transform);

        dVectorSet((dReal*)&Translate, dGeomGetPosition(dGeomTransformGetGeom(transform)));
        dVectorSet((dReal*)&Translate1, dBodyGetPosition(body));

        aTransform.identity();
        aTransform.translate_over(Translate);
        aTransform.mulA_43(NormTransform);
        aTransform.translate_over(Translate1);
        aTransform.mulA_43(Transform);

        //	Translate.add(Translate1);
        // transform.translate_over(Translate1);

        // transform.translate_add
        // normalTransform=oMatrix4x4(dGeomGetRotation(dGeomTransformGetGeom(geom)))*normalTransform;
        // oMatrix4x4 meshTransform(normalTransform);

        // meshTransform.PreTranslate(oVector3(dGeomGetPosition(dGeomTransformGetGeom(geom))));
        // meshTransform.PostTranslate(oVector3(dBodyGetPosition(body)));
    }
    static inline void DMXPStoFMX(const dReal* R, const dReal* pos, Fmatrix& aTransform)
    {
        CopyMemory(&aTransform, R, sizeof(dMatrix3));
        aTransform.transpose();
        CopyMemory(&aTransform.c, pos, sizeof(Fvector));
        aTransform._14 = 0.f;
        aTransform._24 = 0.f;
        aTransform._34 = 0.f;
        aTransform._44 = 1.f;
    };
    static inline void DMXtoFMX(const dReal* R, Fmatrix& aTransform)
    {
        aTransform._11 = R[0];
        aTransform._12 = R[4];
        aTransform._13 = R[8];
        aTransform._14 = 0.f;

        aTransform._21 = R[1];
        aTransform._22 = R[5];
        aTransform._23 = R[9];
        aTransform._24 = 0.f;

        aTransform._31 = R[2];
        aTransform._32 = R[6];
        aTransform._33 = R[10];
        aTransform._34 = 0.f;
        aTransform._44 = 1.f;
    };
    static inline void FMX33toDMX(const Fmatrix33& aTransform, dReal* R)
    {
        R[0] = aTransform._11;
        R[4] = aTransform._12;
        R[8] = aTransform._13;

        R[1] = aTransform._21;
        R[5] = aTransform._22;
        R[9] = aTransform._23;

        R[2] = aTransform._31;
        R[6] = aTransform._32;
        R[10] = aTransform._33;
    };
    static inline void FMXtoDMX(const Fmatrix& aTransform, dReal* R)
    {
        R[0] = aTransform._11;
        R[4] = aTransform._12;
        R[8] = aTransform._13;

        R[1] = aTransform._21;
        R[5] = aTransform._22;
        R[9] = aTransform._23;

        R[2] = aTransform._31;
        R[6] = aTransform._32;
        R[10] = aTransform._33;
    };

private:
    void CalculateR_N_PosOfChilds(dBodyID parent);

public:
    bool SetGeom(dGeomID ageom);
    bool SetTransform(dGeomID ageom);
};
