#ifndef Kinematics_included
#define Kinematics_included
#pragma once

#include "RenderVisual.h"
#include "Layers/xrRender/KinematicsAddBoneTransform.hpp" //--#SM+#--

typedef void (*UpdateCallback)(IKinematics* P);

class CBoneData;
class IBoneData;
class IKinematicsAnimated;
class IRenderVisual;
class CBoneInstance;
struct SEnumVerticesCallback;

template <class T>
class _box3;
typedef _box3<float> Fbox;

template <class T>
struct _obb;
typedef _obb<float> Fobb;

// 100 ms = 10 fps
#define UCalc_Interval (u32(100))

class IKinematics
{
public:
    typedef xr_vector<std::pair<shared_str, u16>> accel;
    struct pick_result
    {
        Fvector normal;
        float dist;
        Fvector tri[3];
    };

public:
    virtual void Bone_Calculate(CBoneData* bd, Fmatrix* parent) = 0;
    virtual void Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 channel_mask, bool ignore_callbacks) = 0;

    virtual bool PickBone(const Fmatrix& parent_xform, pick_result& r, float dist, const Fvector& start,
        const Fvector& dir, u16 bone_id) = 0;
    virtual void EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id) = 0;

    // Low level interface
    virtual u16 LL_BoneID(LPCSTR B) = 0;
    virtual u16 LL_BoneID(const shared_str& B) = 0;
    virtual LPCSTR LL_BoneName_dbg(u16 ID) = 0;

    virtual CInifile* LL_UserData() = 0;
    virtual accel* LL_Bones() = 0;

    virtual ICF CBoneInstance& LL_GetBoneInstance(u16 bone_id) = 0;

    virtual CBoneData& LL_GetData(u16 bone_id) = 0;

    virtual const IBoneData& GetBoneData(u16 bone_id) const = 0;

    virtual u16 LL_BoneCount() const = 0;
    virtual u16 LL_VisibleBoneCount() = 0;

    virtual ICF Fmatrix& LL_GetTransform(u16 bone_id) = 0;
    virtual ICF const Fmatrix& LL_GetTransform(u16 bone_id) const = 0;

    virtual ICF Fmatrix& LL_GetTransform_R(u16 bone_id) = 0;
    virtual Fobb& LL_GetBox(u16 bone_id) = 0;
    virtual const Fbox& GetBox() const = 0;
    virtual void LL_GetBindTransform(xr_vector<Fmatrix>& matrices) = 0;
    virtual int LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups) = 0;

    virtual u16 LL_GetBoneRoot() = 0;
    virtual void LL_SetBoneRoot(u16 bone_id) = 0;

    virtual BOOL LL_GetBoneVisible(u16 bone_id) = 0;
    virtual void LL_SetBoneVisible(u16 bone_id, BOOL val, BOOL bRecursive) = 0;
    virtual u64 LL_GetBonesVisible() = 0;
    virtual void LL_SetBonesVisible(u64 mask) = 0;

    virtual void LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset) = 0; //--#SM+#--
    virtual void LL_ClearAdditionalTransform(u16 bone_id) = 0; //--#SM+#--

    // Main functionality
    virtual void CalculateBones(BOOL bForceExact = FALSE) = 0; // Recalculate skeleton
    virtual void CalculateBones_Invalidate() = 0;
    virtual void Callback(UpdateCallback C, void* Param) = 0;

    //	Callback: data manipulation
    virtual void SetUpdateCallback(UpdateCallback pCallback) = 0;
    virtual void SetUpdateCallbackParam(void* pCallbackParam) = 0;

    virtual UpdateCallback GetUpdateCallback() = 0;
    virtual void* GetUpdateCallbackParam() = 0;
    // UpdateCallback						Update_Callback;
    // void*								Update_Callback_Param;
    virtual IRenderVisual* dcast_RenderVisual() = 0;
    virtual IKinematicsAnimated* dcast_PKinematicsAnimated() = 0;

// debug
#ifdef DEBUG
    virtual void DebugRender(Fmatrix& XFORM) = 0;
    virtual shared_str getDebugName() = 0;
#endif
};

IC IKinematics* PKinematics(IRenderVisual* V) { return V ? V->dcast_PKinematics() : 0; }
#endif //	Kinematics_included
