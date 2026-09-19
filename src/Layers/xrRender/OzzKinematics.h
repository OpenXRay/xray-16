#pragma once

#include <memory>
#include <vector>

#include "Include/xrRender/Kinematics.h"
#include "Layers/xrRender/FHierrarhyVisual.h"
#include "OzzModelData.h"
#include "xrCommon/xr_vector.h"
#include "xrCore/_fbox.h"

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"

struct OzzBoneVisibility
{
    u64 mask = u64(-1);
    bool get(u16 bone_id) const { return bone_id >= 64 || (mask & (u64(1) << bone_id)) != 0; }
    void set(u16 bone_id, bool visible)
    {
        if (bone_id >= 64) return;
        if (visible) mask |= (u64(1) << bone_id);
        else mask &= ~(u64(1) << bone_id);
    }
    void setAll(u64 m) { mask = m; }
};

namespace XRay::Animation
{
class OzzKinematics : public xray::render::fg::FHierrarhyVisual, public IKinematics
{
public:
    OzzKinematics();
    ~OzzKinematics() override;

    virtual bool LoadBundle(const OzzxBundle& bundle);
    bool LoadMeshFromBuffer(const std::vector<std::uint8_t>& meshData);

    void Copy(xray::render::fg::dxRender_Visual* pFrom) override;
    void Spawn() override;
    void Depart() override;

    bool HasBones() const
    {
        return model && !boneInstances.empty();
    }

    bool IsInitialized() const
    {
        return model != nullptr;
    }

    const ozz::animation::Skeleton& Skeleton() const
    {
        return model->skeleton;
    }

    void ClearPose();
    void BuildSkinningPalette(xr_vector<Fmatrix>& out_matrices, bool render_space) const;

    void Bone_Calculate(CBoneData* bd, Fmatrix* parent) override;
    void Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 channel_mask, bool ignore_callbacks) override;
    bool PickBone(const Fmatrix& parent_xform, pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id) override;
    void EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id) override;

    u16 LL_BoneID(LPCSTR B) override;
    u16 LL_BoneID(const shared_str& B) override;
    LPCSTR LL_BoneName_dbg(u16 ID) override;

    CInifile* LL_UserData() override
    {
        return model ? model->userData.get() : nullptr;
    }

    accel* LL_Bones() override
    {
        return model ? const_cast<accel*>(&model->boneMapByName) : nullptr;
    }

    CBoneInstance& LL_GetBoneInstance(u16 bone_id) override;
    CBoneData& LL_GetData(u16 bone_id) override;
    const IBoneData& GetBoneData(u16 bone_id) const override;

    u16 LL_BoneCount() const override
    {
        return static_cast<u16>(boneInstances.size());
    }

    u16 LL_VisibleBoneCount() override;

    Fmatrix& LL_GetTransform(u16 bone_id) override;
    const Fmatrix& LL_GetTransform(u16 bone_id) const override;
    Fmatrix& LL_GetTransform_R(u16 bone_id) override;
    Fobb& LL_GetBox(u16 bone_id) override;
    const Fbox& GetBox() const override;
    void LL_GetBindTransform(xr_vector<Fmatrix>& matrices) override;
    int LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups) override;

    u16 LL_GetBoneRoot() override
    {
        return rootBone;
    }

    void LL_SetBoneRoot(u16 bone_id) override
    {
        rootBone = bone_id;
    }

    BOOL LL_GetBoneVisible(u16 bone_id) override;
    void LL_SetBoneVisible(u16 bone_id, BOOL val, BOOL bRecursive) override;
    u64 LL_GetBonesVisible() override;
    void LL_SetBonesVisible(u64 mask) override;

    void LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset) override;
    void LL_ClearAdditionalTransform(u16 bone_id) override;

    void CalculateBones(BOOL bForceExact = FALSE) override;
    void CalculateBones_Invalidate() override;

    void Callback(UpdateCallback C, void* Param) override
    {
        updateCallback = C;
        updateCallbackParam = Param;
    }

    void SetUpdateCallback(UpdateCallback pCallback) override
    {
        updateCallback = pCallback;
    }

    void SetUpdateCallbackParam(void* pCallbackParam) override
    {
        updateCallbackParam = pCallbackParam;
    }

    UpdateCallback GetUpdateCallback() override
    {
        return updateCallback;
    }

    void* GetUpdateCallbackParam() override
    {
        return updateCallbackParam;
    }

    IRenderVisual* dcast_RenderVisual() override
    {
        return static_cast<xray::render::fg::dxRender_Visual*>(this);
    }

    IKinematics* dcast_PKinematics() override
    {
        return static_cast<IKinematics*>(this);
    }

    IKinematicsAnimated* dcast_PKinematicsAnimated() override
    {
        return nullptr;
    }

    void Load(const char* N, IReader* data, u32 dwFlags) override;

#ifdef DEBUG
    void DebugRender(Fmatrix& XFORM) override;
    shared_str getDebugName() override;
#endif

public:
    u32 fg_bone_upload_frame{0};
    u32 fg_bone_upload_offset{0};

protected:
    virtual void OnCalculateBones() {}

    bool IsBoneVisible(u16 bone_id) const;

protected:
    std::shared_ptr<const OzzModelData> model;

    xr_vector<ozz::math::SoaTransform> sampledLocals;
    bool poseValid;

    xr_vector<CBoneInstance> boneInstances;
    xr_vector<Fobb> boneBoxes;

    xr_vector<ozz::math::Float4x4> modelTransforms;
    xr_vector<Fmatrix> subtreeDelta;
    xr_vector<u8> subtreeDirty;

    xr_vector<KinematicsABT::additional_bone_transform> boneOffsets;

    u16 rootBone;
    OzzBoneVisibility boneVisibility;
    u32 lastUpdateTime;
    Fbox cachedBox;

    UpdateCallback updateCallback;
    void* updateCallbackParam;

private:
    void IBoneInstances_Create();
    u64 FullVisibilityMask() const;
    bool ApplyAdditionalBoneTransforms(u16 bone_id, Fmatrix& transform) const;
};
}
