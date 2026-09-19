#include "stdafx.h"

#include "LegacyOmfConverter.h"

#include "LegacyChunkIO.h"
#include "OzzConversion.h"

#include "xrCore/Animation/SkeletonMotionDefs.hpp"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include "xrCore/Threading/ParallelFor.hpp"

#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>

#include <ozz/base/io/archive.h>
#include <ozz/base/maths/math_ex.h>
#include <ozz/base/maths/quaternion.h>
#include <ozz/base/maths/soa_transform.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace XRay
{
namespace Animation
{
namespace
{
void ParseSmparams(const Chunk& chunk, const xr_vector<xr_string>& skeleton_bone_names, LegacyOmfData& output)
{
    BinaryReader reader{ chunk.data, chunk.size };

    const u16 version = reader.read<u16>();
    if (version > xrOGF_SMParamsVersion)
        throw std::runtime_error("unsupported OMF params version");

    const u16 part_count = reader.read<u16>();

    output.bone_remap.assign(skeleton_bone_names.size(), std::numeric_limits<u16>::max());
    output.remap_bone_names.assign(skeleton_bone_names.size(), xr_string());

    xr_vector<xr_string> lower_names;
    lower_names.reserve(skeleton_bone_names.size());
    for (const xr_string& name : skeleton_bone_names)
        lower_names.emplace_back(ToLowerCopy(std::string(name.c_str())));

    std::unordered_map<std::string, u16> skeleton_index_by_name;
    for (u16 idx = 0; idx < lower_names.size(); ++idx)
        skeleton_index_by_name.emplace(lower_names[idx], idx);

    u32 bones_mapped = 0;

    for (u16 part = 0; part < part_count; ++part)
    {
        reader.read_stringz();
        const u16 bone_count = reader.read<u16>();
        for (u16 bone_idx = 0; bone_idx < bone_count; ++bone_idx)
        {
            const xr_string bone_name_raw = reader.read_stringz().c_str();
            const u32 remap_index = reader.read<u32>();
            if (remap_index >= output.bone_remap.size())
                throw std::runtime_error("bone remap index out of range in OMF params");
            const std::string lowered = ToLowerCopy(std::string(bone_name_raw.c_str()));
            const auto it = skeleton_index_by_name.find(lowered);
            if (it == skeleton_index_by_name.end())
            {
                std::string message = "bone ";
                message += bone_name_raw.c_str();
                message += " referenced in OMF params not found in skeleton";
                throw std::runtime_error(message);
            }
            output.bone_remap[remap_index] = it->second;
            output.remap_bone_names[remap_index] = bone_name_raw;
            ++bones_mapped;
        }
    }

    if (bones_mapped != skeleton_bone_names.size())
        throw std::runtime_error("OMF bone remap does not cover all skeleton bones");

    const u16 motion_count = reader.read<u16>();
    output.metadata.clear();
    output.metadata.reserve(motion_count);

    for (u16 motion_idx = 0; motion_idx < motion_count; ++motion_idx)
    {
        LegacyMotionMetadata meta;
        meta.name = reader.read_stringz().c_str();
        meta.flags = reader.read<u32>();
        meta.bone_or_part = reader.read<u16>();
        meta.motion_id = reader.read<u16>();
        meta.speed = reader.read<float>();
        meta.power = reader.read<float>();
        meta.accrue = reader.read<float>();
        meta.falloff = reader.read<float>();

        if (version >= 4)
        {
            const u32 mark_count = reader.read<u32>();
            meta.marks.reserve(mark_count);
            for (u32 mark_idx = 0; mark_idx < mark_count; ++mark_idx)
            {
                LegacyMotionMark mark;
                mark.name = ReadStringCRLF(reader).c_str();
                const u32 interval_count = reader.read<u32>();
                mark.intervals.reserve(interval_count);
                for (u32 interval_idx = 0; interval_idx < interval_count; ++interval_idx)
                {
                    LegacyMotionInterval interval;
                    interval.first = reader.read<float>();
                    interval.second = reader.read<float>();
                    mark.intervals.emplace_back(interval);
                }
                meta.marks.emplace_back(std::move(mark));
            }
        }

        output.metadata.emplace_back(std::move(meta));
    }
}

void ParseMotions(const Chunk& chunk, const LegacyOmfData& params, LegacyOmfData& output)
{
    const auto subchunks = ParseSubchunks(chunk);
    if (subchunks.empty())
        throw std::runtime_error("OMF motion chunk missing motion count");

    const auto& count_chunk = subchunks.front();
    if (count_chunk.first != 0)
        throw std::runtime_error("OMF motion chunk missing count sub-chunk");

    BinaryReader count_reader{ count_chunk.second.data, count_chunk.second.size };
    const u32 motion_count = count_reader.read<u32>();
    if (motion_count != output.metadata.size())
        throw std::runtime_error("motion metadata count mismatch");

    output.motions.clear();
    output.motions.reserve(motion_count);

    std::vector<std::pair<u32, Chunk>> motion_chunks(subchunks.begin() + 1, subchunks.end());
    if (motion_chunks.size() != motion_count)
        throw std::runtime_error("OMF motion chunk count mismatch");

    for (u32 motion_idx = 0; motion_idx < motion_count; ++motion_idx)
    {
        const auto& item = motion_chunks[motion_idx];
        BinaryReader reader{ item.second.data, item.second.size };

        LegacyOmfMotion motion;
        motion.name = reader.read_stringz().c_str();
        motion.frame_count = reader.read<u32>();
        if (motion.frame_count == 0)
            throw std::runtime_error("motion has zero frames: " + std::string(motion.name.c_str()));

        motion.metadata = output.metadata[motion_idx];
        motion.bone_tracks.resize(params.bone_remap.size());

        for (size_t track_idx = 0; track_idx < params.bone_remap.size(); ++track_idx)
        {
            LegacyBoneTrack track;
            track.rotations.resize(motion.frame_count);
            track.translations.resize(motion.frame_count);
            track.translation_init.set(0.f, 0.f, 0.f);
            track.translation_size.set(0.f, 0.f, 0.f);

            const uint8_t flags = reader.read_uint8();
            track.flags = flags;
            const bool rotation_present = (flags & flRKeyAbsent) == 0;
            const bool translation_present = (flags & flTKeyPresent) != 0;
            const bool high_quality_translation = (flags & flTKey16IsBit) != 0;

            if (rotation_present)
            {
                track.rotation_crc = reader.read<u32>();
                track.rotation_keys.resize(motion.frame_count);
                for (u32 frame = 0; frame < motion.frame_count; ++frame)
                {
                    CKeyQR key{};
                    key.x = reader.read<int16_t>();
                    key.y = reader.read<int16_t>();
                    key.z = reader.read<int16_t>();
                    key.w = reader.read<int16_t>();
                    track.rotation_keys[frame] = key;

                    Fquaternion q;
                    q.x = static_cast<float>(key.x) * KEY_QuantI;
                    q.y = static_cast<float>(key.y) * KEY_QuantI;
                    q.z = static_cast<float>(key.z) * KEY_QuantI;
                    q.w = static_cast<float>(key.w) * KEY_QuantI;
                    q.normalize();
                    track.rotations[frame] = q;
                }
            }
            else
            {
                track.rotation_crc = 0;
                track.rotation_keys.resize(1);
                CKeyQR key{};
                key.x = reader.read<int16_t>();
                key.y = reader.read<int16_t>();
                key.z = reader.read<int16_t>();
                key.w = reader.read<int16_t>();
                track.rotation_keys[0] = key;

                Fquaternion q;
                q.x = static_cast<float>(key.x) * KEY_QuantI;
                q.y = static_cast<float>(key.y) * KEY_QuantI;
                q.z = static_cast<float>(key.z) * KEY_QuantI;
                q.w = static_cast<float>(key.w) * KEY_QuantI;
                q.normalize();
                std::fill(track.rotations.begin(), track.rotations.end(), q);
            }

            if (translation_present)
            {
                track.translation_crc = reader.read<u32>();
                if (high_quality_translation)
                {
                    track.translation_keys16.resize(motion.frame_count);
                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        CKeyQT16 sample{};
                        sample.x1 = reader.read<int16_t>();
                        sample.y1 = reader.read<int16_t>();
                        sample.z1 = reader.read<int16_t>();
                        track.translation_keys16[frame] = sample;
                    }

                    const Fvector size = reader.read_fvector3();
                    const Fvector init = reader.read_fvector3();
                    track.translation_size = size;
                    track.translation_init = init;

                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        const CKeyQT16& sample = track.translation_keys16[frame];
                        Fvector t;
                        t.x = static_cast<float>(sample.x1) * size.x + init.x;
                        t.y = static_cast<float>(sample.y1) * size.y + init.y;
                        t.z = static_cast<float>(sample.z1) * size.z + init.z;
                        track.translations[frame] = t;
                    }
                }
                else
                {
                    track.translation_keys8.resize(motion.frame_count);
                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        CKeyQT8 sample{};
                        sample.x1 = reader.read_int8();
                        sample.y1 = reader.read_int8();
                        sample.z1 = reader.read_int8();
                        track.translation_keys8[frame] = sample;
                    }

                    const Fvector size = reader.read_fvector3();
                    const Fvector init = reader.read_fvector3();
                    track.translation_size = size;
                    track.translation_init = init;

                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        const CKeyQT8& sample = track.translation_keys8[frame];
                        Fvector t;
                        t.x = static_cast<float>(sample.x1) * size.x + init.x;
                        t.y = static_cast<float>(sample.y1) * size.y + init.y;
                        t.z = static_cast<float>(sample.z1) * size.z + init.z;
                        track.translations[frame] = t;
                    }
                }
            }
            else
            {
                track.translation_crc = 0;
                const Fvector init = reader.read_fvector3();
                track.translation_init = init;
                track.translation_size.set(0.f, 0.f, 0.f);
                std::fill(track.translations.begin(), track.translations.end(), init);
            }

            motion.bone_tracks[track_idx] = std::move(track);
        }

        if (reader.offset != reader.size)
            throw std::runtime_error("unexpected extra data in motion chunk");

        output.motions.emplace_back(std::move(motion));
    }
}

LegacyOmfData ParseOmfBuffer(const std::byte* data, size_t size, const xr_vector<xr_string>& skeleton_bone_names)
{
    const auto chunks = ParseChunks(data, size);

    const auto params_it = chunks.find(OGF_S_SMPARAMS);
    if (params_it == chunks.end())
        throw std::runtime_error("OMF file missing smparams chunk");

    const auto motions_it = chunks.find(OGF_S_MOTIONS);
    if (motions_it == chunks.end())
        throw std::runtime_error("OMF file missing motions chunk");

    LegacyOmfData output;
    ParseSmparams(params_it->second, skeleton_bone_names, output);
    ParseMotions(motions_it->second, output, output);
    return output;
}

LegacyOmfData ParseOmfFile(const std::filesystem::path& path, const xr_vector<xr_string>& skeleton_bone_names)
{
    const auto data = LoadFileBytes(path);
    return ParseOmfBuffer(data.data(), data.size(), skeleton_bone_names);
}

ozz::animation::offline::RawAnimation BuildRawAnimation(const LegacyOmfMotion& motion, const LegacyOmfData& omf,
    const ozz::animation::Skeleton& skeleton)
{
    const size_t joint_count = omf.bone_remap.size();
    if (joint_count == 0)
        throw std::runtime_error("OMF bone remap is empty");

    ozz::animation::offline::RawAnimation raw_animation;
    raw_animation.name = motion.name.c_str();
    raw_animation.duration = motion.frame_count > 1 ? (motion.frame_count - 1) * SAMPLE_SPF : SAMPLE_SPF;
    raw_animation.tracks.resize(joint_count);

    for (size_t remap_index = 0; remap_index < omf.bone_remap.size(); ++remap_index)
    {
        const u16 joint_index = omf.bone_remap[remap_index];
        if (joint_index >= joint_count)
            throw std::runtime_error("bone remap references invalid joint index");

        const LegacyBoneTrack& source_track = motion.bone_tracks[remap_index];
        auto& track = raw_animation.tracks[joint_index];

        track.translations.resize(motion.frame_count);
        track.rotations.resize(motion.frame_count);
        track.scales.resize(1);
        track.scales[0].time = 0.f;
        track.scales[0].value = ozz::math::Float3(1.f, 1.f, 1.f);

        for (u32 frame = 0; frame < motion.frame_count; ++frame)
        {
            const float time = static_cast<float>(frame) * SAMPLE_SPF;

            const Fquaternion& xr_quat = source_track.rotations[frame];
            const Fvector& xr_translation = source_track.translations[frame];

            track.translations[frame].time = time;
            track.translations[frame].value =
                ozz::math::Float3(xr_translation.x, xr_translation.y, xr_translation.z);
            track.rotations[frame].time = time;
            track.rotations[frame].value =
                ozz::math::Normalize(ozz::math::Quaternion(xr_quat.x, xr_quat.y, xr_quat.z, xr_quat.w));
        }
    }

    return raw_animation;
}

ConvertedOmfAnimation BuildConvertedAnimation(const LegacyOmfMotion& motion, const LegacyOmfData& omf, const ozz::animation::Skeleton& skeleton, bool optimize)
{
    ozz::animation::offline::RawAnimation raw_animation = BuildRawAnimation(motion, omf, skeleton);

    ozz::animation::offline::AnimationBuilder builder;
    ozz::animation::offline::AnimationOptimizer optimizer;

    ozz::animation::offline::RawAnimation prepared = raw_animation;
    if (optimize)
    {
        ozz::animation::offline::RawAnimation optimized_raw;
        if (!optimizer(raw_animation, skeleton, &optimized_raw))
            throw std::runtime_error("animation optimization failed for motion: " + std::string(motion.name.c_str()));
        prepared = std::move(optimized_raw);
    }

    auto animation = builder(prepared);
    if (!animation)
        throw std::runtime_error("ozz animation build failed for motion: " + std::string(motion.name.c_str()));

    ConvertedOmfAnimation converted;
    converted.name = motion.name;
    converted.metadata = motion.metadata;
    converted.animation = std::move(animation);
    converted.frame_count = motion.frame_count;
    converted.bone_motions.reserve(omf.bone_remap.size());

    for (size_t remap_index = 0; remap_index < omf.bone_remap.size(); ++remap_index)
    {
        const u16 bone_id = omf.bone_remap[remap_index];
        if (bone_id == u16(BI_NONE))
            continue;

        const LegacyBoneTrack& track = motion.bone_tracks[remap_index];

        ConvertedBoneMotion bone_motion;
        bone_motion.bone_id = bone_id;
        bone_motion.flags = track.flags;
        bone_motion.rotation_crc = track.rotation_crc;
        bone_motion.translation_crc = track.translation_crc;
        bone_motion.rotation_keys = track.rotation_keys;
        bone_motion.translation_keys8 = track.translation_keys8;
        bone_motion.translation_keys16 = track.translation_keys16;
        bone_motion.translation_init = track.translation_init;
        bone_motion.translation_size = track.translation_size;

        converted.bone_motions.emplace_back(std::move(bone_motion));
    }

    return converted;
}

LegacyOmfMotion const* FindMotionByName(const LegacyOmfData& omf, const xr_string& name)
{
    const std::string target = ToLowerCopy(std::string(name.c_str()));
    for (const auto& motion : omf.motions)
    {
        if (ToLowerCopy(std::string(motion.name.c_str())) == target)
            return &motion;
    }
    return nullptr;
}

bool ConvertLegacyOmfImpl(const LegacyOmfData& omf,
    const ozz::animation::Skeleton& skeleton,
    xr_vector<ConvertedOmfAnimation>& out_animations,
    const std::optional<xr_string>& motion_filter,
    bool optimize)
{
    out_animations.clear();

    if (motion_filter)
    {
        const LegacyOmfMotion* motion = FindMotionByName(omf, *motion_filter);
        if (!motion)
            return false;
        out_animations.emplace_back(BuildConvertedAnimation(*motion, omf, skeleton, optimize));
        return true;
    }

    const size_t motion_count = omf.motions.size();
    out_animations.resize(motion_count);

    xr_parallel_for(TaskRange<size_t>(0, motion_count), [&](const TaskRange<size_t>& range)
    {
        for (size_t idx = range.begin(); idx != range.end(); ++idx)
        {
            out_animations[idx] = BuildConvertedAnimation(omf.motions[idx], omf, skeleton, optimize);
        }
    });

    return true;
}
} // namespace

void SerializeBoneMotions(ozz::io::OArchive& archive, const ConvertedOmfAnimation& animation)
{
    const uint32_t frame_count = animation.frame_count;
    archive << frame_count;

    const uint32_t bone_motion_count = static_cast<uint32_t>(animation.bone_motions.size());
    archive << bone_motion_count;

    for (const ConvertedBoneMotion& bone : animation.bone_motions)
    {
        archive << bone.bone_id;
        archive << bone.flags;

        uint8_t translation_format = 0;
        if (!bone.translation_keys16.empty())
            translation_format = 2;
        else if (!bone.translation_keys8.empty())
            translation_format = 1;
        archive << translation_format;

        archive << bone.rotation_crc;
        archive << bone.translation_crc;

        const uint32_t rotation_key_count = static_cast<uint32_t>(bone.rotation_keys.size());
        archive << rotation_key_count;
        for (const CKeyQR& key : bone.rotation_keys)
        {
            archive << key.x;
            archive << key.y;
            archive << key.z;
            archive << key.w;
        }

        switch (translation_format)
        {
        case 1:
        {
            const uint32_t translation_key_count = static_cast<uint32_t>(bone.translation_keys8.size());
            archive << translation_key_count;
            for (const CKeyQT8& key : bone.translation_keys8)
            {
                archive << key.x1;
                archive << key.y1;
                archive << key.z1;
            }
            break;
        }
        case 2:
        {
            const uint32_t translation_key_count = static_cast<uint32_t>(bone.translation_keys16.size());
            archive << translation_key_count;
            for (const CKeyQT16& key : bone.translation_keys16)
            {
                archive << key.x1;
                archive << key.y1;
                archive << key.z1;
            }
            break;
        }
        default:
        {
            const uint32_t translation_key_count = 0;
            archive << translation_key_count;
            break;
        }
        }

        archive << bone.translation_size.x;
        archive << bone.translation_size.y;
        archive << bone.translation_size.z;
        archive << bone.translation_init.x;
        archive << bone.translation_init.y;
        archive << bone.translation_init.z;
    }
}

bool ConvertLegacyOmf(const std::filesystem::path& omf_path,
                      const xr_vector<xr_string>& skeleton_bone_names,
                      const ozz::animation::Skeleton& skeleton,
                      xr_vector<ConvertedOmfAnimation>& out_animations,
                      std::optional<xr_string> motion_filter,
                      bool optimize)
{
    LegacyOmfData omf = ParseOmfFile(omf_path, skeleton_bone_names);
    return ConvertLegacyOmfImpl(omf, skeleton, out_animations, motion_filter, optimize);
}

bool ConvertLegacyOmf(const std::byte* data,
                      size_t size,
                      const xr_vector<xr_string>& skeleton_bone_names,
                      const ozz::animation::Skeleton& skeleton,
                      xr_vector<ConvertedOmfAnimation>& out_animations,
                      std::optional<xr_string> motion_filter,
                      bool optimize)
{
    if (!data || size == 0)
        return false;

    LegacyOmfData omf = ParseOmfBuffer(data, size, skeleton_bone_names);
    return ConvertLegacyOmfImpl(omf, skeleton, out_animations, motion_filter, optimize);
}
}
} // namespace XRay::Animation
