#pragma once

#include "Include/xrRender/Kinematics.h"
#include "OzzKinematicsCore.h"

namespace XRay::Animation
{
/**
 * Static kinematics implementation using Ozz runtime.
 * This class provides IKinematics functionality for non-animated skeletons.
 */
class OzzKinematics : public IKinematics
{
public:
    OzzKinematics();
    ~OzzKinematics();

    bool InitializeFromOzz(pcstr skeletonPath);
    bool InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData);
    bool ApplyExtendedBoneMetadata(const ExtendedBoneMetadataCollection& metadata);
    bool LoadUserDataFromBuffer(const std::vector<std::uint8_t>& buffer);

    bool HasBones() const
    {
        return core.HasBones();
    }

    bool IsInitialized() const
    {
        return core.IsInitialized();
    }

    const ozz::animation::Skeleton& Skeleton() const
    {
        return core.Skeleton();
    }

    bool SetPoseLocals(ozz::span<const ozz::math::SoaTransform> locals);
    void ClearPose();
    void BuildSkinningPalette(xr_vector<Fmatrix>& out_matrices, bool render_space) const;

    void Bone_Calculate(CBoneData* bd, Fmatrix* parent) override;
    void Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 channel_mask, bool ignore_callbacks) override;
    bool PickBone(const Fmatrix& parent_xform, pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id) override;
    void EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id) override;

    u16 LL_BoneID(LPCSTR B) override;
    u16 LL_BoneID(const shared_str& B) override;
    LPCSTR LL_BoneName_dbg(u16 ID) override;

    CInifile* LL_UserData() override;
    accel* LL_Bones() override;

    CBoneInstance& LL_GetBoneInstance(u16 bone_id) override;
    CBoneData& LL_GetData(u16 bone_id) override;
    const IBoneData& GetBoneData(u16 bone_id) const override;

    u16 LL_BoneCount() const override;
    u16 LL_VisibleBoneCount() override;

    Fmatrix& LL_GetTransform(u16 bone_id) override;
    const Fmatrix& LL_GetTransform(u16 bone_id) const override;
    Fmatrix& LL_GetTransform_R(u16 bone_id) override;
    Fobb& LL_GetBox(u16 bone_id) override;
    const Fbox& GetBox() const override;
    void LL_GetBindTransform(xr_vector<Fmatrix>& matrices) override;
    int LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups) override;

    u16 LL_GetBoneRoot() override;
    void LL_SetBoneRoot(u16 bone_id) override;

    BOOL LL_GetBoneVisible(u16 bone_id) override;
    void LL_SetBoneVisible(u16 bone_id, BOOL val, BOOL bRecursive) override;
    u64 LL_GetBonesVisible() override;
    void LL_SetBonesVisible(u64 mask) override;

    void LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset) override;
    void LL_ClearAdditionalTransform(u16 bone_id) override;

    void CalculateBones(BOOL bForceExact = FALSE) override;
    void CalculateBones_Invalidate() override;
    void Callback(UpdateCallback C, void* Param) override;

    void SetUpdateCallback(UpdateCallback pCallback) override;
    void SetUpdateCallbackParam(void* pCallbackParam) override;
    UpdateCallback GetUpdateCallback() override;
    void* GetUpdateCallbackParam() override;

    void SetVisualOwner(IRenderVisual* visual)
    {
        core.SetVisualOwner(visual);
    }

    IRenderVisual* GetVisualOwner() const
    {
        return core.GetVisualOwner();
    }

    IRenderVisual* dcast_RenderVisual() override
    {
        return core.GetVisualOwner();
    }

    IKinematicsAnimated* dcast_PKinematicsAnimated() override
    {
        return nullptr;
    }

#ifdef DEBUG
    void DebugRender(Fmatrix& XFORM) override;
    shared_str getDebugName() override;
#endif

protected:
    OzzKinematicsCore core;

    mutable CBoneInstance stubBoneInstance;
    mutable CBoneData stubBoneData;
    mutable Fmatrix stubMatrix;
    mutable Fobb stubObb;
    mutable accel stubAccel;

    virtual void OnSkeletonLoaded() {}

    void NotImplemented(pcstr function_name) const;
    CBoneInstance& StubBoneInstance() const;
    CBoneData& StubBoneData() const;
    Fmatrix& StubMatrix() const;
    Fobb& StubObb() const;
};
} // namespace XRay::Animation
