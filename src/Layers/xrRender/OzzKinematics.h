#pragma once

#include "Include/xrRender/Kinematics.h"
#include "Layers/xrRender/FHierrarhyVisual.h"
#include "OzzKinematicsCore.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"

#include <entt/entt.hpp>

namespace XRay::Animation
{
class OzzKinematics : public xray::render::fg::FHierrarhyVisual, public IKinematics
{
public:
    OzzKinematics();
    ~OzzKinematics() override;

    bool InitializeFromOzz(pcstr skeletonPath);
    bool InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData);
    bool ApplyExtendedBoneMetadata(const ExtendedBoneMetadataCollection& metadata);
    bool LoadUserDataFromBuffer(const std::vector<std::uint8_t>& buffer);
    bool LoadMeshFromBuffer(const std::vector<std::uint8_t>& meshData);

    void CacheBundlePayloads(
        std::vector<std::uint8_t> skeleton,
        std::vector<std::uint8_t> mesh,
        xr_vector<xr_string> motion_refs,
        ExtendedBoneMetadataCollection bone_metadata,
        std::vector<std::uint8_t> user_data,
        std::vector<std::uint8_t> embedded_anim);

    void Copy(xray::render::fg::dxRender_Visual* pFrom) override;

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
    void CalculateBonesFG(BOOL bForceExact = FALSE) override;
    void CalculateBones_InvalidateFG() override;
    void Callback(UpdateCallback C, void* Param) override;

    void SetUpdateCallback(UpdateCallback pCallback) override;
    void SetUpdateCallbackParam(void* pCallbackParam) override;
    UpdateCallback GetUpdateCallback() override;
    void* GetUpdateCallbackParam() override;

    virtual entt::entity GetSkeletonEntity() const { return m_skeleton_entity; }

    IRenderVisual* dcast_RenderVisual() override
    {
        return this;
    }

    IKinematics* dcast_PKinematics() override
    {
        return this;
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

protected:
    OzzKinematicsCore core;

    mutable CBoneInstance stubBoneInstance;
    mutable CBoneData stubBoneData;
    mutable Fmatrix stubMatrix;
    mutable Fobb stubObb;
    mutable accel stubAccel;

    std::vector<std::uint8_t> m_BundleSkeleton;
    std::vector<std::uint8_t> m_BundleMesh;
    xr_vector<xr_string>      m_BundleMotionRefs;
    ExtendedBoneMetadataCollection m_BundleBoneMeta;
    std::vector<std::uint8_t> m_BundleUserData;
    std::vector<std::uint8_t> m_BundleEmbeddedAnim;

    entt::entity m_skeleton_entity{ entt::null };

    virtual void OnSkeletonLoaded();

    void NotImplemented(pcstr function_name) const;
    CBoneInstance& StubBoneInstance() const;
    CBoneData& StubBoneData() const;
    Fmatrix& StubMatrix() const;
    Fobb& StubObb() const;
};
} // namespace XRay::Animation
