#pragma once

#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "ModelGeometry.h"
#include "SkeletonBones.h"
#include "SkeletonMotions.h"
#include "MotionPlayback.h"
#include "xrCore/Animation/Bone.hpp"

#include <memory>
#include <vector>

namespace xray::render::vulkan
{
class VulkanKinematics final : public IKinematics, public IKinematicsAnimated
{
public:
    static std::unique_ptr<VulkanKinematics> create(const SkeletonBones& source,
        IRenderVisual* owner, std::string& error);
    VulkanKinematics(const VulkanKinematics& source, IRenderVisual* owner);
    bool attach_geometry(const ModelGeometry& geometry, std::string& error);
    void set_motions(std::vector<MotionSlot> motions);
    const std::vector<MotionSlot>& motions() const;
    bool play_motion(const std::string& name, bool fx = false, bool mixing = true,
        float power = 1.f, MotionPlayback::Finished finished = {});
    void advance_motions(float seconds);
    void stop_motions(uint16_t part = UINT16_MAX);
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
    IKinematicsAnimated* dcast_PKinematicsAnimated() override { return this; }
    IKinematics* dcast_PKinematics() override { return this; }
    void OnCalculateBones() override { UpdateTracks(); }
#ifdef DEBUG
    std::pair<LPCSTR, LPCSTR> LL_MotionDefName_dbg(MotionID id) override;
    void LL_DumpBlends_dbg() override;
#endif
    u32 LL_PartBlendsCount(u32 part) override;
    CBlend* LL_PartBlend(u32 part, u32 index) override;
    void LL_IterateBlends(IterateBlendsCallback& callback) override;
    u16 LL_MotionsSlotCount() override;
    const shared_motions& LL_MotionsSlot(u16 index) override;
    CMotionDef* LL_GetMotionDef(MotionID id) override;
    CMotion* LL_GetRootMotion(MotionID id) override;
    CMotion* LL_GetMotion(MotionID id, u16 bone) override;
    void LL_BuldBoneMatrixDequatize(const CBoneData* bone, u8 mask, SKeyTable& keys) override;
    void LL_BoneMatrixBuild(CBoneInstance& instance, const Fmatrix* parent, const SKeyTable& keys) override;
    IBlendDestroyCallback* GetBlendDestroyCallback() override { return blend_destroy_; }
    void SetBlendDestroyCallback(IBlendDestroyCallback* callback) override { blend_destroy_ = callback; }
    void SetUpdateTracksCalback(IUpdateTracksCallback* callback) override { tracks_update_ = callback; }
    IUpdateTracksCallback* GetUpdateTracksCalback() override { return tracks_update_; }
    MotionID LL_MotionID(LPCSTR name) override;
    u16 LL_PartID(LPCSTR name) override;
    CBlend* LL_PlayCycle(u16 part, MotionID id, BOOL mixing, float accrue, float falloff,
        float speed, BOOL noloop, PlayCallback callback, LPVOID param, u8 channel) override;
    CBlend* LL_PlayCycle(u16 part, MotionID id, BOOL mixing, PlayCallback callback,
        LPVOID param, u8 channel) override;
    void LL_CloseCycle(u16 part, u8 mask) override;
    void LL_SetChannelFactor(u16 channel, float factor) override;
    void UpdateTracks() override;
    void LL_UpdateTracks(float dt, bool force, bool leave_blends) override;
    MotionID ID_Cycle(LPCSTR name) override;
    MotionID ID_Cycle_Safe(LPCSTR name) override;
    MotionID ID_Cycle(shared_str name) override;
    MotionID ID_Cycle_Safe(shared_str name) override;
    CBlend* PlayCycle(LPCSTR name, BOOL mixing, PlayCallback callback, LPVOID param, u8 channel) override;
    CBlend* PlayCycle(MotionID id, BOOL mixing, PlayCallback callback, LPVOID param, u8 channel) override;
    CBlend* PlayCycle(u16 part, MotionID id, BOOL mixing, PlayCallback callback, LPVOID param, u8 channel) override;
    MotionID ID_FX(LPCSTR name) override;
    MotionID ID_FX_Safe(LPCSTR name) override;
    CBlend* PlayFX(LPCSTR name, float power) override;
    CBlend* PlayFX(MotionID id, float power) override;
    CBlend* PlayFX_Safe(cpcstr name, float power) override;
    const CPartition& partitions() const override { return partitions_; }
    float get_animation_length(MotionID id) override;
#ifdef DEBUG
    void DebugRender(Fmatrix&) override {}
    shared_str getDebugName() override { return "vulkan_kinematics"; }
#endif

private:
    struct Data;
    CBlend* create_blend(u16 part, MotionID id, bool fx, bool mixing,
        float accrue, float falloff, float speed, float power, bool noloop,
        PlayCallback callback, LPVOID param, u8 channel);
    bool valid_motion(MotionID id) const;
    VulkanKinematics(std::shared_ptr<Data> data, IRenderVisual* owner);
    void bind_transform(u16 id, const Fmatrix& parent, xr_vector<Fmatrix>& matrices);
    void set_visible(u16 id, BOOL visible, BOOL recursive);
    std::shared_ptr<Data> data_;
    IRenderVisual* owner_{};
    std::vector<CBoneInstance> instances_;
    std::vector<KinematicsABT::additional_bone_transform> offsets_;
    MotionPlayback playback_;
    CPartition partitions_;
    std::vector<std::unique_ptr<CBlend>> blends_;
    IBlendDestroyCallback* blend_destroy_{};
    IUpdateTracksCallback* tracks_update_{};
    u32 last_motion_frame_{UINT32_MAX};
    float channel_factors_[4]{1.f, 1.f, 1.f, 1.f};
    u16 root_ = BI_NONE;
    u64 visible_{};
    bool dirty_{true};
    UpdateCallback update_{};
    void* update_param_{};
};
}
