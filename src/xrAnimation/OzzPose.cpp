#include "stdafx.h"
#include "OzzPose.h"
#include "OzzSkeletonMirror.h"

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_transform.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace XRay::Animation
{
namespace
{
using namespace ozz::math;
constexpr u16 noBlend = u16(-1);
constexpr u16 channelCount = MAX_CHANNELS;
constexpr u16 boneBlendCapacity = MAX_BLENDED;

SoaTransform EmptyPose()
{
    auto pose = SoaTransform::identity();
    pose.rotation.w = simd_float4::zero();
    return pose;
}

template <size_t sourceLane>
void CopyPoseLane(SoaTransform& target, int lane, const SoaTransform& source)
{
    const auto copy = [lane](SimdFloat4 target, SimdFloat4 source)
    {
        return SetI(target, Swizzle<sourceLane, sourceLane, sourceLane, sourceLane>(source), lane);
    };
    target.translation = {copy(target.translation.x, source.translation.x),
        copy(target.translation.y, source.translation.y), copy(target.translation.z, source.translation.z)};
    target.rotation = {copy(target.rotation.x, source.rotation.x), copy(target.rotation.y, source.rotation.y),
        copy(target.rotation.z, source.rotation.z), copy(target.rotation.w, source.rotation.w)};
    target.scale = {copy(target.scale.x, source.scale.x), copy(target.scale.y, source.scale.y),
        copy(target.scale.z, source.scale.z)};
}

void GatherPoseLane(SoaTransform& target, int lane, const ozz::vector<SoaTransform>& source, u16 track)
{
    const auto& pose = source[track / 4];
    switch (track % 4)
    {
    case 0: CopyPoseLane<0>(target, lane, pose); break;
    case 1: CopyPoseLane<1>(target, lane, pose); break;
    case 2: CopyPoseLane<2>(target, lane, pose); break;
    case 3: CopyPoseLane<3>(target, lane, pose); break;
    }
}

SoaTransform SelectPose(SimdInt4 mask, const SoaTransform& a, const SoaTransform& b)
{
    SoaTransform result;
    result.translation = {Select(mask, a.translation.x, b.translation.x),
        Select(mask, a.translation.y, b.translation.y), Select(mask, a.translation.z, b.translation.z)};
    result.rotation = {Select(mask, a.rotation.x, b.rotation.x), Select(mask, a.rotation.y, b.rotation.y),
        Select(mask, a.rotation.z, b.rotation.z), Select(mask, a.rotation.w, b.rotation.w)};
    result.scale = SoaFloat3::one();
    return result;
}

SoaTransform Interpolate(const SoaTransform& a, const SoaTransform& b, SimdFloat4 factor)
{
    const auto one = simd_float4::one();
    const auto zero = simd_float4::zero();
    const auto dot = Dot(a.rotation, b.rotation);
    const auto cosine = Min(Abs(dot), one);
    const auto spherical = CmpGt(one - cosine, simd_float4::Load1(EPS));
    const auto squared = cosine * cosine;
    const auto omega = simd_float4::Load1(PI_DIV_2) - cosine *
        (simd_float4::Load1(0.892399f) + squared * (simd_float4::Load1(1.693204f) +
        squared * (simd_float4::Load1(-3.853735f) + squared * simd_float4::Load1(2.838933f))));
    const auto sine = Select(spherical, Sin(omega), one);
    const auto scale0 = Select(spherical, Sin(omega - factor * omega) / sine, one - factor);
    const auto scale1 = Select(spherical, Sin(factor * omega) / sine, factor) *
        Select(CmpLt(dot, zero), -one, one);
    SoaTransform result;
    result.rotation = a.rotation * scale0 + b.rotation * scale1;
    result.translation = a.translation + (b.translation - a.translation) * factor;
    result.scale = SoaFloat3::one();
    return result;
}

SoaQuaternion ScaleRotation(const SoaQuaternion& q, float factor)
{
    const auto zero = simd_float4::zero();
    const auto one = simd_float4::one();
    const auto sine = Sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
    const auto active = CmpGt(sine, simd_float4::Load1(EPS_S));
    const auto denominator = Select(active, sine, one);
    const auto acute = ATan(sine / Max(Abs(q.w), simd_float4::Load1(std::numeric_limits<float>::min())));
    const auto halfAngle = Select(CmpLt(q.w, zero), simd_float4::Load1(PI) - acute, acute);
    const auto angle = Select(active, halfAngle * simd_float4::Load1(factor), zero);
    const auto scale = Sin(angle) / denominator;
    return {q.x * scale, q.y * scale, q.z * scale, Cos(angle)};
}

Float4x4 ImportMatrix(const Fmatrix& matrix)
{
    return {{simd_float4::LoadPtrU(&matrix._11), simd_float4::LoadPtrU(&matrix._21),
        simd_float4::LoadPtrU(&matrix._31), simd_float4::LoadPtrU(&matrix._41)}};
}

void ExportMatrix(const Float4x4& matrix, Fmatrix& result)
{
    StorePtrU(matrix.cols[0], &result._11);
    StorePtrU(matrix.cols[1], &result._21);
    StorePtrU(matrix.cols[2], &result._31);
    StorePtrU(matrix.cols[3], &result._41);
}
}

struct OzzPose::State
{
    struct Blend
    {
        MotionID motion;
        float time = 0.f;
        float weight = 0.f;
        u8 channel = 0;
        u16 source = noBlend;
        MotionID sampledMotion;
        float sampledTime = std::numeric_limits<float>::quiet_NaN();
        ozz::animation::SamplingJob::Context context;
        ozz::vector<SoaTransform> locals;
    };
    struct Bone
    {
        u16 slots[boneBlendCapacity]{};
        u16 count = 0;
    };
    struct Packet
    {
        u64 revision = 0;
        u8 channels = 0;
    };

    std::shared_ptr<const OzzModelAnimations> assets;
    std::unique_ptr<Blend[]> blends;
    u16 blendCount = 0;
    xr_vector<Bone> bones;
    xr_vector<Packet> packets;
    ozz::vector<SoaTransform> locals;
    ozz::vector<SoaTransform> queryLocals;
    ozz::vector<SoaTransform> samplingLocals;
    ozz::vector<Float4x4> models;
    float factors[channelCount] = {1.f, 1.f, 1.f, 1.f};
    u64 revision = 1;

    const ozz::vector<SoaTransform>& Sample(u16 slot, float time)
    {
        Blend& blend = blends[slot];
        if (blend.source != noBlend)
        {
            const auto& source = blends[blend.source];
            if (source.sampledMotion == blend.motion && source.sampledTime == time)
                return source.locals;
        }
        for (u16 i = 0; i < blendCount; ++i)
        {
            if (blends[i].sampledMotion == blend.motion && blends[i].sampledTime == time)
            {
                blend.source = i;
                return blends[i].locals;
            }
        }
        const auto& library = *assets->libraries[blend.motion.slot];
        const auto& binding = assets->bindings[blend.motion.slot];
        const auto* animation = library.animations[blend.motion.idx].get();
        if (blend.sampledMotion != blend.motion)
            blend.context.Invalidate();
        const float duration = library.metadata.clips[blend.motion.idx].duration;
        const float wrapped = duration > 0.f ? std::fmod(std::max(time, 0.f), duration) : 0.f;
        ozz::animation::SamplingJob job;
        job.animation = animation;
        job.context = &blend.context;
        job.ratio = animation->duration() > 0.f ? wrapped / animation->duration() : 0.f;
        job.output = ozz::make_span(binding.jointToTrack.empty() ? blend.locals : samplingLocals);
        R_ASSERT2(job.Run(), "Ozz motion sampling failed");
        if (!binding.jointToTrack.empty())
        {
            for (size_t packet = 0; packet < blend.locals.size(); ++packet)
            {
                auto& pose = blend.locals[packet];
                pose = SoaTransform::identity();
                for (size_t lane = 0; lane < 4 && packet * 4 + lane < binding.jointToTrack.size(); ++lane)
                    GatherPoseLane(pose, int(lane), samplingLocals, binding.jointToTrack[packet * 4 + lane]);
            }
        }
        blend.sampledMotion = blend.motion;
        blend.sampledTime = time;
        blend.source = slot;
        return blend.locals;
    }

    SoaTransform Compose(size_t packet, u8 channels, u16 queryBone = noBlend,
        const OzzPoseOverride* controls = nullptr)
    {
        const auto zero = simd_float4::zero();
        const auto one = simd_float4::one();
        SoaTransform result = EmptyPose();
        for (u16 channel = 0; channel < channelCount; ++channel)
        {
            if (!(channels & (1 << channel)))
                continue;
            u16 ordered[4][boneBlendCapacity];
            u16 counts[4]{};
            u16 maximum = 0;
            int present[4]{};
            for (u16 lane = 0; lane < 4; ++lane)
            {
                const size_t joint = packet * 4 + lane;
                if (joint >= assets->skeleton->jointToBone.size())
                    continue;
                const u16 boneId = assets->skeleton->jointToBone[joint];
                const auto& bone = bones[boneId];
                for (u16 i = 0; i < bone.count; ++i)
                {
                    const u16 slot = bone.slots[i];
                    if (blends[slot].channel != channel ||
                        (controls && boneId == queryBone && controls->isolated && slot != controls->blend))
                        continue;
                    ordered[lane][counts[lane]++] = slot;
                }
                if (counts[lane] > 2)
                    std::sort(ordered[lane], ordered[lane] + counts[lane], [&](u16 a, u16 b)
                        { return blends[a].weight > blends[b].weight; });
                maximum = std::max(maximum, counts[lane]);
                present[lane] = counts[lane] ? -1 : 0;
            }
            if (!maximum)
                continue;
            SoaTransform mixed = EmptyPose();
            float totals[4]{};
            for (u16 index = 0; index < maximum; ++index)
            {
                SoaTransform input = EmptyPose();
                float fractions[4]{};
                int valid[4]{};
                for (u16 lane = 0; lane < 4; ++lane)
                {
                    if (index >= counts[lane])
                        continue;
                    const u16 slot = ordered[lane][index];
                    const auto& blend = blends[slot];
                    const u16 boneId = assets->skeleton->jointToBone[packet * 4 + lane];
                    const bool overridden = controls && boneId == queryBone && controls->blend == slot;
                    auto value = Sample(slot, overridden && controls->time ? *controls->time : blend.time)[packet];
                    if (channel >= 2)
                    {
                        const auto& binding = assets->bindings[blend.motion.slot];
                        const auto& frames = assets->libraries[blend.motion.slot]->firstFrame[blend.motion.idx];
                        auto first = SoaTransform::identity();
                        if (binding.jointToTrack.empty())
                            first = frames[packet];
                        else
                            GatherPoseLane(first, lane, frames, binding.jointToTrack[packet * 4 + lane]);
                        value.rotation = value.rotation * Conjugate(first.rotation);
                        value.translation = value.translation - first.translation;
                    }
                    if (overridden && controls->rotation)
                    {
                        const auto& q = *controls->rotation;
                        value.rotation = {simd_float4::Load1(q.x), simd_float4::Load1(q.y),
                            simd_float4::Load1(q.z), simd_float4::Load1(q.w)};
                    }
                    if (overridden && controls->translation)
                    {
                        const auto& t = *controls->translation;
                        value.translation = {simd_float4::Load1(t.x), simd_float4::Load1(t.y), simd_float4::Load1(t.z)};
                    }
                    const auto laneMask = SetI(simd_int4::zero(), simd_int4::Load1(-1), lane);
                    input = SelectPose(laneMask, value, input);
                    totals[lane] += blend.weight;
                    fractions[lane] = fis_zero(totals[lane]) ? 0.f :
                        std::clamp(blend.weight / totals[lane], 0.f, 1.f);
                    valid[lane] = -1;
                }
                const auto validMask = simd_int4::LoadPtrU(valid);
                const auto next = index ? Interpolate(mixed, input, simd_float4::LoadPtrU(fractions)) : input;
                mixed = SelectPose(validMask, next, mixed);
            }
            const auto presentMask = simd_int4::LoadPtrU(present);
            if (channel < 2)
            {
                if (channel == 1)
                    R_ASSERT2(factors[channel] != 0.f, "Zero override-channel factor");
                result = SelectPose(presentMask, mixed, result);
            }
            else
            {
                SoaTransform added = result;
                added.rotation = ScaleRotation(mixed.rotation, factors[channel]) * result.rotation;
                added.translation = result.translation + mixed.translation * simd_float4::Load1(factors[channel]);
                result = SelectPose(presentMask, added, result);
            }
        }
        result.rotation.w = Select(CmpEq(Dot(result.rotation, result.rotation), zero), one, result.rotation.w);
        result.rotation = Conjugate(result.rotation);
        return result;
    }

    void Model(Fmatrix& result, u16 bone, const Fmatrix& parent, ozz::span<const SoaTransform> input)
    {
        const u16 joint = assets->skeleton->boneToJoint[bone];
        const auto parentMatrix = ImportMatrix(parent);
        const auto parentJoint = assets->skeleton->skeleton.joint_parents()[joint];
        if (parentJoint != ozz::animation::Skeleton::kNoParent)
            models[parentJoint] = parentMatrix;
        ozz::animation::LocalToModelJob job;
        job.skeleton = &assets->skeleton->skeleton;
        job.input = input;
        job.output = ozz::make_span(models);
        job.from = joint;
        job.to = joint;
        job.root = &parentMatrix;
        R_ASSERT2(job.Run(), "Ozz hierarchy evaluation failed");
        ExportMatrix(models[joint], result);
    }
};

OzzPose::OzzPose() = default;
OzzPose::~OzzPose() = default;

void OzzPose::Reset(std::shared_ptr<const OzzModelAnimations> assets, u16 blendCapacity)
{
    state = std::make_unique<State>();
    state->assets = std::move(assets);
    const auto& skeleton = state->assets->skeleton->skeleton;
    state->blendCount = blendCapacity;
    state->blends = std::make_unique<State::Blend[]>(blendCapacity);
    state->bones.resize(skeleton.num_joints());
    state->packets.resize(skeleton.num_soa_joints());
    state->locals.resize(skeleton.num_soa_joints(), SoaTransform::identity());
    state->queryLocals.resize(skeleton.num_soa_joints(), SoaTransform::identity());
    state->models.resize(skeleton.num_joints());
    for (const auto& binding : state->assets->bindings)
        if (!binding.jointToTrack.empty())
        {
            state->samplingLocals.resize(skeleton.num_soa_joints());
            break;
        }
}

void OzzPose::ReserveBlend(u16 slot)
{
    R_ASSERT(slot < state->blendCount);
    auto& blend = state->blends[slot];
    if (blend.context.max_tracks())
        return;
    const auto& skeleton = state->assets->skeleton->skeleton;
    blend.context.Resize(skeleton.num_joints());
    blend.locals.resize(skeleton.num_soa_joints());
}

bool OzzPose::SetBlend(u16 slot, MotionID motion, float time, float weight, u8 channel)
{
    auto& blend = state->blends[slot];
    if (blend.motion == motion && blend.time == time && blend.weight == weight && blend.channel == channel)
        return false;
    blend.motion = motion;
    blend.time = time;
    blend.weight = weight;
    blend.channel = channel;
    ++state->revision;
    return true;
}

bool OzzPose::SetBoneBlends(u16 bone, ozz::span<const u16> slots)
{
    R_ASSERT(slots.size() <= boneBlendCapacity);
    auto& target = state->bones[bone];
    if (slots.size() == target.count && std::equal(slots.begin(), slots.end(), target.slots))
        return false;
    target.count = u16(slots.size());
    std::copy(slots.begin(), slots.end(), target.slots);
    ++state->revision;
    return true;
}

bool OzzPose::SetChannelFactor(u16 channel, float factor)
{
    R_ASSERT(channel < channelCount);
    if (state->factors[channel] == factor)
        return false;
    state->factors[channel] = factor;
    ++state->revision;
    return true;
}

void OzzPose::BuildBone(Fmatrix& result, u16 bone, const Fmatrix& parent, u8 channels)
{
    const size_t packet = state->assets->skeleton->boneToJoint[bone] / 4;
    auto& stamp = state->packets[packet];
    if (stamp.revision != state->revision || stamp.channels != channels)
    {
        state->locals[packet] = state->Compose(packet, channels);
        stamp = {state->revision, channels};
    }
    state->Model(result, bone, parent, ozz::make_span(state->locals));
}

void OzzPose::QueryBone(Fmatrix& result, u16 bone, const Fmatrix& parent, u8 channels,
    const OzzPoseOverride& controls)
{
    const size_t packet = state->assets->skeleton->boneToJoint[bone] / 4;
    state->queryLocals[packet] = state->Compose(packet, channels, bone, &controls);
    state->Model(result, bone, parent, ozz::make_span(state->queryLocals));
}
}
