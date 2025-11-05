#include "stdafx.h"

#include "OzzKinematicsAnimated.h"

#include "Layers/xrRender/AnimationKeyCalculate.h"
#include "OzzConversion.h"

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
namespace Render = xray::render::RENDER_NAMESPACE;

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

std::string ReadOzzString(ozz::io::IArchive& archive)
{
    uint32_t length = 0;
    archive >> length;
    std::string result;
    if (length == 0)
        return result;

    result.resize(length);
    archive >> ozz::io::MakeArray(result.data(), length);
    return result;
}

void SkipOzzAnimationMetadata(ozz::io::IArchive& archive)
{
    ReadOzzString(archive);

    uint32_t flags = 0;
    uint16_t bone_or_part = 0;
    uint16_t motion_id = 0;
    float speed = 0.f;
    float power = 0.f;
    float accrue = 0.f;
    float falloff = 0.f;

    archive >> flags;
    archive >> bone_or_part;
    archive >> motion_id;
    archive >> speed;
    archive >> power;
    archive >> accrue;
    archive >> falloff;

    uint32_t mark_count = 0;
    archive >> mark_count;
    for (uint32_t mark_index = 0; mark_index < mark_count; ++mark_index)
    {
        ReadOzzString(archive);

        uint32_t interval_count = 0;
        archive >> interval_count;
        for (uint32_t interval_index = 0; interval_index < interval_count; ++interval_index)
        {
            float first = 0.f;
            float second = 0.f;
            archive >> first;
            archive >> second;
        }
    }
}

struct MotionBoneData
{
    u16 bone_id = BI_NONE;
    u8 flags = 0;
    u8 translation_format = 0;
    u32 rotation_crc = 0;
    u32 translation_crc = 0;
    xr_vector<CKeyQR> rotation_keys;
    xr_vector<CKeyQT8> translation_keys8;
    xr_vector<CKeyQT16> translation_keys16;
    Fvector translation_size{};
    Fvector translation_init{};
};

struct MotionMetadata
{
    xr_string name;
    uint32_t flags = 0;
    uint16_t bone_or_part = 0;
    uint16_t motion_id = 0;
    float speed = 1.f;
    float power = 1.f;
    float accrue = 0.f;
    float falloff = 0.f;
    uint32_t frame_count = 0;
    xr_vector<MotionBoneData> bone_motions;
};

MotionMetadata ReadMotionMetadataFromArchive(ozz::io::IArchive& archive)
{
    MotionMetadata metadata;

    std::string name = ReadOzzString(archive);
    metadata.name = name.c_str();

    archive >> metadata.flags;
    archive >> metadata.bone_or_part;
    archive >> metadata.motion_id;
    archive >> metadata.speed;
    archive >> metadata.power;
    archive >> metadata.accrue;
    archive >> metadata.falloff;

    uint32_t mark_count = 0;
    archive >> mark_count;

    for (uint32_t mark_index = 0; mark_index < mark_count; ++mark_index)
    {
        ReadOzzString(archive);

        uint32_t interval_count = 0;
        archive >> interval_count;
        for (uint32_t interval_index = 0; interval_index < interval_count; ++interval_index)
        {
            float start = 0.f;
            float end = 0.f;
            archive >> start;
            archive >> end;
        }
    }

    archive >> metadata.frame_count;

    uint32_t bone_motion_count = 0;
    archive >> bone_motion_count;
    metadata.bone_motions.resize(bone_motion_count);

    for (uint32_t bone_index = 0; bone_index < bone_motion_count; ++bone_index)
    {
        MotionBoneData& bone = metadata.bone_motions[bone_index];

        archive >> bone.bone_id;

        uint8_t flags = 0;
        archive >> flags;
        bone.flags = flags;

        uint8_t translation_format = 0;
        archive >> translation_format;
        bone.translation_format = translation_format;

        archive >> bone.rotation_crc;
        archive >> bone.translation_crc;

        uint32_t rotation_key_count = 0;
        archive >> rotation_key_count;
        bone.rotation_keys.clear();
        bone.rotation_keys.resize(rotation_key_count);
        for (uint32_t key_index = 0; key_index < rotation_key_count; ++key_index)
        {
            s16 x = 0;
            s16 y = 0;
            s16 z = 0;
            s16 w = 0;
            archive >> x;
            archive >> y;
            archive >> z;
            archive >> w;
            bone.rotation_keys[key_index].x = x;
            bone.rotation_keys[key_index].y = y;
            bone.rotation_keys[key_index].z = z;
            bone.rotation_keys[key_index].w = w;
        }

        uint32_t translation_key_count = 0;
        archive >> translation_key_count;

        switch (translation_format)
        {
        case 1:
            bone.translation_keys8.clear();
            bone.translation_keys8.resize(translation_key_count);
            bone.translation_keys16.clear();
            for (uint32_t key_index = 0; key_index < translation_key_count; ++key_index)
            {
                s8 x = 0;
                s8 y = 0;
                s8 z = 0;
                archive >> x;
                archive >> y;
                archive >> z;
                bone.translation_keys8[key_index].x1 = x;
                bone.translation_keys8[key_index].y1 = y;
                bone.translation_keys8[key_index].z1 = z;
            }
            break;
        case 2:
            bone.translation_keys16.clear();
            bone.translation_keys16.resize(translation_key_count);
            bone.translation_keys8.clear();
            for (uint32_t key_index = 0; key_index < translation_key_count; ++key_index)
            {
                s16 x = 0;
                s16 y = 0;
                s16 z = 0;
                archive >> x;
                archive >> y;
                archive >> z;
                bone.translation_keys16[key_index].x1 = x;
                bone.translation_keys16[key_index].y1 = y;
                bone.translation_keys16[key_index].z1 = z;
            }
            break;
        default:
            bone.translation_keys8.clear();
            bone.translation_keys16.clear();
            break;
        }

        archive >> bone.translation_size.x;
        archive >> bone.translation_size.y;
        archive >> bone.translation_size.z;
        archive >> bone.translation_init.x;
        archive >> bone.translation_init.y;
        archive >> bone.translation_init.z;
    }

    return metadata;
}

xr_string BuildMotionNameFromPath(const xr_string& path)
{
    xr_string name = path;
    const size_t slash = name.find_last_of("/\\");
    if (slash != xr_string::npos)
        name.erase(0, slash + 1);

    const size_t dot = name.find_last_of('.');
    if (dot != xr_string::npos)
        name.erase(dot);

    return name;
}

u32 CalculateFrameCountFromAnimation(const ozz::animation::Animation* animation)
{
    if (!animation)
        return 1;

    const float duration = animation->duration();
    if (!std::isfinite(duration) || duration <= 0.f)
        return 1;

    const double frames = static_cast<double>(duration) * static_cast<double>(SAMPLE_FPS);
    const double rounded = std::round(frames);
    if (!std::isfinite(rounded) || rounded <= 0.0)
        return 1;

    const double clamped = std::min(rounded, static_cast<double>(std::numeric_limits<u32>::max()));
    const u32 frame_count = static_cast<u32>(clamped);
    return frame_count > 0 ? frame_count : 1u;
}

u32 DetermineFrameCount(const MotionMetadata& metadata, const ozz::animation::Animation* animation)
{
    if (metadata.frame_count > 0)
        return metadata.frame_count;
    return CalculateFrameCountFromAnimation(animation);
}

u32 ComputeOrDefaultCrc(u32 provided_crc, const void* data, size_t byte_count)
{
    if (provided_crc != 0 || byte_count == 0 || data == nullptr)
        return provided_crc;
    return crc32(data, static_cast<u32>(byte_count));
}
} // namespace

OzzKinematicsAnimated::OzzKinematicsAnimated()
{
    InitializeChannelState();
    ResetPlaybackState();
    IBlend_Startup();

    // Initialize ECS if enabled
    if (m_use_ecs)
    {
        auto& registry = AnimationECS::GetAnimationRegistry();
        registry.Initialize();
        m_ecs_entity = registry.CreateAnimatedEntity();
    }
}

OzzKinematicsAnimated::~OzzKinematicsAnimated()
{
    // Destroy ECS entity if enabled
    if (m_use_ecs && m_ecs_entity != entt::null)
    {
        auto& registry = AnimationECS::GetAnimationRegistry();
        if (registry.IsValidEntity(m_ecs_entity))
        {
            registry.DestroyAnimatedEntity(m_ecs_entity);
        }
        m_ecs_entity = entt::null;
    }

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

    // Initialize ECS components when skeleton is loaded
    if (m_use_ecs && m_ecs_entity != entt::null && core.IsInitialized())
    {
        auto& registry = AnimationECS::GetAnimationRegistry();

        // Initialize AnimationBuffers with skeleton
        auto* buffers = registry.GetComponent<AnimationECS::AnimationBuffers>(m_ecs_entity);
        if (buffers)
        {
            buffers->Initialize(&core.Skeleton());
        }

        // Set skeleton in AnimationController
        auto* controller = registry.GetComponent<AnimationECS::AnimationController>(m_ecs_entity);
        if (controller)
        {
            controller->SetSkeleton(&core.Skeleton(), shared_str("ozz_skeleton"));
        }

        // Initialize IK configuration
        // Compute bind pose models from rest pose
        const int num_joints = core.Skeleton().num_joints();
        const int num_soa_joints = core.Skeleton().num_soa_joints();

        ozz::vector<ozz::math::SoaTransform> bind_locals;
        bind_locals.resize(num_soa_joints);
        for (int i = 0; i < num_soa_joints; ++i)
        {
            bind_locals[i] = core.Skeleton().joint_rest_poses()[i];
        }

        ozz::vector<ozz::math::Float4x4> bind_models;
        bind_models.resize(num_joints);

        ozz::animation::LocalToModelJob ltm_job;
        ltm_job.skeleton = &core.Skeleton();
        ltm_job.input = ozz::make_span(bind_locals);
        ltm_job.output = ozz::make_span(bind_models);
        if (ltm_job.Run())
        {
            // Auto-detect IK chains (legs/arms) based on bone names
            AnimationECS::IKInitializationSystem::Initialize(
                registry.GetRegistry(),
                m_ecs_entity,
                core.Skeleton().joint_names(),
                ozz::make_span(bind_models)
            );

            Msg("[OzzKinematicsAnimated] ECS components initialized for skeleton with IK");
        }
        else
        {
            Msg("! [OzzKinematicsAnimated] Failed to compute bind pose models for IK initialization");
        }
    }
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
        xr_string embeddedKey = xr_string("<embedded_") + xr_string(std::to_string(skelFP.hash).c_str()) + xr_string(">");

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

    // Reset ECS animation state
    if (m_use_ecs && m_ecs_entity != entt::null)
    {
        auto& registry = AnimationECS::GetAnimationRegistry();
        auto* state = registry.GetComponent<AnimationECS::AnimationState>(m_ecs_entity);
        auto* controller = registry.GetComponent<AnimationECS::AnimationController>(m_ecs_entity);

        if (state)
        {
            state->is_playing = false;
            state->is_looping = true;
            state->current_time = 0.0f;
            state->time_ratio = 0.0f;
            state->current_partition = 0;
            state->blend_amount = 1.0f;
        }

        if (controller)
        {
            controller->animation = nullptr;
            controller->playback_speed = 1.0f;
        }
    }
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

    // Stop ECS animation
    if (m_use_ecs && m_ecs_entity != entt::null)
    {
        auto& registry = AnimationECS::GetAnimationRegistry();
        auto* state = registry.GetComponent<AnimationECS::AnimationState>(m_ecs_entity);
        if (state)
        {
            state->is_playing = false;
            state->current_time = 0.0f;
            state->time_ratio = 0.0f;
        }
    }
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

    ozz::io::File file(path.u8string().c_str(), "rb");
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
    core.CalculateTransforms(bForceExact);
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
        xray::render::RENDER_NAMESPACE::process_single_channel(channel_keys[channel_count], def, keys.keys[channel], keys.blends[channel],
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
    xray::render::RENDER_NAMESPACE::MixChannels(mixed_key, channel_keys, channel_defs, channel_count);

    Fmatrix local_matrix;
    local_matrix.mk_xform(mixed_key.Q, mixed_key.T);

    if (parent)
        bi.mTransform.mul_43(*parent, local_matrix);
    else
        bi.mTransform = local_matrix;

#ifdef DEBUG
#    ifndef _EDITOR
    if (!xray::render::RENDER_NAMESPACE::check_scale(local_matrix))
    {
        VERIFY(xray::render::RENDER_NAMESPACE::check_scale(bi.mTransform));
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

    if (!mixing && !activeBlends.empty())
        ClearActiveBlends(true);

    const MotionID resolvedMotion(motion.slot, record->id.idx);

    if (!controllerMotion.valid() || controllerMotion != resolvedMotion)
    {
        if (!LoadAnimationClip(record->animation))
            return nullptr;

        controllerMotion = resolvedMotion;
        animationApplied = false;
        CalculateBones_Invalidate();
    }

    loopPlayback = !stop_at_end;

    const int existingIndex = FindActiveBlendIndex(resolvedPartition, channel);
    if (existingIndex >= 0)
        RemoveActiveBlend(static_cast<size_t>(existingIndex), true);

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

    // Initialize ECS animation when starting playback
    if (m_use_ecs && m_ecs_entity != entt::null && core.IsInitialized())
    {
        auto& registry = AnimationECS::GetAnimationRegistry();

        auto* state = registry.GetComponent<AnimationECS::AnimationState>(m_ecs_entity);
        auto* controller = registry.GetComponent<AnimationECS::AnimationController>(m_ecs_entity);

        if (state && controller)
        {
            state->is_playing = true;
            state->is_looping = loopPlayback;
            state->current_time = 0.0f;
            state->time_ratio = 0.0f;
            state->current_partition = resolvedPartition;

            controller->animation = activeAnimation.get();
            controller->playback_speed = Speed;

            // Set animation name for debugging
            if (record)
            {
                controller->SetAnimation(activeAnimation.get(), shared_str(record->name.c_str()));
            }
        }

        // Set up callbacks in ECS
        auto* callbacks = registry.GetComponent<AnimationECS::AnimationCallbacks>(m_ecs_entity);
        if (callbacks)
        {
            callbacks->callback_param = B;  // Pass CBlend pointer to callbacks
            callbacks->on_play = Callback;
            callbacks->on_end = Callback;
        }
    }

    CBlend& blend = *entry.blend;
    blend.set_accrue_state();
    blend.blendAmount = mixing ? EPS_S : 1.f;
    blend.blendAccrue = blendAccrue;
    blend.blendFalloff = blendFalloff;
    blend.blendPower = 1.f;

    const float def_speed = record->definition.Speed();
    float playback_speed_value = Speed;
    if (fis_zero(playback_speed_value))
        playback_speed_value = !fis_zero(def_speed) ? def_speed : 1.f;
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

    // Stop ECS animation if all blends were removed
    if (activeBlends.empty() && m_use_ecs && m_ecs_entity != entt::null)
    {
        auto& registry = AnimationECS::GetAnimationRegistry();
        auto* state = registry.GetComponent<AnimationECS::AnimationState>(m_ecs_entity);
        if (state)
        {
            state->is_playing = false;
        }
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
    // ECS-driven animation pipeline
    if (m_use_ecs && m_ecs_entity != entt::null && core.IsInitialized())
    {
        auto& registry = AnimationECS::GetAnimationRegistry();

        // Sync current animation state to ECS components
        auto* state = registry.GetComponent<AnimationECS::AnimationState>(m_ecs_entity);
        auto* controller = registry.GetComponent<AnimationECS::AnimationController>(m_ecs_entity);
        auto* buffers = registry.GetComponent<AnimationECS::AnimationBuffers>(m_ecs_entity);

        if (state && controller && buffers && activeAnimation)
        {
            // Update ECS state from legacy blend system
            state->is_playing = animationLoaded && !activeBlends.empty();
            state->is_looping = loopPlayback;
            controller->animation = activeAnimation.get();
            controller->playback_speed = playbackSpeed;

            // Set partition from active blends
            if (!activeBlends.empty() && activeBlends.front().blend)
            {
                state->current_partition = activeBlends.front().partition;
            }
        }

        // NOTE: Multi-blend support removed - ECS systems will handle sampling
        // Previously we were sampling animations here AND in ECS systems (duplicate work!)
        // Now we let AnimationSamplingSystem do ALL sampling for better performance

        // Remove BlendState if it exists (multi-blend not yet supported in optimized path)
        if (registry.HasComponent<AnimationECS::BlendState>(m_ecs_entity))
        {
            registry.RemoveComponent<AnimationECS::BlendState>(m_ecs_entity);
        }

        // IMPORTANT: Do NOT call registry.Update() here!
        // The global batched update will handle all entities at once for much better performance
        // Calling Update() 500 times (once per character) was causing the performance drop

        // Just apply ECS animation results to skeleton (already computed by batched update)
        if (state && buffers && buffers->IsInitialized())
        {
            // Use ECS-generated local transforms instead of legacy sampledLocals
            ozz::span<const ozz::math::SoaTransform> ecs_locals(buffers->locals.data(), buffers->locals.size());

            if (!SetPoseLocals(ecs_locals))
            {
                Msg("! [OzzKinematicsAnimated] Failed to apply ECS animation results");
            }
            else
            {
                animationApplied = true;
            }

            // Sync playback time back from ECS
            playbackTime = state->current_time;
        }

        // Update blend system for compatibility (callbacks, state tracking)
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

                    // Sync blend time from ECS state
                    if (state)
                    {
                        blend.timeCurrent = state->current_time;
                    }

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

        return;
    }

    // Legacy path (used when m_use_ecs is false)
    if (!activeBlends.empty())
    {
        const CBlend* primaryBlend = activeBlends.front().blend;
        if (primaryBlend)
        {
            loopPlayback = !primaryBlend->stop_at_end;
            SetPlaybackSpeed(primaryBlend->speed);
        }
    }

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

} // namespace Animation
} // namespace XRay
