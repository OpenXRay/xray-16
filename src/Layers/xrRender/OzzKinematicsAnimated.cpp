#include "stdafx.h"

#include "OzzKinematicsAnimated.h"
#include "OzzMesh.h"

#include "Layers/xrRender/AnimationKeyCalculate.h"
#include "OzzConversion.h"

#include "framework/mesh.h"

#include "xrCore/Animation/Motion.hpp"
#include "xrCore/_std_extensions.h"
#include "xrEngine/device.h"

#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/animation/runtime/local_to_model_job.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>

namespace Render = xray::render::fg;

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
}

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

    auto* src = static_cast<OzzKinematicsAnimated*>(pFrom);

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

void OzzKinematicsAnimated::OnSkeletonLoaded()
{
    EnsureMotionLibraryLoaded();

    if (IsInitialized())
    {
        defaultPartition[0].Name = "default";
        const u16 bone_count = LL_BoneCount();
        defaultPartition[0].bones.resize(bone_count);
        for (u16 i = 0; i < bone_count; ++i)
            defaultPartition[0].bones[i] = i;
    }
}

bool OzzKinematicsAnimated::InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData, const xr_vector<xr_string>& motionRefs)
{
    motionReferences = motionRefs;

    if (!OzzKinematics::InitializeFromOzzBuffer(skeletonData))
    {
        motionReferences.clear();
        return false;
    }

    ResetSamplingBuffers();
    ResetPlaybackState();

    InitializeChannelState();

    return true;
}

void OzzKinematicsAnimated::SetEmbeddedAnimationData(const std::vector<std::uint8_t>& data)
{
    embeddedAnimationData = data;
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
    if (!g_pOzzMotionsContainer || !IsInitialized())
        return;

    m_Motions.clear();

    SkeletonFingerprint skelFP = SkeletonFingerprint::Compute(Skeleton());

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

        if (!slot.motions.Create(request, Skeleton()))
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

        if (!slot.motions.Create(request, Skeleton()))
        {
            m_Motions.pop_back();
            continue;
        }

        BuildBoneMotionCache(slot);
    }
}

void OzzKinematicsAnimated::BuildBoneMotionCache(SMotionsSlot& slot)
{
    if (!IsInitialized())
        return;

    const u32 bone_count = Skeleton().num_joints();
    slot.bone_motions.resize(bone_count);

    auto joint_names = Skeleton().joint_names();
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
    if (activeBlends.empty())
    {
        ResetPlaybackState();
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
}

bool OzzKinematicsAnimated::AdvanceAnimation(float dt)
{
    if (!IsInitialized())
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

    if (sampledLocals.size() != static_cast<size_t>(Skeleton().num_soa_joints()))
        ResetSamplingBuffers();

    ozz::animation::SamplingJob job;
    job.animation = activeAnimation.get();
    job.context = &samplingContext;
    job.ratio = loopPlayback ? ratio : std::clamp(ratio, 0.f, 1.f);
    job.output = ozz::span<ozz::math::SoaTransform>(sampledLocals.data(), sampledLocals.size());

    if (!job.Run())
        return false;

    poseValid = true;
    CalculateBones_Invalidate();

    animationApplied = true;
    return true;
}

void OzzKinematicsAnimated::ResetSamplingBuffers()
{
    if (!IsInitialized())
    {
        sampledLocals.clear();
        poseValid = false;
        return;
    }

    const size_t soa_count = static_cast<size_t>(Skeleton().num_soa_joints());
    if (sampledLocals.size() == soa_count)
        return;

    sampledLocals.resize(soa_count);
    for (ozz::math::SoaTransform& transform : sampledLocals)
        transform = ozz::math::SoaTransform::identity();
    poseValid = false;
}

bool OzzKinematicsAnimated::LoadAnimationClip(const std::shared_ptr<ozz::animation::Animation>& animation)
{
    if (!IsInitialized() || !animation)
        return false;

    if (animation->num_tracks() != Skeleton().num_joints())
        return false;

    activeAnimation = animation;
    samplingContext.Resize(animation->num_tracks());
    ResetSamplingBuffers();
    animationLoaded = true;
    playbackTime = 0.f;
    return true;
}

#ifdef DEBUG
std::pair<LPCSTR, LPCSTR> OzzKinematicsAnimated::LL_MotionDefName_dbg(MotionID)
{
    static xr_string empty;
    return { empty.c_str(), empty.c_str() };
}

void OzzKinematicsAnimated::LL_DumpBlends_dbg()
{
}
#endif

u32 OzzKinematicsAnimated::LL_PartBlendsCount(u32)
{
    return 0;
}

CBlend* OzzKinematicsAnimated::LL_PartBlend(u32, u32)
{
    return nullptr;
}

void OzzKinematicsAnimated::LL_IterateBlends(IterateBlendsCallback&) {}

u16 OzzKinematicsAnimated::LL_MotionsSlotCount()
{
    return 0;
}

const shared_motions& OzzKinematicsAnimated::LL_MotionsSlot(u16)
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

    const u16 root_bone = LL_GetBoneRoot();
    const u16 resolved_root = (root_bone != BI_NONE) ? root_bone : u16(0);
    return LL_GetMotion(id, resolved_root);
}

CMotion* OzzKinematicsAnimated::LL_GetMotion(MotionID id, u16 bone_id)
{
    if (!id.valid() || id.slot >= m_Motions.size())
        return nullptr;

    if (!IsInitialized() || bone_id == BI_NONE)
        return nullptr;

    const u32 joint_count = static_cast<u32>(Skeleton().num_joints());
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
    if (!IsInitialized() || !bd)
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

u16 OzzKinematicsAnimated::LL_PartID(LPCSTR)
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

    CBlend& blend = *entry.blend;
    blend.set_accrue_state();
    blend.blendAmount = mixing ? EPS_S : 1.f;
    blend.blendAccrue = blendAccrue;
    blend.blendFalloff = blendFalloff;
    blend.blendPower = 1.f;

    blend.speed = playback_speed_value;
    playbackSpeed = playback_speed_value;

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
    if (activeAnimation && animationLoaded && !activeBlends.empty())
        AdvanceAnimation(dt);

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
                blend.timeCurrent = playbackTime;

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

MotionID OzzKinematicsAnimated::ID_FX(LPCSTR)
{
    return MotionID();
}

MotionID OzzKinematicsAnimated::ID_FX_Safe(LPCSTR)
{
    return MotionID();
}

CBlend* OzzKinematicsAnimated::PlayFX(LPCSTR, float)
{
    return nullptr;
}

CBlend* OzzKinematicsAnimated::PlayFX(MotionID, float)
{
    return nullptr;
}

CBlend* OzzKinematicsAnimated::PlayFX_Safe(cpcstr, float)
{
    return nullptr;
}

const CPartition& OzzKinematicsAnimated::partitions() const
{
    return defaultPartition;
}

float OzzKinematicsAnimated::get_animation_length(MotionID)
{
    return (animationLoaded && activeAnimation) ? activeAnimation->duration() : 0.f;
}

void OzzKinematicsAnimated::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset)
{
    OzzKinematics::LL_AddTransformToBone(offset);
}

void OzzKinematicsAnimated::LL_ClearAdditionalTransform(u16 bone_id)
{
    OzzKinematics::LL_ClearAdditionalTransform(bone_id);
}
}
}
