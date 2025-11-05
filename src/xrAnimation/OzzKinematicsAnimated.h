#pragma once

#include "Include/xrRender/KinematicsAnimated.h"
#include "Include/xrRender/animation_motion.h"
#include "Layers/xrRender/Animation.h"
#include "OzzKinematics.h"
#include "OzzSharedMotions.hpp"
#include "xrCommon/xr_unordered_map.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/span.h"

#include "AnimationECS_Registry.h"
#include "AnimationECS_IK.h"
#include "entt/entt.hpp"

#include <filesystem>
#include <memory>

class CMotion;
class CMotionDef;

namespace XRay::Animation
{
/**
 * Animated kinematics implementation using Ozz runtime.
 * This class extends OzzKinematics with IKinematicsAnimated functionality.
 */
class OzzKinematicsAnimated : public OzzKinematics,
                              public IKinematicsAnimated
{
public:
    OzzKinematicsAnimated();
    ~OzzKinematicsAnimated() override;
    virtual void OnSkeletonLoaded() override;

    void Copy(OzzKinematicsAnimated* from);

    bool InitializeFromOzz(pcstr skeletonPath, const xr_vector<xr_string>& motionRefs = xr_vector<xr_string>());
    bool InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData, const xr_vector<xr_string>& motionRefs = xr_vector<xr_string>());

    void SetEmbeddedAnimationData(const std::vector<std::uint8_t>& data);

    bool LoadAnimationFromFile(const std::filesystem::path& path);
    void StopAnimation();
    bool AdvanceAnimation(float dt);

    bool HasActiveAnimation() const
    {
        return animationApplied;
    }

    bool HasLoadedAnimation() const;
    void SetLooping(bool loop);
    void SetPlaybackSpeed(float speed);
    float AnimationDuration() const;
#ifdef DEBUG
    ozz::span<const ozz::math::SoaTransform> DebugSampledLocals() const;
#endif

    struct ActiveBlendEntry
    {
        CBlend* blend = nullptr;
        MotionID motionId{};
        u16 partition = BI_NONE;
        u8 channel = 0;
        u16 recordIndex = u16(-1);
    };

    void OnCalculateBones() override;
    void CalculateBones(BOOL bForceExact = FALSE) override;
#ifdef DEBUG
    std::pair<LPCSTR, LPCSTR> LL_MotionDefName_dbg(MotionID ID) override;
    void LL_DumpBlends_dbg() override;
#endif
    u32 LL_PartBlendsCount(u32 bone_part_id) override;
    CBlend* LL_PartBlend(u32 bone_part_id, u32 n) override;
    void LL_IterateBlends(IterateBlendsCallback& callback) override;
    u16 LL_MotionsSlotCount() override;
    const shared_motions& LL_MotionsSlot(u16 idx) override;
    CMotionDef* LL_GetMotionDef(MotionID id) override;
    CMotion* LL_GetRootMotion(MotionID id) override;
    CMotion* LL_GetMotion(MotionID id, u16 bone_id) override;
    void LL_BuldBoneMatrixDequatize(const CBoneData* bd, u8 channel_mask, SKeyTable& keys) override;
    void LL_BoneMatrixBuild(CBoneInstance& bi, const Fmatrix* parent, const SKeyTable& keys) override;

    void LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset) override;
    void LL_ClearAdditionalTransform(u16 bone_id) override;

    IBlendDestroyCallback* GetBlendDestroyCallback() override;
    void SetBlendDestroyCallback(IBlendDestroyCallback* cb) override;
    void SetUpdateTracksCalback(IUpdateTracksCallback* callback) override;
    IUpdateTracksCallback* GetUpdateTracksCalback() override;

    MotionID LL_MotionID(LPCSTR B) override;
    u16 LL_PartID(LPCSTR B) override;

    CBlend* LL_PlayCycle(u16 partition, MotionID motion, BOOL bMixing, float blendAccrue, float blendFalloff, float Speed, BOOL noloop, PlayCallback Callback,
        LPVOID CallbackParam, u8 channel = 0) override;
    CBlend* LL_PlayCycle(u16 partition, MotionID motion, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel = 0) override;
    void LL_CloseCycle(u16 partition, u8 mask_channel = (1 << 0)) override;
    void LL_SetChannelFactor(u16 channel, float factor) override;
    void UpdateTracks() override;
    void LL_UpdateTracks(float dt, bool b_force, bool leave_blends) override;

    MotionID ID_Cycle(LPCSTR N) override;
    MotionID ID_Cycle_Safe(LPCSTR N) override;
    MotionID ID_Cycle(shared_str N) override;
    MotionID ID_Cycle_Safe(shared_str N) override;
    CBlend* PlayCycle(LPCSTR N, BOOL bMixIn = TRUE, PlayCallback Callback = nullptr, LPVOID CallbackParam = nullptr, u8 channel = 0) override;
    CBlend* PlayCycle(MotionID M, BOOL bMixIn = TRUE, PlayCallback Callback = nullptr, LPVOID CallbackParam = nullptr, u8 channel = 0) override;
    CBlend* PlayCycle(u16 partition, MotionID M, BOOL bMixIn = TRUE, PlayCallback Callback = nullptr, LPVOID CallbackParam = nullptr, u8 channel = 0) override;

    MotionID ID_FX(LPCSTR N) override;
    MotionID ID_FX_Safe(LPCSTR N) override;
    CBlend* PlayFX(LPCSTR N, float power_scale) override;
    CBlend* PlayFX(MotionID M, float power_scale) override;
    CBlend* PlayFX_Safe(cpcstr N, float power_scale) override;

    const CPartition& partitions() const override;
    float get_animation_length(MotionID motion_ID) override;

    // Animation enumeration helpers for utilities
    u16 GetAvailableMotionCount() const;
    bool GetMotionName(u16 index, xr_string& out_name) const;
    bool GetMotionInfo(u16 index, xr_string& out_name, float& out_duration) const;

    IRenderVisual* dcast_RenderVisual() override
    {
        return OzzKinematics::dcast_RenderVisual();
    }

    IKinematics* dcast_PKinematics() override
    {
        return this;
    }

    IKinematicsAnimated* dcast_PKinematicsAnimated() override
    {
        return this;
    }

    OzzMotionsContainer* GetMotionsContainer() const;

private:
    struct SMotionsSlot
    {
        SharedOzzMotions motions;
        BoneMotionsVec bone_motions;
    };

    using MotionsSlotVec = xr_vector<SMotionsSlot>;

    void ResetAnimationState();
    void InitializeChannelState();
    void EnsureMotionLibraryLoaded();
    void BuildBoneMotionCache(SMotionsSlot& slot);

    CBlend* IBlend_Create();
    void IBlend_Startup();

    int FindActiveBlendIndex(u16 partition, u8 channel) const;
    void RemoveActiveBlend(size_t index, bool notifyDestroy);
    void ClearActiveBlends(bool notifyDestroy);
    void ResetPlaybackState();
    void InitializeSamplingState();
    void ResetSamplingBuffers();
    bool LoadAnimationClip(const std::shared_ptr<ozz::animation::Animation>& animation);
    bool LoadAnimationClipFromFile(const std::filesystem::path& path);

private:
    std::shared_ptr<ozz::animation::Animation> activeAnimation;
    ozz::animation::SamplingJob::Context samplingContext;
    xr_vector<ozz::math::SoaTransform> sampledLocals;
    bool animationLoaded = false;
    bool loopPlayback = true;
    float playbackSpeed = 1.f;
    float playbackTime = 0.f;

    MotionsSlotVec m_Motions;
    xr_vector<xr_string> motionReferences;
    std::vector<std::uint8_t> embeddedAnimationData;

    svector<CBlend, MAX_BLENDED_POOL> blend_pool;
    xr_vector<ActiveBlendEntry> activeBlends;
    MotionID controllerMotion{};
    bool animationApplied = false;

    IBlendDestroyCallback* blendDestroyCallback = nullptr;
    IUpdateTracksCallback* updateTracksCallback = nullptr;

    xray::render::RENDER_NAMESPACE::animation::channal_rule channelRules[MAX_CHANNELS]{};
    float channelFactors[MAX_CHANNELS]{};

    CPartition defaultPartition{};

    // ECS Integration
    entt::entity m_ecs_entity{entt::null};
    bool m_use_ecs{true};  // Toggle for ECS vs legacy path
};
} // namespace XRay::Animation
