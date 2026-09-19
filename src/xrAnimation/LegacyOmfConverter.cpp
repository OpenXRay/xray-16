#include "stdafx.h"
#include "LegacyOmfConverter.h"
#include "LegacyChunkIO.h"
#include "OzzSkeletonMirror.h"

#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/base/maths/quaternion.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <unordered_map>

namespace XRay::Animation
{
namespace
{
using RawAnimation = ozz::animation::offline::RawAnimation;

float ReadFinite(BinaryReader& reader)
{
    const float value = reader.read<float>();
    if (!std::isfinite(value))
        throw std::runtime_error("non-finite animation value");
    return value;
}

Fvector ReadTranslation(BinaryReader& reader)
{
    Fvector value;
    value.x = reader.read<float>();
    value.y = reader.read<float>();
    value.z = reader.read<float>();
    return value;
}

Fquaternion ReadRotation(BinaryReader& reader)
{
    Fquaternion value;
    value.x = float(reader.read<s16>()) * KEY_QuantI;
    value.y = float(reader.read<s16>()) * KEY_QuantI;
    value.z = float(reader.read<s16>()) * KEY_QuantI;
    value.w = float(reader.read<s16>()) * KEY_QuantI;
    const float norm = value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w;
    if (!(norm > 0.f))
        throw std::runtime_error("zero animation quaternion");
    value.normalize();
    return value;
}

ozz::math::Quaternion ToOzz(const Fquaternion& value)
{
    return ozz::math::Quaternion(value.x, value.y, value.z, value.w);
}

unsigned RotationSegments(const Fquaternion& a, const Fquaternion& b)
{
    const double dot = std::min(1.0, std::abs(double(a.x) * b.x + double(a.y) * b.y +
        double(a.z) * b.z + double(a.w) * b.w));
    const double angle = std::acos(dot);
    unsigned segments = 1;
    while (segments < 16)
    {
        const double theta = angle / segments;
        if (theta < 1.e-4)
            break;
        const double sine = std::sin(theta);
        const double cosine = std::cos(theta);
        const double t = (1.0 - std::sqrt(std::max(0.0,
            1.0 - 2.0 * (1.0 - sine / theta) / (1.0 - cosine)))) * 0.5;
        const double error = 2.0 * std::abs(std::atan2(t * sine, 1.0 - t + t * cosine) - t * theta);
        if (error <= 0.0002)
            break;
        segments *= 2;
    }
    return segments;
}

void ReadTrack(BinaryReader& reader, u32 frames, RawAnimation::JointTrack& track)
{
    const u8 flags = reader.read<u8>();
    if (flags & ~(flTKeyPresent | flRKeyAbsent | flTKey16IsBit))
        throw std::runtime_error("unsupported animation track flags");
    const u32 rotationCount = flags & flRKeyAbsent ? 1 : frames;
    if (!(flags & flRKeyAbsent))
        reader.read<u32>();
    if (rotationCount > (reader.size - reader.offset) / sizeof(CKeyQR))
        throw std::runtime_error("truncated rotation track");
    xr_vector<Fquaternion> rotations(rotationCount);
    for (auto& value : rotations)
        value = ReadRotation(reader);
    const auto& first = rotations.front();
    const bool constant = std::all_of(rotations.begin(), rotations.end(), [&](const Fquaternion& value)
    {
        return (value.x == first.x && value.y == first.y && value.z == first.z && value.w == first.w) ||
            (value.x == -first.x && value.y == -first.y && value.z == -first.z && value.w == -first.w);
    });
    track.rotations.push_back({0.f, ToOzz(first)});
    if (!constant)
    {
        for (u32 frame = 0; frame < frames; ++frame)
        {
            const auto& a = rotations[frame];
            const auto& b = rotations[(frame + 1) % frames];
            const unsigned segments = RotationSegments(a, b);
            for (unsigned part = 1; part <= segments; ++part)
            {
                const float fraction = float(part) / float(segments);
                Fquaternion value;
                value.slerp(a, b, fraction);
                value.normalize();
                track.rotations.push_back({(float(frame) + fraction) * SAMPLE_SPF, ToOzz(value)});
            }
        }
    }

    if (flags & flTKeyPresent)
    {
        reader.read<u32>();
        const size_t keySize = flags & flTKey16IsBit ? sizeof(CKeyQT16) : sizeof(CKeyQT8);
        if (frames > (reader.size - reader.offset) / keySize)
            throw std::runtime_error("truncated translation track");
        const auto* keys = reader.data + reader.offset;
        reader.skip(size_t(frames) * keySize);
        const Fvector scale = ReadTranslation(reader);
        const Fvector offset = ReadTranslation(reader);
        BinaryReader packed{keys, size_t(frames) * keySize};
        track.translations.reserve(size_t(frames) + 1);
        for (u32 frame = 0; frame < frames; ++frame)
        {
            const float x = flags & flTKey16IsBit ? float(packed.read<s16>()) : float(packed.read<s8>());
            const float y = flags & flTKey16IsBit ? float(packed.read<s16>()) : float(packed.read<s8>());
            const float z = flags & flTKey16IsBit ? float(packed.read<s16>()) : float(packed.read<s8>());
            const ozz::math::Float3 value(x * scale.x + offset.x, y * scale.y + offset.y, z * scale.z + offset.z);
            track.translations.push_back({float(frame) * SAMPLE_SPF, value});
        }
        const auto initial = track.translations.front().value;
        const bool fixed = std::all_of(track.translations.begin(), track.translations.end(), [&](const auto& key)
        { return key.value.x == initial.x && key.value.y == initial.y && key.value.z == initial.z; });
        if (fixed)
            track.translations.resize(1);
        else
            track.translations.push_back({float(frames) * SAMPLE_SPF, initial});
    }
    else
    {
        const Fvector value = ReadTranslation(reader);
        track.translations.push_back({0.f, ozz::math::Float3(value.x, value.y, value.z)});
    }
    track.scales.push_back({0.f, ozz::math::Float3::one()});
}

xr_vector<u16> ReadMetadata(const Chunk& chunk, const OzzSkeletonMirror& skeleton, MotionLibraryMetadata& metadata)
{
    BinaryReader reader{chunk.data, chunk.size};
    const u16 version = reader.read<u16>();
    if (version > xrOGF_SMParamsVersion)
        throw std::runtime_error("unsupported OMF metadata version");
    const u16 partCount = reader.read<u16>();
    if (partCount > MAX_PARTS)
        throw std::runtime_error("too many OMF partitions");
    const size_t bones = skeleton.boneToJoint.size();
    xr_vector<u16> remap(bones, BI_NONE);
    xr_vector<bool> assigned(bones, false);
    std::unordered_map<std::string, u16> boneNames;
    const auto names = skeleton.skeleton.joint_names();
    for (u16 bone = 0; bone < bones; ++bone)
        boneNames.emplace(ToLowerCopy(names[skeleton.boneToJoint[bone]]), bone);
    for (u16 part = 0; part < partCount; ++part)
    {
        auto& target = metadata.partition[part];
        target.Name = ToLowerCopy(reader.read_stringz()).c_str();
        const u16 count = reader.read<u16>();
        if (count > bones)
            throw std::runtime_error("invalid OMF partition size");
        for (u16 index = 0; index < count; ++index)
        {
            const auto name = ToLowerCopy(reader.read_stringz());
            const u32 sourceBone = reader.read<u32>();
            const auto found = boneNames.find(name);
            if (sourceBone >= bones || remap[sourceBone] != BI_NONE || found == boneNames.end() || assigned[found->second])
                throw std::runtime_error("invalid or duplicate OMF bone mapping: " + name);
            target.bones.push_back(found->second);
            remap[sourceBone] = found->second;
            assigned[found->second] = true;
        }
    }
    if (std::find(remap.begin(), remap.end(), BI_NONE) != remap.end())
        throw std::runtime_error("OMF partitions do not cover the skeleton");
    const u16 count = reader.read<u16>();
    if (!count || count >= 0x3fff)
        throw std::runtime_error("invalid OMF motion count");
    metadata.clips.resize(count);
    for (u16 index = 0; index < count; ++index)
    {
        auto& clip = metadata.clips[index];
        clip.name = ToLowerCopy(reader.read_stringz()).c_str();
        auto& definition = clip.definition;
        const u32 flags = reader.read<u32>();
        definition.flags = u16(flags);
        definition.bone_or_part = reader.read<u16>();
        definition.motion = reader.read<u16>();
        if (definition.motion >= count)
            throw std::runtime_error("OMF definition references invalid motion");
        if (flags & esmFX)
        {
            if (definition.bone_or_part != BI_NONE)
            {
                if (definition.bone_or_part >= bones)
                    throw std::runtime_error("OMF effect references invalid bone");
                definition.bone_or_part = remap[definition.bone_or_part];
            }
        }
        else if (definition.bone_or_part != BI_NONE && definition.bone_or_part >= partCount)
            throw std::runtime_error("OMF cycle references invalid partition");
        definition.speed = definition.Quantize(std::clamp(ReadFinite(reader), 0.f, 100.f));
        definition.power = definition.Quantize(std::clamp(ReadFinite(reader), 0.f, 100.f));
        definition.accrue = definition.Quantize(std::clamp(ReadFinite(reader), 0.f, 100.f));
        definition.falloff = definition.Quantize(std::clamp(ReadFinite(reader), 0.f, 100.f));
        if (!(flags & esmFX) && definition.falloff >= definition.accrue)
            definition.falloff = u16(definition.accrue - 1);
        if (version >= 4)
        {
            const u32 marks = reader.read<u32>();
            if (marks > (reader.size - reader.offset) / 5)
                throw std::runtime_error("invalid OMF marks count");
            definition.marks.resize(marks);
            for (auto& mark : definition.marks)
            {
                mark.name = ReadStringCRLF(reader).c_str();
                const u32 intervals = reader.read<u32>();
                if (intervals > (reader.size - reader.offset) / (2 * sizeof(float)))
                    throw std::runtime_error("truncated OMF marks");
                xr_vector<motion_marks::interval> values;
                values.reserve(intervals);
                for (u32 item = 0; item < intervals; ++item)
                {
                    const float begin = ReadFinite(reader);
                    const float end = ReadFinite(reader);
                    if (end < begin)
                        throw std::runtime_error("reversed OMF mark interval");
                    values.emplace_back(begin, end);
                }
                mark.SetIntervals(std::move(values));
            }
        }
        if (!metadata.motions.emplace(clip.name, index).second)
            throw std::runtime_error("duplicate OMF motion name");
        (flags & esmFX ? metadata.effects : metadata.cycles).emplace(clip.name, index);
    }
    if (reader.offset != reader.size)
        throw std::runtime_error("unexpected OMF metadata trailing bytes");
    return remap;
}
}

ConvertedOmfLibrary ConvertLegacyOmf(const std::byte* data, size_t size, pcstr source, const OzzSkeletonMirror& skeleton)
{
    try
    {
        if (!data || !size || !skeleton.skeleton.num_joints())
            throw std::runtime_error("empty OMF or skeleton");
        ChunkStorage storage;
        const auto chunks = ParseChunks(data, size, storage);
        const auto params = chunks.find(OGF_S_SMPARAMS);
        const auto motions = chunks.find(OGF_S_MOTIONS);
        if (params == chunks.end() || motions == chunks.end())
            throw std::runtime_error("missing OMF metadata or motions");
        ConvertedOmfLibrary output;
        output.metadata.source = source;
        const auto remap = ReadMetadata(params->second, skeleton, output.metadata);
        const auto clips = ParseChunks(motions->second.data, motions->second.size, storage);
        const auto countChunk = clips.find(0);
        if (countChunk == clips.end())
            throw std::runtime_error("missing OMF motion count");
        BinaryReader countReader{countChunk->second.data, countChunk->second.size};
        const u32 count = countReader.read<u32>();
        if (count != output.metadata.clips.size() || clips.size() != size_t(count) + 1)
            throw std::runtime_error("OMF metadata/clip count mismatch");
        output.animations.reserve(count);
        for (u32 index = 0; index < count; ++index)
        {
            const auto found = clips.find(index + 1);
            if (found == clips.end())
                throw std::runtime_error("missing OMF clip");
            BinaryReader reader{found->second.data, found->second.size};
            const auto name = ToLowerCopy(reader.read_stringz());
            if (name != output.metadata.clips[index].name.c_str())
                throw std::runtime_error("OMF metadata/clip name mismatch: " + name);
            const u32 frames = reader.read<u32>();
            if (!frames || frames > 0x00ffffff)
                throw std::runtime_error("unsupported OMF frame count: " + name);
            RawAnimation raw;
            raw.name = name.c_str();
            raw.duration = float(frames) * SAMPLE_SPF;
            raw.tracks.resize(remap.size());
            for (u16 bone : remap)
                ReadTrack(reader, frames, raw.tracks[skeleton.boneToJoint[bone]]);
            if (reader.offset != reader.size || !raw.Validate())
                throw std::runtime_error("invalid OMF clip: " + name);
            std::set<float> times{0.f, raw.duration};
            for (const auto& track : raw.tracks)
            {
                for (const auto& key : track.rotations)
                    times.insert(key.time);
                for (const auto& key : track.translations)
                    times.insert(key.time);
                if (times.size() >= 65535)
                    throw std::runtime_error("Ozz timepoint limit exceeded: " + name);
            }
            const ozz::animation::offline::AnimationBuilder builder;
            auto animation = builder(raw);
            if (!animation)
                throw std::runtime_error("Ozz animation build failed: " + name);
            output.metadata.clips[index].duration = raw.duration;
            output.animations.push_back(std::move(animation));
        }
        return output;
    }
    catch (const std::exception& error)
    {
        throw std::runtime_error(std::string(source) + ": " + error.what());
    }
}
}
