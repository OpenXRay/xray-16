#pragma once

#include "Include/xrRender/Kinematics.h"
#include "ModelGeometry.h"
#include "SkeletonBones.h"
#include "SkeletonMotions.h"
#include "xrCore/Animation/Bone.hpp"

#include <memory>
#include <vector>

namespace xray::render::vulkan
{
class VulkanKinematics final : public IKinematics
{
public:
    static std::unique_ptr<VulkanKinematics> create(const SkeletonBones& source,
        IRenderVisual* owner, std::string& error);
    VulkanKinematics(const VulkanKinematics& source, IRenderVisual* owner);
    bool attach_geometry(const ModelGeometry& geometry, std::string& error);
    void set_motions(std::vector<MotionSlot> motions);
    const std::vector<MotionSlot>& motions() const;
    void reset_instance_state();

    void Bone_Calculate(CBoneData* bone, Fmatrix* parent) override;
    void Bone_GetAnimPos(Fmatrix& pos, u16 id, u8, bool) override;
    bool PickBone(const Fmatrix&, pick_result&, float, const Fvector&, const Fvector&, u16) override;
    void EnumBoneVertices(SEnumVerticesCallback&, u16) override;
    u16 LL_BoneID(LPCSTR name) override;
    u16 LL_BoneID(const shared_str& name) override;
    LPCSTR LL_BoneName_dbg(u16 id) override;
    CInifile* LL_UserData() override;
    accel* LL_Bones() override;
    CBoneInstance& LL_GetBoneInstance(u16 id) override;
    CBoneData& LL_GetData(u16 id) override;
    const IBoneData& GetBoneData(u16 id) const override;
    u16 LL_BoneCount() const override;
    u16 LL_VisibleBoneCount() override;
    Fmatrix& LL_GetTransform(u16 id) override;
    const Fmatrix& LL_GetTransform(u16 id) const override;
    Fmatrix& LL_GetTransform_R(u16 id) override;
    Fobb& LL_GetBox(u16 id) override;
    const Fbox& GetBox() const override;
    void LL_GetBindTransform(xr_vector<Fmatrix>& matrices) override;
    int LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups) override;
    u16 LL_GetBoneRoot() override { return root_; }
    void LL_SetBoneRoot(u16 id) override;
    BOOL LL_GetBoneVisible(u16 id) override;
    void LL_SetBoneVisible(u16 id, BOOL visible, BOOL recursive) override;
    u64 LL_GetBonesVisible() override { return visible_; }
    void LL_SetBonesVisible(u64 mask) override;
    void LL_AddTransformToBone(KinematicsABT::additional_bone_transform& transform) override;
    void LL_ClearAdditionalTransform(u16 id) override;
    void CalculateBones(BOOL force = FALSE) override;
    void CalculateBones_Invalidate() override { dirty_ = true; }
    void Callback(UpdateCallback callback, void* param) override;
    void SetUpdateCallback(UpdateCallback callback) override { update_ = callback; }
    void SetUpdateCallbackParam(void* param) override { update_param_ = param; }
    UpdateCallback GetUpdateCallback() override { return update_; }
    void* GetUpdateCallbackParam() override { return update_param_; }
    IRenderVisual* dcast_RenderVisual() override { return owner_; }
    IKinematicsAnimated* dcast_PKinematicsAnimated() override { return nullptr; }
#ifdef DEBUG
    void DebugRender(Fmatrix&) override {}
    shared_str getDebugName() override { return "vulkan_kinematics"; }
#endif

private:
    struct Data;
    VulkanKinematics(std::shared_ptr<Data> data, IRenderVisual* owner);
    void bind_transform(u16 id, const Fmatrix& parent, xr_vector<Fmatrix>& matrices);
    void set_visible(u16 id, BOOL visible, BOOL recursive);
    std::shared_ptr<Data> data_;
    IRenderVisual* owner_{};
    std::vector<CBoneInstance> instances_;
    std::vector<KinematicsABT::additional_bone_transform> offsets_;
    u16 root_ = BI_NONE;
    u64 visible_{};
    bool dirty_{true};
    UpdateCallback update_{};
    void* update_param_{};
};
}
