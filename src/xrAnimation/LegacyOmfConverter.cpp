#include "stdafx.h"

#include "LegacyOmfConverter.h"

#include "OzzConversion.h"

#include "xrCore/Animation/SkeletonMotionDefs.hpp"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include "xrCore/Threading/ParallelFor.hpp"

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
struct BinaryReader
{
    const std::byte* data = nullptr;
    size_t size = 0;
    size_t offset = 0;

    template <class T>
    T Read()
    {
        if (offset + sizeof(T) > size)
            throw std::runtime_error("unexpected end of chunk while reading typed data");
        T value;
        std::memcpy(&value, data + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    int8_t ReadInt8()
    {
        if (offset + sizeof(int8_t) > size)
            throw std::runtime_error("unexpected end of chunk while reading int8");
        const int8_t value = *reinterpret_cast<const int8_t*>(data + offset);
        offset += sizeof(int8_t);
        return value;
    }

    uint8_t ReadUInt8()
    {
        if (offset + sizeof(uint8_t) > size)
            throw std::runtime_error("unexpected end of chunk while reading uint8");
        const uint8_t value = *reinterpret_cast<const uint8_t*>(data + offset);
        offset += sizeof(uint8_t);
        return value;
    }

    std::string ReadStringZ()
    {
        const auto* begin = data + offset;
        const auto* end = data + size;
        const auto* cursor = begin;
        while (cursor < end && *reinterpret_cast<const char*>(cursor) != '\0')
            ++cursor;
        if (cursor == end)
            throw std::runtime_error("unterminated string in chunk");
        std::string value(reinterpret_cast<const char*>(begin), static_cast<size_t>(cursor - begin));
        offset += static_cast<size_t>(cursor - begin) + 1;
        return value;
    }

    Fvector ReadFvector3()
    {
        Fvector v{};
        v.x = Read<float>();
        v.y = Read<float>();
        v.z = Read<float>();
        return v;
    }
};

struct Chunk
{
    const std::byte* data = nullptr;
    size_t size = 0;
};

std::vector<std::byte> LoadFileBytes(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        throw std::runtime_error("failed to open file: " + path.string());

    const auto size = stream.tellg();
    if (size <= 0)
        throw std::runtime_error("input file is empty: " + path.string());

    std::vector<std::byte> data(static_cast<size_t>(size));
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!stream)
        throw std::runtime_error("failed to read file: " + path.string());

    return data;
}

std::unordered_map<u32, Chunk> ParseChunks(const std::byte* data, size_t size)
{
    std::unordered_map<u32, Chunk> chunks;
    size_t offset = 0;
    while (offset + sizeof(u32) * 2 <= size)
    {
        u32 id = 0;
        u32 chunk_size = 0;
        std::memcpy(&id, data + offset, sizeof(u32));
        offset += sizeof(u32);
        std::memcpy(&chunk_size, data + offset, sizeof(u32));
        offset += sizeof(u32);
        if (offset + chunk_size > size)
            throw std::runtime_error("chunk extends past end of file");
        chunks.emplace(id, Chunk{ data + offset, chunk_size });
        offset += chunk_size;
    }
    return chunks;
}

std::vector<std::pair<u32, Chunk>> ParseSubchunks(const Chunk& chunk)
{
    std::vector<std::pair<u32, Chunk>> subchunks;
    size_t offset = 0;
    while (offset + sizeof(u32) * 2 <= chunk.size)
    {
        u32 id = 0;
        u32 sub_size = 0;
        std::memcpy(&id, chunk.data + offset, sizeof(u32));
        offset += sizeof(u32);
        std::memcpy(&sub_size, chunk.data + offset, sizeof(u32));
        offset += sizeof(u32);
        if (offset + sub_size > chunk.size)
            throw std::runtime_error("sub-chunk extends past parent chunk");
        subchunks.emplace_back(id, Chunk{ chunk.data + offset, sub_size });
        offset += sub_size;
    }
    return subchunks;
}

std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch)
        {
            return static_cast<char>(std::tolower(ch));
        });
    return value;
}

std::string ReadStringCRLF(BinaryReader& reader)
{
    std::string value;
    while (reader.offset < reader.size)
    {
        const char ch = static_cast<char>(reader.ReadInt8());
        if (ch == '\r')
        {
            if (reader.offset >= reader.size)
                throw std::runtime_error("unexpected end of data while reading CRLF string");
            const char lf = static_cast<char>(reader.ReadInt8());
            if (lf != '\n')
                throw std::runtime_error("expected LF after CR in motion mark name");
            break;
        }
        value.push_back(ch);
    }
    return value;
}

void ParseSmparams(const Chunk& chunk, const xr_vector<xr_string>& skeleton_bone_names, LegacyOmfData& output)
{
    BinaryReader reader{ chunk.data, chunk.size };

    const u16 version = reader.Read<u16>();
    if (version > xrOGF_SMParamsVersion)
        throw std::runtime_error("unsupported OMF params version");

    const u16 part_count = reader.Read<u16>();

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
        reader.ReadStringZ();
        const u16 bone_count = reader.Read<u16>();
        for (u16 bone_idx = 0; bone_idx < bone_count; ++bone_idx)
        {
            const xr_string bone_name_raw = reader.ReadStringZ().c_str();
            const u32 remap_index = reader.Read<u32>();
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

    const u16 motion_count = reader.Read<u16>();
    output.metadata.clear();
    output.metadata.reserve(motion_count);

    for (u16 motion_idx = 0; motion_idx < motion_count; ++motion_idx)
    {
        LegacyMotionMetadata meta;
        meta.name = reader.ReadStringZ().c_str();
        meta.flags = reader.Read<u32>();
        meta.bone_or_part = reader.Read<u16>();
        meta.motion_id = reader.Read<u16>();
        meta.speed = reader.Read<float>();
        meta.power = reader.Read<float>();
        meta.accrue = reader.Read<float>();
        meta.falloff = reader.Read<float>();

        if (version >= 4)
        {
            const u32 mark_count = reader.Read<u32>();
            meta.marks.reserve(mark_count);
            for (u32 mark_idx = 0; mark_idx < mark_count; ++mark_idx)
            {
                LegacyMotionMark mark;
                mark.name = ReadStringCRLF(reader).c_str();
                const u32 interval_count = reader.Read<u32>();
                mark.intervals.reserve(interval_count);
                for (u32 interval_idx = 0; interval_idx < interval_count; ++interval_idx)
                {
                    LegacyMotionInterval interval;
                    interval.first = reader.Read<float>();
                    interval.second = reader.Read<float>();
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
    const u32 motion_count = count_reader.Read<u32>();
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
        motion.name = reader.ReadStringZ().c_str();
        motion.frame_count = reader.Read<u32>();
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

            const uint8_t flags = reader.ReadUInt8();
            track.flags = flags;
            const bool rotation_present = (flags & flRKeyAbsent) == 0;
            const bool translation_present = (flags & flTKeyPresent) != 0;
            const bool high_quality_translation = (flags & flTKey16IsBit) != 0;

            if (rotation_present)
            {
                track.rotation_crc = reader.Read<u32>();
                track.rotation_keys.resize(motion.frame_count);
                for (u32 frame = 0; frame < motion.frame_count; ++frame)
                {
                    CKeyQR key{};
                    key.x = reader.Read<int16_t>();
                    key.y = reader.Read<int16_t>();
                    key.z = reader.Read<int16_t>();
                    key.w = reader.Read<int16_t>();
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
                key.x = reader.Read<int16_t>();
                key.y = reader.Read<int16_t>();
                key.z = reader.Read<int16_t>();
                key.w = reader.Read<int16_t>();
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
                track.translation_crc = reader.Read<u32>();
                if (high_quality_translation)
                {
                    track.translation_keys16.resize(motion.frame_count);
                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        CKeyQT16 sample{};
                        sample.x1 = reader.Read<int16_t>();
                        sample.y1 = reader.Read<int16_t>();
                        sample.z1 = reader.Read<int16_t>();
                        track.translation_keys16[frame] = sample;
                    }

                    const Fvector size = reader.ReadFvector3();
                    const Fvector init = reader.ReadFvector3();
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
                        sample.x1 = reader.ReadInt8();
                        sample.y1 = reader.ReadInt8();
                        sample.z1 = reader.ReadInt8();
                        track.translation_keys8[frame] = sample;
                    }

                    const Fvector size = reader.ReadFvector3();
                    const Fvector init = reader.ReadFvector3();
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
                const Fvector init = reader.ReadFvector3();
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

            Fmatrix local;
            local.mk_xform(xr_quat, xr_translation);

            const auto ozz_matrix = ConvertXRayMatrixToOzz(local);
            track.translations[frame].time = time;
            track.translations[frame].value = XRay::Animation::ExtractTranslation(ozz_matrix);
            track.rotations[frame].time = time;
            track.rotations[frame].value = XRay::Animation::ExtractQuaternion(ozz_matrix);
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
