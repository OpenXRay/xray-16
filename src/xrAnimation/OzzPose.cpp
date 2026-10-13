#include "stdafx.h"
#include "OzzPose.h"
#include "OzzMath.h"
#include "OzzSkeletonMirror.h"

#include "xrCore/Profiler/Profiler.h"
#include "xrCore/_quaternion.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/math_constant.h>
#include <ozz/base/maths/soa_float4x4.h>
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

void ComputeLocalMatrices(const SoaTransform& pose, Float4x4 (&result)[4])
{
    const auto rotation = Normalize(pose.rotation);
    const auto matrices = SoaFloat4x4::FromAffine(pose.translation, rotation, pose.scale);
    Transpose16x16(&matrices.cols[0].x, result->cols);
}

SoaTransform BroadcastPose(SimdFloat4 translation, SimdFloat4 rotation)
{
    SoaTransform value;
    value.translation = {SplatX(translation), SplatY(translation), SplatZ(translation)};
    value.rotation = {SplatX(rotation), SplatY(rotation), SplatZ(rotation), SplatW(rotation)};
    value.scale = SoaFloat3::one();
    return value;
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
    };
    struct Sample
    {
        u16 source = noBlend;
        MotionID sampledMotion;
        float sampledTime = std::numeric_limits<float>::quiet_NaN();
        ozz::animation::SamplingJob::Context context;
        ozz::vector<SoaTransform> locals;
    };
    struct SamplingBank
    {
        xr_vector<std::unique_ptr<Sample>> samples;
        ozz::vector<SoaTransform> remapScratch;

        void ReserveBlend(u16 slot, const ozz::animation::Skeleton& skeleton)
        {
            auto& sample = samples[slot];
            if (sample)
                return;
            sample = std::make_unique<Sample>();
            sample->context.Resize(skeleton.num_joints());
            sample->locals.resize(skeleton.num_soa_joints());
        }
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
        bool includeBasePose = false;
        Fmatrix matrices[4];
    };

    std::shared_ptr<const OzzModelAnimations> assets;
    std::unique_ptr<Blend[]> blends;
    u16 blendCount = 0;
    xr_vector<Bone> bones;
    xr_vector<Packet> packets;
    SamplingBank normal;
    SamplingBank query;
    xr_vector<SoaTransform, ozz::StdAllocator<SoaTransform>> basePose;
    xr_vector<SimdInt4, ozz::StdAllocator<SimdInt4>> basePoseMask;
    float basePoseWeight = 0.f;
    bool basePoseActive = false;
    float factors[channelCount] = {1.f, 1.f, 1.f, 1.f};
    u64 revision = 1;

    const ozz::vector<SoaTransform>& SampleBlend(SamplingBank& bank, u16 slot, float time) const
    {
        const Blend& blend = blends[slot];
        auto& sample = *bank.samples[slot];
        if (sample.source != noBlend)
        {
            const auto& source = *bank.samples[sample.source];
            if (source.sampledMotion == blend.motion && source.sampledTime == time)
                return source.locals;
        }
        for (u16 i = 0; i < blendCount; ++i)
        {
            const auto& source = bank.samples[i];
            if (source && source->sampledMotion == blend.motion && source->sampledTime == time)
            {
                sample.source = i;
                return source->locals;
            }
        }
        const auto& library = *assets->libraries[blend.motion.slot];
        const auto& binding = assets->bindings[blend.motion.slot];
        const auto* animation = library.animations[blend.motion.idx].get();
        if (sample.sampledMotion != blend.motion)
            sample.context.Invalidate();
        const float duration = library.metadata.clips[blend.motion.idx].duration;
        const float wrapped = duration > 0.f ? std::fmod(std::max(time, 0.f), duration) : 0.f;
        ozz::animation::SamplingJob job;
        job.animation = animation;
        job.context = &sample.context;
        job.ratio = animation->duration() > 0.f ? wrapped / animation->duration() : 0.f;
        job.output = ozz::make_span(binding.jointToTrack.empty() ? sample.locals : bank.remapScratch);
        {
            ZoneScopedN("Animation::Sampling");
            R_ASSERT2(job.Run(), "Ozz motion sampling failed");
        }
        if (!binding.jointToTrack.empty())
        {
            ZoneScopedN("Animation::Remapping");
            for (size_t packet = 0; packet < sample.locals.size(); ++packet)
            {
                auto& pose = sample.locals[packet];
                pose = SoaTransform::identity();
                for (size_t lane = 0; lane < 4 && packet * 4 + lane < binding.jointToTrack.size(); ++lane)
                    GatherPoseLane(pose, int(lane), bank.remapScratch, binding.jointToTrack[packet * 4 + lane]);
            }
        }
        sample.sampledMotion = blend.motion;
        sample.sampledTime = time;
        sample.source = slot;
        return sample.locals;
    }

    SoaTransform Compose(SamplingBank& bank, size_t packet, u8 channels, bool includeBasePose,
        u16 queryBone = noBlend, const OzzPoseOverride* controls = nullptr) const
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
                    auto value = SampleBlend(bank, slot, overridden && controls->time ? *controls->time : blend.time)[packet];
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
        if (includeBasePose && basePoseActive && basePoseWeight > 0.f)
        {
            SoaTransform captured = basePose[packet];
            if (basePoseWeight < 1.f)
            {
                SoaTransform animated = result;
                animated.rotation = Normalize(animated.rotation);
                captured = Interpolate(animated, captured, simd_float4::Load1(basePoseWeight));
            }
            result = SelectPose(basePoseMask[packet], captured, result);
        }
        result.rotation = Conjugate(result.rotation);
        return result;
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
    state->normal.samples.resize(blendCapacity);
    state->query.samples.resize(blendCapacity);
    for (const auto& binding : state->assets->bindings)
        if (!binding.jointToTrack.empty())
        {
            state->normal.remapScratch.resize(skeleton.num_soa_joints());
            state->query.remapScratch.resize(skeleton.num_soa_joints());
            break;
        }
}

void OzzPose::ReserveBlend(u16 slot)
{
    R_ASSERT(slot < state->blendCount);
    const auto& skeleton = state->assets->skeleton->skeleton;
    state->normal.ReserveBlend(slot, skeleton);
    state->query.ReserveBlend(slot, skeleton);
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

bool OzzPose::SetBasePose(ozz::span<const Fmatrix> modelPose, float weight,
    ozz::span<const u8> visibleBones, PoseCaptureResult* result)
{
    if (result)
        *result = {};
    const auto reject = [result](pcstr reason, u16 bone, float value, float limit)
    {
        if (result)
            *result = {reason, bone, value, limit};
        return false;
    };
    if (!state || !state->assets || !state->assets->skeleton)
        return reject("missing-skeleton", BI_NONE, 0.f, 0.f);
    if (!std::isfinite(weight))
        return reject("invalid-weight", BI_NONE, weight, 1.f);

    const auto& mirror = *state->assets->skeleton;
    if (modelPose.size() != mirror.boneToJoint.size())
        return reject("bone-count", BI_NONE, float(modelPose.size()), float(mirror.boneToJoint.size()));
    if (!visibleBones.empty() && visibleBones.size() != modelPose.size())
        return reject("visibility-count", BI_NONE, float(visibleBones.size()), float(modelPose.size()));

    size_t visibleCount = 0;
    for (size_t bone = 0; bone < modelPose.size(); ++bone)
    {
        if (!visibleBones.empty() && !visibleBones[bone])
            continue;
        ++visibleCount;
        const Fmatrix& matrix = modelPose[bone];
        if (!_valid(matrix))
            return reject("non-finite-model-transform", u16(bone), 0.f, 0.f);
        const float affineError = std::max({_abs(matrix._14), _abs(matrix._24), _abs(matrix._34),
            _abs(matrix._44 - 1.f)});
        if (affineError > EPS)
            return reject("non-affine-model-transform", u16(bone), affineError, EPS);
        const Float4x4 transform = ImportOzzMatrix(matrix);
        if (!AreAllTrue3(IsNormalizedEst(transform)))
        {
            const float lengthError = std::max({_abs(matrix.i.square_magnitude() - 1.f),
                _abs(matrix.j.square_magnitude() - 1.f), _abs(matrix.k.square_magnitude() - 1.f)});
            return reject("non-unit-model-basis", u16(bone), lengthError, kNormalizationToleranceEstSq);
        }
        const float orthogonalityError = std::max({_abs(matrix.i.dotproduct(matrix.j)),
            _abs(matrix.i.dotproduct(matrix.k)), _abs(matrix.j.dotproduct(matrix.k))});
        if (orthogonalityError > kNormalizationToleranceEstSq)
            return reject("non-orthogonal-model-basis", u16(bone), orthogonalityError, kNormalizationToleranceEstSq);
        Fvector normal;
        normal.crossproduct(matrix.i, matrix.j);
        const float handedness = normal.dotproduct(matrix.k);
        if (handedness <= 0.f)
            return reject("reflected-model-basis", u16(bone), handedness, 0.f);
    }
    if (!visibleCount)
        return reject("no-visible-bones", BI_NONE, 0.f, 1.f);

    const auto jointParents = mirror.skeleton.joint_parents();
    xr_vector<SoaTransform, ozz::StdAllocator<SoaTransform>> capturedPose(size_t(mirror.skeleton.num_soa_joints()));
    xr_vector<SimdInt4, ozz::StdAllocator<SimdInt4>> capturedMask(capturedPose.size());
    for (size_t packet = 0; packet < capturedPose.size(); ++packet)
    {
        SoaTransform value = SoaTransform::identity();
        s32 mask[4]{};
        for (size_t lane = 0; lane < 4; ++lane)
        {
            const size_t joint = packet * 4 + lane;
            if (joint >= mirror.jointToBone.size())
                break;
            const u16 bone = mirror.jointToBone[joint];
            if (bone >= modelPose.size())
                return reject("bone-map", bone, float(bone), float(modelPose.size()));
            if (!visibleBones.empty() && !visibleBones[bone])
                continue;
            Fmatrix local = modelPose[bone];
            const auto parentJoint = jointParents[joint];
            if (parentJoint >= 0)
            {
                if (size_t(parentJoint) >= mirror.jointToBone.size())
                    return reject("parent-map", bone, float(parentJoint), float(mirror.jointToBone.size()));
                const u16 parentBone = mirror.jointToBone[size_t(parentJoint)];
                if (parentBone >= modelPose.size())
                    return reject("parent-bone-map", bone, float(parentBone), float(modelPose.size()));
                if (visibleBones.empty() || visibleBones[parentBone])
                {
                    Fmatrix inverseParent;
                    if (!inverseParent.invert_b(modelPose[parentBone]))
                        return reject("singular-parent-transform", parentBone, 0.f, 0.f);
                    local.mul_43(inverseParent, modelPose[bone]);
                }
            }
            if (!_valid(local))
                return reject("non-finite-local-transform", bone, 0.f, 0.f);
            Fquaternion rotation;
            rotation.set(local);
            const float lengthSquared = rotation.magnitude();
            if (!std::isfinite(lengthSquared) || lengthSquared <= EPS_S * EPS_S)
                return reject("invalid-local-quaternion", bone, lengthSquared, EPS_S * EPS_S);
            rotation.normalize();
            const auto translation = simd_float4::Load3PtrU(&local.c.x);
            const auto quaternion = simd_float4::LoadPtrU(&rotation.x);
            CopyPoseLane<0>(value, int(lane), BroadcastPose(translation, quaternion));
            mask[lane] = -1;
        }
        capturedPose[packet] = value;
        capturedMask[packet] = simd_int4::Load(mask[0], mask[1], mask[2], mask[3]);
    }
    state->basePose.swap(capturedPose);
    state->basePoseMask.swap(capturedMask);
    state->basePoseWeight = std::clamp(weight, 0.f, 1.f);
    state->basePoseActive = true;
    ++state->revision;
    return true;
}

bool OzzPose::SetBasePoseWeight(float weight)
{
    if (!HasBasePose() || !std::isfinite(weight))
        return false;
    weight = std::clamp(weight, 0.f, 1.f);
    if (state->basePoseWeight != weight)
    {
        state->basePoseWeight = weight;
        ++state->revision;
    }
    return true;
}

void OzzPose::ClearBasePose()
{
    if (!state || !state->basePoseActive)
        return;
    state->basePoseActive = false;
    state->basePoseWeight = 0.f;
    ++state->revision;
}

bool OzzPose::HasBasePose() const
{
    return state && state->basePoseActive;
}

float OzzPose::BasePoseWeight() const
{
    return state ? state->basePoseWeight : 0.f;
}

const Fmatrix& OzzPose::EvaluateLocalBone(u16 bone, u8 channels, bool includeBasePose)
{
    includeBasePose = includeBasePose && state->basePoseActive && state->basePoseWeight > 0.f;
    const u16 joint = state->assets->skeleton->boneToJoint[bone];
    auto& packet = state->packets[joint / 4];
    if (packet.revision != state->revision || packet.channels != channels ||
        packet.includeBasePose != includeBasePose)
    {
        ZoneScopedN("Animation::LocalPose");
        Float4x4 matrices[4];
        ComputeLocalMatrices(state->Compose(state->normal, joint / 4, channels, includeBasePose), matrices);
        for (size_t lane = 0; lane < 4; ++lane)
            ExportOzzMatrix(matrices[lane], packet.matrices[lane]);
        packet.revision = state->revision;
        packet.channels = channels;
        packet.includeBasePose = includeBasePose;
    }
    return packet.matrices[joint % 4];
}

void OzzPose::QueryBone(Fmatrix& result, u16 bone, const Fmatrix& parent, u8 channels,
    const OzzPoseOverride& controls)
{
    const u16 joint = state->assets->skeleton->boneToJoint[bone];
    Float4x4 matrices[4];
    ComputeLocalMatrices(state->Compose(state->query, joint / 4, channels, false, bone, &controls), matrices);
    ExportOzzMatrix(ImportOzzMatrix(parent) * matrices[joint % 4], result);
}
}
