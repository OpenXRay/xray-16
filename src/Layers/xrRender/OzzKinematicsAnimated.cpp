#include "stdafx.h"

#include "OzzKinematicsAnimated.h"
#include "OzzMesh.h"

#include "Layers/xrRender/AnimationKeyCalculate.h"
#include "OzzConversion.h"
#include "OzzMotionArchive.h"

#include "framework/mesh.h"

#include "xrCore/Animation/Motion.hpp"
#include "xrCore/FS.h"
#include "xrCore/FS_impl.h"
#include "xrCore/_std_extensions.h"
#include "xrEngine/device.h"

#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>

namespace fs = std::filesystem;
namespace Render = xray::render::fg;

// Forward declaration for GPU compute manager (defined in xrRender)
namespace xray::render::fg
{
    class AnimationComputeManager;
}

namespace XRay
{
namespace Animation
{
namespace
{
constexpr Render::animation::channal_rule kDefaultChannelRules[MAX_CHANNELS] = {
    { Render::animation::lerp, Render::animation::lerp },
    { Render::animation::lerp, Render::animation::lerp },
    { Render::animation::lerp,  Render::animation::add },
    { Render::animation::lerp,  Render::animation::add },
};

bool EndsWithIgnoreCase(const xr_string& value, pcstr suffix)
{
    if (!suffix)
        return false;
    const size_t suffix_length = xr_strlen(suffix);
    if (value.size() < suffix_length)
        return false;
    return xr_stricmp(value.c_str() + value.size() - suffix_length, suffix) == 0;
}

} // namespace

OzzKinematicsAnimated::OzzKinematicsAnimated()
{
    InitializeChannelState();
    ResetPlaybackState();
    IBlend_Startup();
}

OzzKinematicsAnimated::~OzzKinematicsAnimated()
{
    blendDestroyCallback = nullptr;
    updateTracksCallback = nullptr;

    for (ActiveBlendEntry& entry : activeBlends)
    {
        if (entry.blend)
        {
            entry.blend->Callback = nullptr;
            entry.blend->CallbackParam = nullptr;
        }
    }

    ClearActiveBlends(true);
}


void OzzKinematicsAnimated::Copy(xray::render::fg::dxRender_Visual* pFrom)
{
    xray::render::fg::FHierrarhyVisual::Copy(pFrom);

    auto* src = dynamic_cast<OzzKinematicsAnimated*>(pFrom);
    R_ASSERT2(src, "OzzKinematicsAnimated::Copy: source is not OzzKinematicsAnimated");

    m_BundleSkeleton     = src->m_BundleSkeleton;
    m_BundleMesh         = src->m_BundleMesh;
    m_BundleMotionRefs   = src->m_BundleMotionRefs;
    m_BundleBoneMeta     = src->m_BundleBoneMeta;
    m_BundleUserData     = src->m_BundleUserData;
    m_BundleEmbeddedAnim = src->m_BundleEmbeddedAnim;

    if (m_BundleSkeleton.empty())
        return;

    const auto skeleton_span = ozz::span<const std::byte>(
        reinterpret_cast<const std::byte*>(m_BundleSkeleton.data()),
        m_BundleSkeleton.size());

    R_ASSERT2(InitializeFromOzzBuffer(skeleton_span, m_BundleMotionRefs),
        "OzzKinematicsAnimated::Copy: re-init from cached skeleton failed");

    if (!m_BundleBoneMeta.empty())
        ApplyExtendedBoneMetadata(m_BundleBoneMeta);

    if (!m_BundleUserData.empty())
        LoadUserDataFromBuffer(m_BundleUserData);

    if (!m_BundleEmbeddedAnim.empty())
        SetEmbeddedAnimationData(m_BundleEmbeddedAnim);

    OnSkeletonLoaded();

    for (auto* child : children)
    {
        if (auto* mesh_child = dynamic_cast<xray::render::fg::OzzMesh*>(child))
            mesh_child->SetParent(this);
    }
}

void OzzKinematicsAnimated::Copy(OzzKinematicsAnimated* from)
{
    if (!from || !from->core.IsInitialized())
        return;

    // Skeleton payload is shared, so runtime state must be rebuilt instead of copying the core.
    m_Motions = from->m_Motions;

    motionReferences = from->motionReferences;
    embeddedAnimationData = from->embeddedAnimationData;

    defaultPartition = from->defaultPartition;

    if (core.IsInitialized())
    {
        for (auto& slot : m_Motions)
        {
            BuildBoneMotionCache(slot);
        }
    }

    InitializeChannelState();
    ResetPlaybackState();
    InitializeSamplingState();
    IBlend_Startup();

    blendDestroyCallback = nullptr;
    updateTracksCallback = nullptr;
}

void OzzKinematicsAnimated::OnSkeletonLoaded()
{
    EnsureMotionLibraryLoaded();
    PopulateEntityBindPose();

    if (core.IsInitialized())
    {
        defaultPartition[0].Name = "default";
        const u16 bone_count = core.GetBoneCount();
        defaultPartition[0].bones.resize(bone_count);
        for (u16 i = 0; i < bone_count; ++i)
            defaultPartition[0].bones[i] = i;
    }
}

void OzzKinematicsAnimated::PopulateEntityBindPose()
{
    if (!core.IsInitialized())
        return;

    auto& bufs = m_animBufs;
    const int num_joints     = core.Skeleton().num_joints();
    const int num_soa_joints = core.Skeleton().num_soa_joints();

    bufs.locals.resize(static_cast<std::size_t>(num_soa_joints));
    bufs.models.resize(static_cast<std::size_t>(num_joints));
    bufs.context.Resize(num_joints);

    for (int i = 0; i < num_soa_joints; ++i)
        bufs.locals[i] = core.Skeleton().joint_rest_poses()[i];

    m_animCtl.skeleton = &core.Skeleton();

    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &core.Skeleton();
    ltm_job.input    = ozz::make_span(bufs.locals);
    ltm_job.output   = ozz::make_span(bufs.models);
    R_ASSERT2(ltm_job.Run(), "ozz LocalToModelJob (bind-pose) failed");
}

bool OzzKinematicsAnimated::InitializeFromOzz(pcstr skeletonPath, const xr_vector<xr_string>& motionRefs)
{
    if (!skeletonPath || !skeletonPath[0])
        return false;

    motionReferences = motionRefs;

    if (!core.InitializeFromOzz(skeletonPath))
    {
        motionReferences.clear();
        return false;
    }

    InitializeSamplingState();
    ResetPlaybackState();

    InitializeChannelState();
    EnsureMotionLibraryLoaded();

    return true;
}

bool OzzKinematicsAnimated::InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData, const xr_vector<xr_string>& motionRefs)
{
    motionReferences = motionRefs;

    if (!core.InitializeFromOzzBuffer(skeletonData))
    {
        motionReferences.clear();
        return false;
    }

    InitializeSamplingState();
    ResetPlaybackState();

    InitializeChannelState();
    EnsureMotionLibraryLoaded();

    return true;
}

void OzzKinematicsAnimated::SetEmbeddedAnimationData(const std::vector<std::uint8_t>& data)
{
    embeddedAnimationData = data;
}

void OzzKinematicsAnimated::ResetAnimationState()
{
    InitializeChannelState();
    motionReferences.clear();
    m_Motions.clear();
    blendDestroyCallback = nullptr;
    updateTracksCallback = nullptr;
    animationApplied = false;
    embeddedAnimationData.clear();
    ResetPlaybackState();
}

void OzzKinematicsAnimated::InitializeChannelState()
{
    for (u32 channel = 0; channel < MAX_CHANNELS; ++channel)
    {
        channelRules[channel] = kDefaultChannelRules[channel];
        channelFactors[channel] = 1.f;
    }
}

void OzzKinematicsAnimated::IBlend_Startup()
{
    CBlend B;
    B.set_free_state();

    blend_pool.clear();
    for (u32 i = 0; i < MAX_BLENDED_POOL; i++)
    {
        blend_pool.push_back(B);
    }

    activeBlends.clear();
}

CBlend* OzzKinematicsAnimated::IBlend_Create()
{
    for (auto& B : blend_pool)
    {
        if (B.blend_state() == CBlend::eFREE_SLOT)
            return &B;
    }

    return nullptr;
}

void OzzKinematicsAnimated::EnsureMotionLibraryLoaded()
{
    if (!g_pOzzMotionsContainer || !core.IsInitialized())
        return;

    m_Motions.clear();

    SkeletonFingerprint skelFP = SkeletonFingerprint::Compute(core.Skeleton());

    if (!embeddedAnimationData.empty())
    {
        const u32 dataHash = crc32(embeddedAnimationData.data(),
                                   static_cast<u32>(embeddedAnimationData.size()));
        xr_string embeddedKey = xr_string("<embedded_") + xr_string(std::to_string(dataHash).c_str()) + xr_string(">");

        m_Motions.push_back(SMotionsSlot());
        SMotionsSlot& slot = m_Motions.back();

        OzzMotionsContainer::LoadRequest request;
        request.key = shared_str(embeddedKey.c_str());
        request.skelFingerprint = skelFP;
        request.blocking = true;
        request.embeddedData = embeddedAnimationData;

        if (!slot.motions.Create(request, core.Skeleton()))
            m_Motions.pop_back();
        else
            BuildBoneMotionCache(slot);

        embeddedAnimationData.clear();
    }

    for (const auto& reference : motionReferences)
    {
        m_Motions.push_back(SMotionsSlot());
        SMotionsSlot& slot = m_Motions.back();

        xr_string reference_path = reference;
        if (reference_path.find(".ozz") == xr_string::npos)
            reference_path += ".ozz";

        OzzMotionsContainer::LoadRequest request;
        request.key = shared_str(reference_path.c_str());
        request.skelFingerprint = skelFP;
        request.blocking = true;

        if (!slot.motions.Create(request, core.Skeleton()))
        {
            m_Motions.pop_back();
            continue;
        }

        BuildBoneMotionCache(slot);
    }
}

void OzzKinematicsAnimated::BuildBoneMotionCache(SMotionsSlot& slot)
{
    if (!core.IsInitialized())
        return;

    const u32 bone_count = core.Skeleton().num_joints();
    slot.bone_motions.resize(bone_count);

    auto joint_names = core.Skeleton().joint_names();
    for (u32 bone_idx = 0; bone_idx < bone_count; ++bone_idx)
    {
        const char* bone_name = joint_names[bone_idx];
        slot.bone_motions[bone_idx] = slot.motions.GetBoneMotions(bone_name);
    }
}

int OzzKinematicsAnimated::FindActiveBlendIndex(u16 partition, u8 channel) const
{
    const u16 resolvedPartition = (partition == BI_NONE) ? u16(0) : partition;
    for (size_t index = 0; index < activeBlends.size(); ++index)
    {
        const ActiveBlendEntry& entry = activeBlends[index];
        if (entry.partition == resolvedPartition && entry.channel == channel)
            return static_cast<int>(index);
    }
    return -1;
}

void OzzKinematicsAnimated::RemoveActiveBlend(size_t index, bool notifyDestroy)
{
    if (index >= activeBlends.size())
        return;

    ActiveBlendEntry& entry = activeBlends[index];

    if (entry.blend)
    {
        entry.blend->Callback = nullptr;
        entry.blend->CallbackParam = nullptr;

        if (notifyDestroy && blendDestroyCallback)
            blendDestroyCallback->BlendDestroy(*entry.blend);

        entry.blend->set_free_state();
    }

    activeBlends.erase(activeBlends.begin() + index);

    if (activeBlends.empty())
        ResetPlaybackState();
}

void OzzKinematicsAnimated::ClearActiveBlends(bool notifyDestroy)
{
    static thread_local int s_depth = 0;
    ++s_depth;
    if (s_depth > 8)
        Msg("! [ClearActiveBlends] depth=%d (recursion?)", s_depth);

    if (activeBlends.empty())
    {
        ResetPlaybackState();
        --s_depth;
        return;
    }

    for (ActiveBlendEntry& entry : activeBlends)
    {
        if (entry.blend)
        {
            entry.blend->Callback = nullptr;
            entry.blend->CallbackParam = nullptr;

            if (notifyDestroy && blendDestroyCallback)
                blendDestroyCallback->BlendDestroy(*entry.blend);

            entry.blend->set_free_state();
        }
    }

    activeBlends.clear();
    ResetPlaybackState();
    --s_depth;
}

void OzzKinematicsAnimated::ResetPlaybackState()
{
    controllerMotion.invalidate();
    activeAnimation.reset();
    animationLoaded = false;
    animationApplied = false;
    playbackTime = 0.f;
    playbackSpeed = 1.f;
    loopPlayback = true;
    samplingContext.Resize(0);
    ResetSamplingBuffers();

        auto& state = m_animState;
    auto& ctl   = m_animCtl;

    state.current_time = 0.0f;
    state.time_ratio   = 0.0f;
    ctl.animation      = nullptr;
    ctl.playback_speed = 1.0f;

    m_isPlaying = false;
    m_isLooping = true;
}

bool OzzKinematicsAnimated::LoadAnimationFromFile(const std::filesystem::path& path)
{
    if (!LoadAnimationClipFromFile(path))
        return false;

    controllerMotion.invalidate();
    animationApplied = false;
    CalculateBones_Invalidate();
    return true;
}

void OzzKinematicsAnimated::StopAnimation()
{
    ClearActiveBlends(true);
    ClearPose();
    CalculateBones_Invalidate();

        auto& state = m_animState;
    state.current_time = 0.0f;
    state.time_ratio   = 0.0f;
    m_isPlaying = false;
}

bool OzzKinematicsAnimated::AdvanceAnimation(float dt)
{
    if (!core.IsInitialized())
        return false;

    if (!animationLoaded || !activeAnimation)
    {
        if (animationApplied)
        {
            ClearPose();
            animationApplied = false;
            return true;
        }
        return false;
    }

    const float duration = activeAnimation->duration();
    if (duration <= 0.f)
        return false;

    playbackTime += dt * playbackSpeed;

    if (loopPlayback && duration > 0.f)
    {
        while (playbackTime < 0.f)
            playbackTime += duration;
        while (playbackTime > duration)
            playbackTime -= duration;
    }
    else
    {
        if (playbackTime < 0.f)
            playbackTime = 0.f;
        if (playbackTime > duration)
            playbackTime = duration;
    }

    const float ratio = duration > 0.f ? playbackTime / duration : 0.f;

    if (sampledLocals.size() != static_cast<size_t>(core.Skeleton().num_soa_joints()))
        ResetSamplingBuffers();

    ozz::animation::SamplingJob job;
    job.animation = activeAnimation.get();
    job.context = &samplingContext;
    job.ratio = loopPlayback ? ratio : std::clamp(ratio, 0.f, 1.f);
    job.output = ozz::span<ozz::math::SoaTransform>(sampledLocals.data(), sampledLocals.size());

    if (!job.Run())
        return false;

    if (!SetPoseLocals(ozz::span<const ozz::math::SoaTransform>(sampledLocals.data(), sampledLocals.size())))
        return false;

    animationApplied = true;
    return true;
}

bool OzzKinematicsAnimated::HasLoadedAnimation() const
{
    return animationLoaded && activeAnimation;
}

void OzzKinematicsAnimated::SetLooping(bool loop)
{
    loopPlayback = loop;
}

void OzzKinematicsAnimated::SetPlaybackSpeed(float speed)
{
    playbackSpeed = speed;
}

float OzzKinematicsAnimated::AnimationDuration() const
{
    return (animationLoaded && activeAnimation) ? activeAnimation->duration() : 0.f;
}

#ifdef DEBUG
ozz::span<const ozz::math::SoaTransform> OzzKinematicsAnimated::DebugSampledLocals() const
{
    if (sampledLocals.empty())
        return {};
    return ozz::span<const ozz::math::SoaTransform>(sampledLocals.data(), sampledLocals.size());
}
#endif

void OzzKinematicsAnimated::InitializeSamplingState()
{
    ResetSamplingBuffers();
}

void OzzKinematicsAnimated::ResetSamplingBuffers()
{
    if (!core.IsInitialized())
    {
        sampledLocals.clear();
        return;
    }

    const int soa_count = core.Skeleton().num_soa_joints();
    sampledLocals.resize(static_cast<size_t>(soa_count));
    for (ozz::math::SoaTransform& transform : sampledLocals)
        transform = ozz::math::SoaTransform::identity();
}

bool OzzKinematicsAnimated::LoadAnimationClip(const std::shared_ptr<ozz::animation::Animation>& animation)
{
    if (!core.IsInitialized() || !animation)
        return false;

    if (animation->num_tracks() != core.Skeleton().num_joints())
        return false;

    activeAnimation = animation;
    samplingContext.Resize(animation->num_tracks());
    ResetSamplingBuffers();
    animationLoaded = true;
    playbackTime = 0.f;
    return true;
}

bool OzzKinematicsAnimated::LoadAnimationClipFromFile(const std::filesystem::path& path)
{
    if (!core.IsInitialized())
        return false;

    ozz::io::File file(path.string().c_str(), "rb");
    if (!file.opened())
        return false;

    ozz::io::IArchive archive(&file);
    std::shared_ptr<ozz::animation::Animation> animation = std::make_shared<ozz::animation::Animation>();
    if (archive.TestTag<ozz::animation::Animation>())
    {
        archive >> *animation;
    }
    else
    {
        file.Seek(0, ozz::io::Stream::kSet);
        ozz::io::IArchive aggregate(&file);

        uint32_t animation_count = 0;
        aggregate >> animation_count;
        if (animation_count == 0)
            return false;

        aggregate >> *animation;
        SkipOzzAnimationMetadata(aggregate);

        for (uint32_t idx = 1; idx < animation_count; ++idx)
        {
            ozz::animation::Animation discarded;
            aggregate >> discarded;
            SkipOzzAnimationMetadata(aggregate);
        }
    }

    return LoadAnimationClip(animation);
}

void OzzKinematicsAnimated::OnCalculateBones()
{
    UpdateTracks();
}

void OzzKinematicsAnimated::CalculateBones(BOOL bForceExact)
{
    if (m_isPlaying && activeAnimation)
        AdvanceAnimation(0.f);

    core.CalculateTransforms(!!bForceExact);
}

void OzzKinematicsAnimated::CalculateBonesFG(BOOL bForceExact)
{
    UpdateTracks();
    core.CalculateTransforms(!!bForceExact);
}

#ifdef DEBUG
std::pair<LPCSTR, LPCSTR> OzzKinematicsAnimated::LL_MotionDefName_dbg(MotionID /*ID*/)
{
    static xr_string empty;
    return { empty.c_str(), empty.c_str() };
}

void OzzKinematicsAnimated::LL_DumpBlends_dbg()
{
}
#endif

u32 OzzKinematicsAnimated::LL_PartBlendsCount(u32 /*bone_part_id*/)
{
    return 0;
}

CBlend* OzzKinematicsAnimated::LL_PartBlend(u32 /*bone_part_id*/, u32 /*n*/)
{
    return nullptr;
}

void OzzKinematicsAnimated::LL_IterateBlends(IterateBlendsCallback& /*callback*/) {}

u16 OzzKinematicsAnimated::LL_MotionsSlotCount()
{
    return 0;
}

const shared_motions& OzzKinematicsAnimated::LL_MotionsSlot(u16 /*idx*/)
{
    static shared_motions dummy;
    return dummy;
}

CMotionDef* OzzKinematicsAnimated::LL_GetMotionDef(MotionID id)
{
    if (!id.valid())
        return nullptr;

    if (id.slot < m_Motions.size() && g_pOzzMotionsContainer)
    {
        OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(m_Motions[id.slot].motions.GetHandle());

        if (value)
        {
            auto* record = value->FindMotion(id.idx);
            return record ? &record->definition : nullptr;
        }
    }

    return nullptr;
}

CMotion* OzzKinematicsAnimated::LL_GetRootMotion(MotionID id)
{
    if (!id.valid())
        return nullptr;

    const u16 root_bone = core.GetRootBone();
    const u16 resolved_root = (root_bone != BI_NONE) ? root_bone : u16(0);
    return LL_GetMotion(id, resolved_root);
}

CMotion* OzzKinematicsAnimated::LL_GetMotion(MotionID id, u16 bone_id)
{
    if (!id.valid() || id.slot >= m_Motions.size())
        return nullptr;

    if (!core.IsInitialized() || bone_id == BI_NONE)
        return nullptr;

    const u32 joint_count = static_cast<u32>(core.Skeleton().num_joints());
    if (bone_id >= joint_count)
        return nullptr;

    MotionVec* bone_motions = m_Motions[id.slot].bone_motions[bone_id];

    if (!bone_motions)
        return nullptr;

    if (id.idx >= bone_motions->size())
        return nullptr;

    return &bone_motions->at(id.idx);
}

void OzzKinematicsAnimated::LL_BuldBoneMatrixDequatize(const CBoneData* bd, u8 channel_mask, SKeyTable& keys)
{
    if (!core.IsInitialized() || !bd)
        return;

    const u16 self_id = bd->GetSelfID();
    if (self_id == BI_NONE)
        return;

    CKey base_keys[MAX_CHANNELS][MAX_BLENDED];

    if (activeBlends.empty())
        return;

    for (const ActiveBlendEntry& entry : activeBlends)
    {
        CBlend* blend = entry.blend;
        if (!blend)
            continue;

        const u8 channel = blend->channel;
        if (channel >= MAX_CHANNELS)
            continue;

        if ((channel_mask & (1 << channel)) == 0)
            continue;

        int& blend_count = keys.chanel_blend_conts[channel];
        if (blend_count >= static_cast<int>(MAX_BLENDED))
            continue;

        CMotion* motion = LL_GetMotion(blend->motionID, self_id);
        if (!motion)
            continue;

        CKey& destination = keys.keys[channel][blend_count];
        Render::key_identity(destination);
        Render::Dequantize(destination, *blend, *motion);

        CKey& base = base_keys[channel][blend_count];
        Render::key_identity(base);

        if (motion->_keysR.size())
            Render::QR2Quat(motion->_keysR[0], base.Q);

        if (motion->test_flag(flTKeyPresent))
        {
            if (motion->test_flag(flTKey16IsBit) && motion->_keysT16.size())
                Render::QT16_2T(motion->_keysT16[0], *motion, base.T);
            else if (motion->_keysT8.size())
                Render::QT8_2T(motion->_keysT8[0], *motion, base.T);
            else
                base.T.set(motion->_initT);
        }
        else
        {
            base.T.set(motion->_initT);
        }

        keys.blends[channel][blend_count] = blend;
        ++blend_count;
    }

    for (u16 channel = 0; channel < MAX_CHANNELS; ++channel)
    {
        if (channelRules[channel].extern_ == Render::animation::add)
            Render::keys_substruct(keys.keys[channel], base_keys[channel], keys.chanel_blend_conts[channel]);
    }
}

void OzzKinematicsAnimated::LL_BoneMatrixBuild(CBoneInstance& bi, const Fmatrix* parent, const SKeyTable& keys)
{
    CKey channel_keys[MAX_CHANNELS];
    Render::animation::channel_def channel_defs[MAX_CHANNELS];
    u16 channel_count = 0;

    for (u16 channel = 0; channel < MAX_CHANNELS; ++channel)
    {
        if (channel != 0 && keys.chanel_blend_conts[channel] == 0)
            continue;

        Render::animation::channel_def def{};
        def.rule = channelRules[channel];
        def.factor = channelFactors[channel];

        channel_defs[channel_count] = def;
        xray::render::fg::process_single_channel(channel_keys[channel_count], def, keys.keys[channel], keys.blends[channel],
            keys.chanel_blend_conts[channel]);
        ++channel_count;
    }

    if (channel_count == 0)
    {
        if (parent)
            bi.mTransform = *parent;
        else
            bi.mTransform.identity();
        return;
    }

    CKey mixed_key;
    xray::render::fg::MixChannels(mixed_key, channel_keys, channel_defs, channel_count);

    Fmatrix local_matrix;
    local_matrix.mk_xform(mixed_key.Q, mixed_key.T);

    if (parent)
        bi.mTransform.mul_43(*parent, local_matrix);
    else
        bi.mTransform = local_matrix;

#ifdef DEBUG
#    ifndef _EDITOR
    if (!xray::render::fg::check_scale(local_matrix))
    {
        VERIFY(xray::render::fg::check_scale(bi.mTransform));
    }
    VERIFY(_valid(bi.mTransform));

    Fbox dbg_box;
    constexpr float kBoxExtent = 100000.f;
    dbg_box.set(-kBoxExtent, -kBoxExtent, -kBoxExtent, kBoxExtent, kBoxExtent, kBoxExtent);
    VERIFY2(dbg_box.contains(bi.mTransform.c),
        make_string("[OzzKinematicsAnimated] bone transform out of bounds: (%.3f, %.3f, %.3f)", bi.mTransform.c.x, bi.mTransform.c.y, bi.mTransform.c.z).c_str());
#    endif
#endif
}

IBlendDestroyCallback* OzzKinematicsAnimated::GetBlendDestroyCallback()
{
    return blendDestroyCallback;
}

void OzzKinematicsAnimated::SetBlendDestroyCallback(IBlendDestroyCallback* cb)
{
    blendDestroyCallback = cb;
}

void OzzKinematicsAnimated::SetUpdateTracksCalback(IUpdateTracksCallback* callback)
{
    updateTracksCallback = callback;
}

IUpdateTracksCallback* OzzKinematicsAnimated::GetUpdateTracksCalback()
{
    return updateTracksCallback;
}

MotionID OzzKinematicsAnimated::LL_MotionID(LPCSTR B)
{
    if (!B || !*B)
        return MotionID();

    if (!g_pOzzMotionsContainer || m_Motions.empty())
        return MotionID();

    const xr_string motion_name(B);

    for (u16 slot = 0; slot < m_Motions.size(); ++slot)
    {
        OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(m_Motions[slot].motions.GetHandle());

        if (!value)
            continue;

        const auto lookup = value->lookup.find(motion_name);
        if (lookup == value->lookup.end())
            continue;

        return MotionID(slot, lookup->second);
    }

    return MotionID();
}

u16 OzzKinematicsAnimated::LL_PartID(LPCSTR /*B*/)
{
    return BI_NONE;
}

void OzzKinematicsAnimated::EnumerateCycleNames(xr_vector<shared_str>& outNames) const
{
    if (!g_pOzzMotionsContainer)
        return;

    for (const auto& slot : m_Motions)
    {
        OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(slot.motions.GetHandle());
        if (!value)
            continue;

        for (const auto& rec : value->records)
            outNames.emplace_back(rec.name.c_str());
    }
}

CBlend* OzzKinematicsAnimated::LL_PlayCycle(u16 partition, MotionID motion, BOOL bMixing, float blendAccrue, float blendFalloff, float Speed, BOOL noloop,
    PlayCallback Callback, LPVOID CallbackParam, u8 channel)
{
    static thread_local int s_play_depth = 0;
    struct DepthGuard { int& d; DepthGuard(int& v) : d(v) { ++d; } ~DepthGuard() { --d; } } _g(s_play_depth);
    if (s_play_depth > 8)
        Msg("! [LL_PlayCycle] depth=%d (recursion via callback?)", s_play_depth);
    if (s_play_depth > 200)
    {
        Msg("! [LL_PlayCycle] runaway recursion at depth=%d, aborting", s_play_depth);
        return nullptr;
    }

    if (!motion.valid() || motion.slot >= m_Motions.size())
        return nullptr;

    OzzMotionsValue::MotionRecord* record = nullptr;
    if (g_pOzzMotionsContainer)
    {
        OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(m_Motions[motion.slot].motions.GetHandle());

        if (value)
            record = value->FindMotion(motion.idx);
    }

    if (!record || !record->animation)
        return nullptr;

    // Msg("[PlayCycle] visual='%s' entity=%d slot=%u idx=%u motion='%s' tracks=%d skel_joints=%d",
    //     dbg_name.c_str() ? dbg_name.c_str() : "<unnamed>",
    //     (int)m_ecs_entity,
    //     (unsigned)motion.slot, (unsigned)motion.idx,
    //     record->name.c_str(),
    //     record->animation->num_tracks(),
    //     core.IsInitialized() ? core.Skeleton().num_joints() : -1);

    const u16 resolvedPartition = (partition == BI_NONE) ? u16(0) : partition;
    const bool mixing = !!bMixing;
    const bool stop_at_end = !!noloop;

    const MotionID resolvedMotion(motion.slot, record->id.idx);

    if (!mixing && !activeBlends.empty())
        ClearActiveBlends(true);

    const int existingIndex = FindActiveBlendIndex(resolvedPartition, channel);
    if (existingIndex >= 0)
        RemoveActiveBlend(static_cast<size_t>(existingIndex), true);

    if (!controllerMotion.valid() || controllerMotion != resolvedMotion)
    {
        if (!LoadAnimationClip(record->animation))
            return nullptr;

        controllerMotion = resolvedMotion;
        animationApplied = false;
        CalculateBones_Invalidate();
    }

    loopPlayback = !stop_at_end;

    CBlend* B = IBlend_Create();
    if (!B)
        return nullptr;

    activeBlends.emplace_back();
    ActiveBlendEntry& entry = activeBlends.back();
    entry.blend = B;
    entry.partition = resolvedPartition;
    entry.channel = channel;
    entry.recordIndex = resolvedMotion.idx;
    entry.motionId = resolvedMotion;

    const float def_speed = record->definition.Speed();
    float playback_speed_value = Speed;
    if (fis_zero(playback_speed_value))
        playback_speed_value = !fis_zero(def_speed) ? def_speed : 1.f;

        auto& state = m_animState;
    auto& ctl   = m_animCtl;

    const bool sameMotion = (ctl.animation == activeAnimation.get());
    if (!sameMotion)
    {
        state.current_time = 0.0f;
        state.time_ratio   = 0.0f;
    }
    ctl.animation      = activeAnimation.get();
    ctl.playback_speed = playback_speed_value;

    m_isPlaying = true;
    if (loopPlayback)
        m_isLooping = true;
    else
        m_isLooping = false;

    CBlend& blend = *entry.blend;
    blend.set_accrue_state();
    blend.blendAmount = mixing ? EPS_S : 1.f;
    blend.blendAccrue = blendAccrue;
    blend.blendFalloff = blendFalloff;
    blend.blendPower = 1.f;

    blend.speed = playback_speed_value;
    SetPlaybackSpeed(playback_speed_value);

    blend.motionID = resolvedMotion;
    blend.timeCurrent = 0.f;
    const float animationDuration = record->animation ? record->animation->duration() : 0.f;
    float motionLength = animationDuration;
    if (motionLength <= 0.f && record->frameCount > 0)
        motionLength = static_cast<float>(record->frameCount) * SAMPLE_SPF;
    if (motionLength <= 0.f)
        motionLength = SAMPLE_SPF;
    blend.timeTotal = motionLength;
    blend.bone_or_part = resolvedPartition;
    blend.stop_at_end = stop_at_end;
    blend.playing = true;
    blend.stop_at_end_callback = true;
    blend.Callback = Callback;
    blend.CallbackParam = CallbackParam;
    blend.channel = channel;
    blend.fall_at_end = blend.stop_at_end && (channel > 1);
    blend.dwFrame = Device.dwFrame ? Device.dwFrame - 1 : 0;

    animationApplied = false;

    return activeBlends.size() ? activeBlends.back().blend : nullptr;
}

CBlend* OzzKinematicsAnimated::LL_PlayCycle(u16 partition, MotionID motion, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel)
{
    CMotionDef* def = LL_GetMotionDef(motion);
    if (!def)
        return nullptr;

    return LL_PlayCycle(partition, motion, bMixIn, def->Accrue(), def->Falloff(), def->Speed(), def->StopAtEnd(), Callback, CallbackParam, channel);
}

void OzzKinematicsAnimated::LL_CloseCycle(u16 partition, u8 mask_channel)
{
    if (activeBlends.empty())
        return;

    const u16 resolvedPartition = (partition == BI_NONE) ? u16(0) : partition;
    size_t index = 0;
    while (index < activeBlends.size())
    {
        const ActiveBlendEntry& entry = activeBlends[index];
        const bool partitionMatch = (partition == BI_NONE) || (entry.partition == resolvedPartition);
        const bool channelMatch = (mask_channel & (1 << entry.channel)) != 0;
        if (partitionMatch && channelMatch)
        {
            RemoveActiveBlend(index, true);
            continue;
        }
        ++index;
    }

    if (activeBlends.empty())
        m_isPlaying = false;
}

void OzzKinematicsAnimated::LL_SetChannelFactor(u16 channel, float factor)
{
    if (channel < MAX_CHANNELS)
        channelFactors[channel] = factor;
}

void OzzKinematicsAnimated::UpdateTracks()
{
    LL_UpdateTracks(Device.fTimeDelta, true, false);
}

void OzzKinematicsAnimated::LL_UpdateTracks(float dt, bool b_force, bool leave_blends)
{
    auto& state = m_animState;
    auto& ctl   = m_animCtl;

    if (activeAnimation)
    {
        ctl.animation      = activeAnimation.get();
        ctl.playback_speed = playbackSpeed;
        m_isPlaying = animationLoaded && !activeBlends.empty();
        m_isLooping = loopPlayback;
    }

    if (m_isPlaying && activeAnimation)
    {
        AdvanceAnimation(dt);
        state.current_time = playbackTime;
        state.time_ratio   = activeAnimation->duration() > 0.f ? playbackTime / activeAnimation->duration() : 0.f;
    }

    if (!activeBlends.empty())
    {
        size_t index = 0;
        while (index < activeBlends.size())
        {
            ActiveBlendEntry& entry = activeBlends[index];
            CBlend& blend = *entry.blend;

            if (b_force || blend.dwFrame != Device.dwFrame)
            {
                blend.dwFrame = Device.dwFrame;
                blend.timeCurrent = state.current_time;

                const bool finished = blend.update(dt, blend.Callback);
                if (finished && !leave_blends)
                {
                    RemoveActiveBlend(index, true);
                    continue;
                }
            }

            ++index;
        }
    }

    if (updateTracksCallback)
        (*updateTracksCallback)(dt, *this);
}

MotionID OzzKinematicsAnimated::ID_Cycle(LPCSTR N)
{
    MotionID id = ID_Cycle_Safe(N);
    if (!id.valid())
        return MotionID();

    return id;
}

MotionID OzzKinematicsAnimated::ID_Cycle_Safe(LPCSTR N)
{
    if (!N || !N[0])
        return MotionID();

    return LL_MotionID(N);
}

MotionID OzzKinematicsAnimated::ID_Cycle(shared_str N)
{
    MotionID id = ID_Cycle_Safe(N);
    if (!id.valid())
        return MotionID();

    return id;
}

MotionID OzzKinematicsAnimated::ID_Cycle_Safe(shared_str N)
{
    if (!N || !N.c_str() || !N.c_str()[0])
        return MotionID();

    return LL_MotionID(N.c_str());
}

CBlend* OzzKinematicsAnimated::PlayCycle(LPCSTR N, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel)
{
    MotionID id = ID_Cycle_Safe(N);
    if (!id.valid())
        return nullptr;

    return PlayCycle(id, bMixIn, Callback, CallbackParam, channel);
}

CBlend* OzzKinematicsAnimated::PlayCycle(MotionID M, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel)
{
    CMotionDef* def = LL_GetMotionDef(M);
    if (!def)
        return nullptr;

    return LL_PlayCycle(def->bone_or_part, M, bMixIn, def->Accrue(), def->Falloff(), def->Speed(), def->StopAtEnd(), Callback, CallbackParam, channel);
}

CBlend* OzzKinematicsAnimated::PlayCycle(u16 partition, MotionID M, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel)
{
    return LL_PlayCycle(partition, M, bMixIn, Callback, CallbackParam, channel);
}

MotionID OzzKinematicsAnimated::ID_FX(LPCSTR /*N*/)
{
    return MotionID();
}

MotionID OzzKinematicsAnimated::ID_FX_Safe(LPCSTR /*N*/)
{
    return MotionID();
}

CBlend* OzzKinematicsAnimated::PlayFX(LPCSTR /*N*/, float /*power_scale*/)
{
    return nullptr;
}

CBlend* OzzKinematicsAnimated::PlayFX(MotionID /*M*/, float /*power_scale*/)
{
    return nullptr;
}

CBlend* OzzKinematicsAnimated::PlayFX_Safe(cpcstr /*N*/, float /*power_scale*/)
{
    return nullptr;
}

const CPartition& OzzKinematicsAnimated::partitions() const
{
    return defaultPartition;
}

float OzzKinematicsAnimated::get_animation_length(MotionID /*motion_ID*/)
{
    return AnimationDuration();
}

void OzzKinematicsAnimated::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset)
{
    OzzKinematics::LL_AddTransformToBone(offset);
}

void OzzKinematicsAnimated::LL_ClearAdditionalTransform(u16 bone_id)
{
    OzzKinematics::LL_ClearAdditionalTransform(bone_id);
}

u16 OzzKinematicsAnimated::GetAvailableMotionCount() const
{
    if (!g_pOzzMotionsContainer)
        return 0;

    u32 total_count = 0;
    for (const auto& slot : m_Motions)
    {
        if (const OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(slot.motions.GetHandle()))
        {
            total_count += value->GetMotionCount();
        }
    }

    return static_cast<u16>(std::min(total_count, static_cast<u32>(std::numeric_limits<u16>::max())));
}

bool OzzKinematicsAnimated::GetMotionName(u16 index, xr_string& out_name) const
{
    if (!g_pOzzMotionsContainer)
        return false;

    u16 current_offset = 0;
    for (const auto& slot : m_Motions)
    {
        const OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(slot.motions.GetHandle());
        if (!value)
            continue;

        const u16 slot_count = value->GetMotionCount();
        if (index < current_offset + slot_count)
        {
            const u16 local_index = index - current_offset;
            if (const auto* record = value->FindMotion(local_index))
            {
                out_name = record->name;
                return true;
            }
            return false;
        }

        current_offset += slot_count;
    }

    return false;
}

bool OzzKinematicsAnimated::GetMotionInfo(u16 index, xr_string& out_name, float& out_duration) const
{
    if (!g_pOzzMotionsContainer)
        return false;

    u16 current_offset = 0;
    for (const auto& slot : m_Motions)
    {
        const OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(slot.motions.GetHandle());
        if (!value)
            continue;

        const u16 slot_count = value->GetMotionCount();
        if (index < current_offset + slot_count)
        {
            const u16 local_index = index - current_offset;
            if (const auto* record = value->FindMotion(local_index))
            {
                out_name = record->name;
                out_duration = record->animation ? record->animation->duration() : 0.f;
                return true;
            }
            return false;
        }

        current_offset += slot_count;
    }

    return false;
}

OzzMotionsContainer* OzzKinematicsAnimated::GetMotionsContainer() const
{
    return g_pOzzMotionsContainer;
}

const ozz::animation::Animation* OzzKinematicsAnimated::ResolveMotionAnimation(MotionID id) const
{
    if (!id.valid() || id.slot >= m_Motions.size() || !g_pOzzMotionsContainer)
        return nullptr;

    OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(m_Motions[id.slot].motions.GetHandle());
    if (!value)
        return nullptr;

    OzzMotionsValue::MotionRecord* record = value->FindMotion(id.idx);
    if (!record || !record->animation)
        return nullptr;

    return record->animation.get();
}

} // namespace Animation
} // namespace XRay
