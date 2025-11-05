#pragma once

#include <cstddef>

#include "Include/xrRender/Kinematics.h"
#include "xrCommon/xr_smart_pointers.h"
#include "xrCore/_fbox.h"

#include "ExtendedBoneMetadata.h"

#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/span.h"

namespace ozz::io
{
class Stream;
}

namespace XRay::Animation
{
/**
 * Core skeleton and bone state management.
 * This class contains all shared functionality needed by both static and animated kinematics.
 * It does not implement any interfaces directly.
 */
class OzzKinematicsCore
{
public:
    OzzKinematicsCore();
    virtual ~OzzKinematicsCore();

    bool InitializeFromOzz(pcstr skeletonPath);
    bool InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData);
    bool ApplyExtendedBoneMetadata(const ExtendedBoneMetadataCollection& metadata);
    bool LoadUserDataFromBuffer(const std::vector<std::uint8_t>& buffer);

    const ozz::animation::Skeleton& Skeleton() const
    {
        return skeleton;
    }

    bool HasBones() const
    {
        return initialized && !boneInstances.empty();
    }

    bool IsInitialized() const
    {
        return initialized;
    }

    bool SetPoseLocals(ozz::span<const ozz::math::SoaTransform> locals);
    void ClearPose();

    ozz::animation::SamplingJob::Context& SamplingContext()
    {
        return samplingContext;
    }

    void BuildSkinningPalette(xr_vector<Fmatrix>& out_matrices, bool render_space) const;

    u16 GetBoneCount() const;

    u16 GetRootBone() const
    {
        return rootBone;
    }

    void SetRootBone(u16 bone_id);

    bool IsBoneVisible(u16 bone_id) const;
    void SetBoneVisible(u16 bone_id, bool visible, bool recursive);

    u64 GetVisibilityMask() const
    {
        return visibleMask;
    }

    void SetVisibilityMask(u64 mask);

    CInifile* GetUserData()
    {
        return userData;
    }

    struct BoneInfo
    {
        shared_str name;
        u16 id;
        u16 parent_id;
        CBoneData* data;
        CBoneInstance* instance;
    };

    bool GetBoneInfo(u16 bone_id, BoneInfo& info) const;
    u16 FindBoneID(pcstr name) const;
    u16 FindBoneID(const shared_str& name) const;
    pcstr GetBoneName(u16 bone_id) const;

    const Fmatrix& GetBoneTransform(u16 bone_id) const;
    const Fmatrix& GetBoneRenderTransform(u16 bone_id) const;
    void GetBindTransforms(xr_vector<Fmatrix>& matrices) const;
    const Fbox& GetBoundingBox() const;
    Fobb& GetBoneBox(u16 bone_id);

    void CalculateTransforms(bool force_exact);
    void InvalidateCache();

    using UpdateCallback = void (*)(IKinematics*);
    void SetUpdateCallback(UpdateCallback callback, void* param);

    UpdateCallback GetUpdateCallback() const
    {
        return updateCallback;
    }

    void* GetUpdateCallbackParam() const
    {
        return updateCallbackParam;
    }

    void SetOwner(IKinematics* owner)
    {
        ownerKinematics = owner;
    }

    IKinematics* GetOwner()
    {
        return ownerKinematics;
    }

    void SetVisualOwner(IRenderVisual* visual)
    {
        ownerVisual = visual;
    }

    IRenderVisual* GetVisualOwner() const
    {
        return ownerVisual;
    }

    void AddBoneTransform(const KinematicsABT::additional_bone_transform& transform);
    void ClearBoneTransform(u16 bone_id);

    IKinematics::accel* GetBoneMapByName()
    {
        return &boneMapByName;
    }

    IKinematics::accel* GetBoneMapByPtr()
    {
        return &boneMapByPtr;
    }

protected:
    virtual void OnSkeletonLoaded() {}

    virtual void OnResetRuntime() {}

    virtual void OnCalculateBones() {}

private:
    bool LoadSkeletonFromStream(ozz::io::Stream* stream, pcstr debug_source);
    bool FinalizeSkeletonInitialization(pcstr debug_source);
    bool BuildBoneMetadata();
    void ResetRuntimeState();
    void ApplyAdditionalBoneTransforms(u16 bone_id, Fmatrix& transform) const;
    void UpdateBoundingBox();

protected:
    ozz::animation::Skeleton skeleton;
    xr_vector<ozz::math::SoaTransform> sampledLocals;
    ozz::animation::SamplingJob::Context samplingContext;

    xr_vector<CBoneInstance> boneInstances;
    xr_vector<CBoneData*> bones;
    xr_vector<xr_unique_ptr<CBoneData>> boneStorage;
    xr_vector<Fobb> boneBoxes;

    xr_vector<ozz::math::Float4x4> modelTransforms;
    xr_vector<Fmatrix> cachedTransformsPreCallbacks;

    IKinematics::accel boneMapByName;
    IKinematics::accel boneMapByPtr;

    xr_vector<KinematicsABT::additional_bone_transform> boneOffsets;

    u16 rootBone;
    u64 visibleMask;
    u32 lastUpdateTime;
    s32 visibilityCounter;
    Fbox cachedBox;
    bool initialized;

    CInifile* userData;
    xr_unique_ptr<CInifile> userDataOwner;

    UpdateCallback updateCallback;
    void* updateCallbackParam;
    IKinematics* ownerKinematics;
    IRenderVisual* ownerVisual;
};
} // namespace XRay::Animation
