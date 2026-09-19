#include "stdafx.h"

#include "LegacyOmfConverter.h"

#include "LegacyChunkIO.h"

#include "xrCore/FMesh.hpp"
#include "xrCore/_quaternion.h"
#include "xrCore/_vector3d.h"
#include "xrCore/Animation/SkeletonMotionDefs.hpp"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include "xrCore/Threading/ParallelFor.hpp"

#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/base/maths/quaternion.h>
#include <ozz/base/maths/vec_float.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace XRay
{
namespace Animation
{
namespace
{
struct BoneTrack
{
    xr_vector<Fquaternion> rotations;
    xr_vector<Fvector> translations;
};

struct OmfMotion
{
    xr_string name;
    u32 frame_count = 0;
    xr_vector<BoneTrack> bone_tracks;
};

struct OmfData
{
    xr_vector<u16> bone_remap;
    u16 motion_count = 0;
    xr_vector<OmfMotion> motions;
};

void ParseSmparams(const Chunk& chunk, const xr_vector<xr_string>& skeleton_bone_names, OmfData& output)
{
    BinaryReader reader{ chunk.data, chunk.size };

    const u16 version = reader.read<u16>();
    if (version > xrOGF_SMParamsVersion)
        throw std::runtime_error("unsupported OMF params version");

    const u16 part_count = reader.read<u16>();

    output.bone_remap.assign(skeleton_bone_names.size(), std::numeric_limits<u16>::max());

    std::unordered_map<std::string, u16> skeleton_index_by_name;
    skeleton_index_by_name.reserve(skeleton_bone_names.size());
    for (u16 idx = 0; idx < skeleton_bone_names.size(); ++idx)
        skeleton_index_by_name.emplace(ToLowerCopy(std::string(skeleton_bone_names[idx].c_str())), idx);

    u32 bones_mapped = 0;

    for (u16 part = 0; part < part_count; ++part)
    {
        reader.read_stringz();
        const u16 bone_count = reader.read<u16>();
        for (u16 bone_idx = 0; bone_idx < bone_count; ++bone_idx)
        {
            const std::string bone_name_raw = reader.read_stringz();
            const u32 remap_index = reader.read<u32>();
            if (remap_index >= output.bone_remap.size())
                throw std::runtime_error("bone remap index out of range in OMF params");
            const auto it = skeleton_index_by_name.find(ToLowerCopy(bone_name_raw));
            if (it == skeleton_index_by_name.end())
                throw std::runtime_error("bone " + bone_name_raw + " referenced in OMF params not found in skeleton");
            output.bone_remap[remap_index] = it->second;
            ++bones_mapped;
        }
    }

    if (bones_mapped != skeleton_bone_names.size())
        throw std::runtime_error("OMF bone remap does not cover all skeleton bones");

    output.motion_count = reader.read<u16>();
}

void ParseMotions(const Chunk& chunk, OmfData& output)
{
    const auto subchunks = ParseSubchunks(chunk);
    if (subchunks.empty())
        throw std::runtime_error("OMF motion chunk missing motion count");

    const auto& count_chunk = subchunks.front();
    if (count_chunk.first != 0)
        throw std::runtime_error("OMF motion chunk missing count sub-chunk");

    BinaryReader count_reader{ count_chunk.second.data, count_chunk.second.size };
    const u32 motion_count = count_reader.read<u32>();
    if (motion_count != output.motion_count)
        throw std::runtime_error("motion metadata count mismatch");

    if (subchunks.size() - 1 != motion_count)
        throw std::runtime_error("OMF motion chunk count mismatch");

    output.motions.clear();
    output.motions.resize(motion_count);

    for (u32 motion_idx = 0; motion_idx < motion_count; ++motion_idx)
    {
        const Chunk& item = subchunks[motion_idx + 1].second;
        BinaryReader reader{ item.data, item.size };

        OmfMotion& motion = output.motions[motion_idx];
        motion.name = reader.read_stringz().c_str();
        motion.frame_count = reader.read<u32>();
        if (motion.frame_count == 0)
            throw std::runtime_error("motion has zero frames: " + std::string(motion.name.c_str()));

        motion.bone_tracks.resize(output.bone_remap.size());

        for (size_t track_idx = 0; track_idx < output.bone_remap.size(); ++track_idx)
        {
            BoneTrack& track = motion.bone_tracks[track_idx];
            track.rotations.resize(motion.frame_count);
            track.translations.resize(motion.frame_count);

            const std::uint8_t flags = reader.read_uint8();
            const bool rotation_present = (flags & flRKeyAbsent) == 0;
            const bool translation_present = (flags & flTKeyPresent) != 0;
            const bool high_quality_translation = (flags & flTKey16IsBit) != 0;

            const auto read_quaternion = [&reader]()
            {
                Fquaternion q;
                q.x = static_cast<float>(reader.read<std::int16_t>()) * KEY_QuantI;
                q.y = static_cast<float>(reader.read<std::int16_t>()) * KEY_QuantI;
                q.z = static_cast<float>(reader.read<std::int16_t>()) * KEY_QuantI;
                q.w = static_cast<float>(reader.read<std::int16_t>()) * KEY_QuantI;
                q.normalize();
                return q;
            };

            if (rotation_present)
            {
                reader.read<u32>();
                for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    track.rotations[frame] = read_quaternion();
            }
            else
            {
                std::fill(track.rotations.begin(), track.rotations.end(), read_quaternion());
            }

            if (translation_present)
            {
                reader.read<u32>();

                xr_vector<Fvector>& translations = track.translations;
                if (high_quality_translation)
                {
                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        Fvector& t = translations[frame];
                        t.x = static_cast<float>(reader.read<std::int16_t>());
                        t.y = static_cast<float>(reader.read<std::int16_t>());
                        t.z = static_cast<float>(reader.read<std::int16_t>());
                    }
                }
                else
                {
                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        Fvector& t = translations[frame];
                        t.x = static_cast<float>(reader.read_int8());
                        t.y = static_cast<float>(reader.read_int8());
                        t.z = static_cast<float>(reader.read_int8());
                    }
                }

                const Fvector size = reader.read_fvector3();
                const Fvector init = reader.read_fvector3();
                for (u32 frame = 0; frame < motion.frame_count; ++frame)
                {
                    Fvector& t = translations[frame];
                    t.x = t.x * size.x + init.x;
                    t.y = t.y * size.y + init.y;
                    t.z = t.z * size.z + init.z;
                }
            }
            else
            {
                const Fvector init = reader.read_fvector3();
                std::fill(track.translations.begin(), track.translations.end(), init);
            }
        }

        if (reader.offset != reader.size)
            throw std::runtime_error("unexpected extra data in motion chunk");
    }
}

OmfData ParseOmfBuffer(const std::byte* data, size_t size, const xr_vector<xr_string>& skeleton_bone_names)
{
    const auto chunks = ParseChunks(data, size);

    const auto params_it = chunks.find(OGF_S_SMPARAMS);
    if (params_it == chunks.end())
        throw std::runtime_error("OMF file missing smparams chunk");

    const auto motions_it = chunks.find(OGF_S_MOTIONS);
    if (motions_it == chunks.end())
        throw std::runtime_error("OMF file missing motions chunk");

    OmfData output;
    ParseSmparams(params_it->second, skeleton_bone_names, output);
    ParseMotions(motions_it->second, output);
    return output;
}

ozz::animation::offline::RawAnimation BuildRawAnimation(const OmfMotion& motion, const OmfData& omf)
{
    const size_t joint_count = omf.bone_remap.size();
    if (joint_count == 0)
        throw std::runtime_error("OMF bone remap is empty");

    ozz::animation::offline::RawAnimation raw_animation;
    raw_animation.name = motion.name.c_str();
    raw_animation.duration = motion.frame_count > 1 ? (motion.frame_count - 1) * SAMPLE_SPF : SAMPLE_SPF;
    raw_animation.tracks.resize(joint_count);

    for (size_t remap_index = 0; remap_index < joint_count; ++remap_index)
    {
        const u16 joint_index = omf.bone_remap[remap_index];
        if (joint_index >= joint_count)
            throw std::runtime_error("bone remap references invalid joint index");

        const BoneTrack& source_track = motion.bone_tracks[remap_index];
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
            track.translations[frame].value = ozz::math::Float3(xr_translation.x, xr_translation.y, xr_translation.z);
            track.rotations[frame].time = time;
            track.rotations[frame].value =
                ozz::math::Normalize(ozz::math::Quaternion(xr_quat.x, xr_quat.y, xr_quat.z, xr_quat.w));
        }
    }

    return raw_animation;
}

ConvertedOmfAnimation BuildConvertedAnimation(const OmfMotion& motion, const OmfData& omf)
{
    const ozz::animation::offline::AnimationBuilder builder;
    auto animation = builder(BuildRawAnimation(motion, omf));
    if (!animation)
        throw std::runtime_error("ozz animation build failed for motion: " + std::string(motion.name.c_str()));

    ConvertedOmfAnimation converted;
    converted.name = motion.name;
    converted.frame_count = motion.frame_count;
    converted.animation = std::move(animation);
    return converted;
}
}

bool ConvertLegacyOmf(const std::byte* data, size_t size, const xr_vector<xr_string>& skeleton_bone_names,
    const ozz::animation::Skeleton& skeleton, xr_vector<ConvertedOmfAnimation>& out_animations)
{
    out_animations.clear();

    if (!data || size == 0)
        return false;

    if (size_t(skeleton.num_joints()) != skeleton_bone_names.size())
        return false;

    const OmfData omf = ParseOmfBuffer(data, size, skeleton_bone_names);

    const size_t motion_count = omf.motions.size();
    out_animations.resize(motion_count);

    xr_parallel_for(TaskRange<size_t>(0, motion_count), [&](const TaskRange<size_t>& range)
    {
        for (size_t idx = range.begin(); idx != range.end(); ++idx)
            out_animations[idx] = BuildConvertedAnimation(omf.motions[idx], omf);
    });

    return true;
}
}
}
