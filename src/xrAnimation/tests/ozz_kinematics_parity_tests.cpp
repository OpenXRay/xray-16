#include "Common/Platform.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

#ifndef PROJECT_ROOT
#    define PROJECT_ROOT ""
#endif

#include "xrCore/xrCore.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrCore/Animation/SkeletonMotionDefs.hpp"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include "xrCore/FMesh.hpp"

#include "OzzConversion.h"
#include "OzzKinematics.h"
#include "OzzKinematicsAnimated.h"
#include "OzzBundle.h"
#include "../samples/framework/mesh.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/transform.h"
#include "ozz/base/span.h"

#include "Layers/xrRender/ModelNaming.h"

namespace
{
using XRay::Animation::ConvertOzzMatrixToXRay;
using XRay::Animation::OzzKinematics;

std::filesystem::path ResolveProjectPath(const std::string& relative)
{
    static const std::filesystem::path root = []
    {
#ifdef PROJECT_ROOT
        std::filesystem::path from_macro(PROJECT_ROOT);
        if (!from_macro.empty())
        {
#    ifdef _WIN32
            from_macro.make_preferred();
#    endif
            return from_macro;
        }
#endif
        std::filesystem::path from_file(__FILE__);
        auto root_path = from_file.parent_path();
        root_path = root_path.parent_path();
        root_path = root_path.parent_path();
        root_path = root_path.parent_path();
#ifdef _WIN32
        root_path.make_preferred();
#endif
        return root_path;
    }();
    return root / relative;
}

struct BindPoseSample
{
    std::unordered_map<std::string, Fmatrix> world_space_transforms;
};

struct AnimationSample
{
    std::unordered_map<std::string, Fmatrix> world_space_transforms;
    float applied_time_seconds = 0.f;
};

std::string ToLowerCopy(std::string_view value)
{
    std::string lower(value.begin(), value.end());
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char ch)
        {
            return static_cast<char>(std::tolower(ch));
        });
    return lower;
}

struct LegacyChunk
{
    const std::byte* data = nullptr;
    size_t size = 0;
};

struct LegacyBinaryReader
{
    const std::byte* data = nullptr;
    size_t size = 0;
    size_t offset = 0;

    template <class T>
    T Read()
    {
        if (offset + sizeof(T) > size)
            throw std::runtime_error("unexpected end of chunk while reading typed data");

        T value{};
        std::memcpy(&value, data + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    template <class T>
    T ReadStruct()
    {
        return Read<T>();
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

    void Skip(size_t count)
    {
        if (offset + count > size)
            throw std::runtime_error("attempted to skip past end of chunk");
        offset += count;
    }
};

std::string ReadStringCrlf(LegacyBinaryReader& reader)
{
    std::string value;
    while (reader.offset < reader.size)
    {
        const char ch = static_cast<char>(reader.Read<uint8_t>());
        if (ch == '\r')
        {
            if (reader.offset >= reader.size)
                throw std::runtime_error("unexpected end of data while reading CRLF string");
            const char lf = static_cast<char>(reader.Read<uint8_t>());
            if (lf != '\n')
                throw std::runtime_error("expected LF after CR");
            break;
        }
        value.push_back(ch);
    }
    return value;
}

using LegacyChunkMap = std::unordered_map<u32, LegacyChunk>;

std::vector<std::byte> LoadBinaryFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        throw std::runtime_error("failed to open input file: " + path.string());

    const auto size = stream.tellg();
    if (size <= 0)
        throw std::runtime_error("input file is empty: " + path.string());

    std::vector<std::byte> data(static_cast<size_t>(size));
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(data.data()), data.size());
    if (!stream)
        throw std::runtime_error("failed to read input file: " + path.string());

    return data;
}

LegacyChunkMap ParseChunks(const std::byte* data, size_t size)
{
    LegacyChunkMap chunks;

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

        chunks[id] = LegacyChunk{ data + offset, chunk_size };
        offset += chunk_size;
    }

    return chunks;
}

std::vector<std::pair<u32, LegacyChunk>> ParseSubchunks(const LegacyChunk& chunk)
{
    std::vector<std::pair<u32, LegacyChunk>> subchunks;

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

        subchunks.emplace_back(id, LegacyChunk{ chunk.data + offset, sub_size });
        offset += sub_size;
    }

    return subchunks;
}

struct LegacyBoneRecord
{
    std::string name;
    std::string parent_name;
    int parent_index = -1;
    Fvector rest_translation{};
    Fvector rest_rotation{};
    Fmatrix local_transform{};
    Fmatrix global_transform{};
};

struct LegacyAnimationTrack
{
    std::vector<Fquaternion> rotations;
    std::vector<Fvector> translations;
};

struct LegacyMotion
{
    std::string name;
    u32 frame_count = 0;
    std::vector<LegacyAnimationTrack> tracks;
};

struct LegacyAnimationData
{
    std::vector<uint16_t> bone_remap;
    std::vector<std::string> remap_bone_names;
    std::vector<LegacyMotion> motions;
    std::vector<std::string> motion_names;
};

struct TestBoneCallbackContext
{
    int call_count = 0;
    CBoneInstance* last_instance = nullptr;
    Fmatrix pre_transform{};
    Fmatrix override_transform{};
};

void TestBoneCallbackFunction(CBoneInstance* instance)
{
    if (!instance)
        return;

    auto* context = static_cast<TestBoneCallbackContext*>(instance->callback_param());
    if (!context)
        return;

    ++context->call_count;
    context->last_instance = instance;
    context->pre_transform = instance->mTransform;
    instance->mTransform = context->override_transform;
}

std::vector<LegacyBoneRecord> ReadBoneNames(const LegacyChunk& chunk)
{
    LegacyBinaryReader reader{ chunk.data, chunk.size, 0 };
    const u32 bone_count = reader.Read<u32>();

    std::vector<LegacyBoneRecord> bones;
    bones.reserve(bone_count);

    for (u32 idx = 0; idx < bone_count; ++idx)
    {
        LegacyBoneRecord record;
        record.name = reader.ReadStringZ();
        record.parent_name = reader.ReadStringZ();
        reader.Skip(sizeof(Fobb));
        bones.emplace_back(std::move(record));
    }

    return bones;
}

void ReadIkData(const LegacyChunk& chunk, std::vector<LegacyBoneRecord>& bones)
{
    LegacyBinaryReader reader{ chunk.data, chunk.size, 0 };

    for (auto& bone : bones)
    {
        const u32 version = reader.Read<u32>();
        reader.ReadStringZ();
        (void)reader.ReadStruct<SBoneShape>();

        reader.Read<u32>();
        for (int axis = 0; axis < 3; ++axis)
        {
            reader.Read<float>();
            reader.Read<float>();
            reader.Read<float>();
            reader.Read<float>();
        }

        reader.Read<float>();
        reader.Read<float>();
        reader.Read<u32>();
        reader.Read<float>();
        reader.Read<float>();
        if (version > 0)
            reader.Read<float>();

        bone.rest_rotation = reader.ReadFvector3();
        bone.rest_translation = reader.ReadFvector3();
        reader.Read<float>();
        reader.ReadFvector3();
    }
}

void ResolveHierarchy(std::vector<LegacyBoneRecord>& bones)
{
    std::unordered_map<std::string, int> index_by_name;
    index_by_name.reserve(bones.size());

    for (size_t idx = 0; idx < bones.size(); ++idx)
        index_by_name[bones[idx].name] = static_cast<int>(idx);

    for (auto& bone : bones)
    {
        if (bone.parent_name.empty())
        {
            bone.parent_index = -1;
            continue;
        }

        const auto it = index_by_name.find(bone.parent_name);
        if (it == index_by_name.end())
            throw std::runtime_error("bone parent not found: " + bone.parent_name + " for bone " + bone.name);
        bone.parent_index = it->second;
    }
}

void BuildLocalTransforms(std::vector<LegacyBoneRecord>& bones)
{
    for (auto& bone : bones)
    {
        Fmatrix matrix;
        matrix.identity();
        matrix.setXYZi(bone.rest_rotation);
        matrix.translate_over(bone.rest_translation);
        bone.local_transform = matrix;
        bone.global_transform.identity();
    }
}

void BuildGlobalTransforms(std::vector<LegacyBoneRecord>& bones)
{
    std::vector<std::vector<int>> children(bones.size());
    for (size_t idx = 0; idx < bones.size(); ++idx)
        if (bones[idx].parent_index >= 0)
            children[static_cast<size_t>(bones[idx].parent_index)].push_back(static_cast<int>(idx));

    std::function<void(int, const Fmatrix&)> visit;
    visit = [&](int index, const Fmatrix& parent_matrix)
    {
        auto& bone = bones[static_cast<size_t>(index)];
        bone.global_transform.mul_43(parent_matrix, bone.local_transform);
        for (int child : children[static_cast<size_t>(index)])
            visit(child, bone.global_transform);
    };

    Fmatrix identity;
    identity.identity();

    for (size_t idx = 0; idx < bones.size(); ++idx)
        if (bones[idx].parent_index < 0)
            visit(static_cast<int>(idx), identity);
}

std::vector<Fmatrix> BuildWorldTransformsFromLocals(const std::vector<LegacyBoneRecord>& bones, const std::vector<Fmatrix>& locals)
{
    std::vector<std::vector<int>> children(bones.size());
    for (size_t idx = 0; idx < bones.size(); ++idx)
        if (bones[idx].parent_index >= 0)
            children[static_cast<size_t>(bones[idx].parent_index)].push_back(static_cast<int>(idx));

    std::vector<Fmatrix> world(bones.size());

    std::function<void(int, const Fmatrix&)> visit;
    visit = [&](int index, const Fmatrix& parent_matrix)
    {
        const size_t bone_index = static_cast<size_t>(index);
        world[bone_index].mul_43(parent_matrix, locals[bone_index]);
        for (int child : children[bone_index])
            visit(child, world[bone_index]);
    };

    Fmatrix identity;
    identity.identity();

    for (size_t idx = 0; idx < bones.size(); ++idx)
        if (bones[idx].parent_index < 0)
            visit(static_cast<int>(idx), identity);

    return world;
}

void ParseLegacySmparams(const LegacyChunk& chunk, const std::vector<LegacyBoneRecord>& skeleton_bones, LegacyAnimationData& output)
{
    LegacyBinaryReader reader{ chunk.data, chunk.size, 0 };

    const u16 version = reader.Read<u16>();
    if (version > xrOGF_SMParamsVersion)
        throw std::runtime_error("unsupported OMF params version");

    const u16 part_count = reader.Read<u16>();

    output.bone_remap.assign(skeleton_bones.size(), std::numeric_limits<uint16_t>::max());
    output.remap_bone_names.assign(skeleton_bones.size(), std::string());

    std::unordered_map<std::string, uint16_t> skeleton_index_by_name;
    skeleton_index_by_name.reserve(skeleton_bones.size());
    for (uint16_t idx = 0; idx < skeleton_bones.size(); ++idx)
        skeleton_index_by_name.emplace(ToLowerCopy(skeleton_bones[idx].name), idx);

    uint32_t mapped = 0;

    for (u16 part = 0; part < part_count; ++part)
    {
        reader.ReadStringZ();
        const u16 bone_count = reader.Read<u16>();
        for (u16 bone_idx = 0; bone_idx < bone_count; ++bone_idx)
        {
            const std::string bone_name = reader.ReadStringZ();
            const uint32_t remap_index = reader.Read<u32>();

            if (remap_index >= output.bone_remap.size())
                throw std::runtime_error("bone remap index out of range");

            const auto it = skeleton_index_by_name.find(ToLowerCopy(bone_name));
            if (it == skeleton_index_by_name.end())
                throw std::runtime_error("bone " + bone_name + " not found in skeleton remap");

            output.bone_remap[remap_index] = it->second;
            output.remap_bone_names[remap_index] = bone_name;
            ++mapped;
        }
    }

    if (mapped != skeleton_bones.size())
        throw std::runtime_error("OMF bone remap does not cover entire skeleton");

    const u16 motion_count = reader.Read<u16>();
    output.motion_names.clear();
    output.motion_names.reserve(motion_count);

    for (u16 motion_idx = 0; motion_idx < motion_count; ++motion_idx)
    {
        std::string motion_name = reader.ReadStringZ();
        output.motion_names.emplace_back(motion_name);

        reader.Read<u32>();
        reader.Read<u16>();
        reader.Read<u16>();
        reader.Read<float>();
        reader.Read<float>();
        reader.Read<float>();
        reader.Read<float>();

        if (version >= 4)
        {
            const u32 mark_count = reader.Read<u32>();
            for (u32 mark_idx = 0; mark_idx < mark_count; ++mark_idx)
            {
                ReadStringCrlf(reader);
                const u32 interval_count = reader.Read<u32>();
                for (u32 interval_idx = 0; interval_idx < interval_count; ++interval_idx)
                {
                    reader.Read<float>();
                    reader.Read<float>();
                }
            }
        }
    }
}

void ParseLegacyMotions(const LegacyChunk& chunk, const LegacyAnimationData& params, LegacyAnimationData& output)
{
    const auto subchunks = ParseSubchunks(chunk);
    if (subchunks.empty())
        throw std::runtime_error("OMF motion chunk missing count");

    const auto& count_chunk = subchunks.front();
    if (count_chunk.first != 0)
        throw std::runtime_error("OMF motion chunk missing count sub-chunk");

    LegacyBinaryReader count_reader{ count_chunk.second.data, count_chunk.second.size, 0 };
    const u32 motion_count = count_reader.Read<u32>();
    if (motion_count != params.motion_names.size())
        throw std::runtime_error("motion metadata count mismatch");

    output.motions.clear();
    output.motions.reserve(motion_count);

    if (subchunks.size() - 1 != motion_count)
        throw std::runtime_error("motion chunk count mismatch");

    for (u32 motion_idx = 0; motion_idx < motion_count; ++motion_idx)
    {
        const auto& item = subchunks[motion_idx + 1];
        LegacyBinaryReader reader{ item.second.data, item.second.size, 0 };

        LegacyMotion motion;
        motion.name = reader.ReadStringZ();
        motion.frame_count = reader.Read<u32>();
        if (motion.frame_count == 0)
            throw std::runtime_error("motion has zero frames: " + motion.name);

        motion.tracks.resize(params.bone_remap.size());

        for (size_t track_idx = 0; track_idx < params.bone_remap.size(); ++track_idx)
        {
            LegacyAnimationTrack track;
            track.rotations.resize(motion.frame_count);
            track.translations.resize(motion.frame_count);

            const uint8_t flags = reader.Read<uint8_t>();
            const bool rotation_present = (flags & flRKeyAbsent) == 0;
            const bool translation_present = (flags & flTKeyPresent) != 0;
            const bool high_quality_translation = (flags & flTKey16IsBit) != 0;

            auto read_quaternion = [&reader]()
            {
                const int16_t x = reader.Read<int16_t>();
                const int16_t y = reader.Read<int16_t>();
                const int16_t z = reader.Read<int16_t>();
                const int16_t w = reader.Read<int16_t>();
                Fquaternion q;
                q.x = static_cast<float>(x) * KEY_QuantI;
                q.y = static_cast<float>(y) * KEY_QuantI;
                q.z = static_cast<float>(z) * KEY_QuantI;
                q.w = static_cast<float>(w) * KEY_QuantI;
                q.normalize();
                return q;
            };

            if (rotation_present)
            {
                reader.Read<u32>();
                for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    track.rotations[frame] = read_quaternion();
            }
            else
            {
                const Fquaternion q = read_quaternion();
                std::fill(track.rotations.begin(), track.rotations.end(), q);
            }

            if (translation_present)
            {
                reader.Read<u32>();

                if (high_quality_translation)
                {
                    std::vector<std::array<int16_t, 3>> samples(motion.frame_count);
                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        samples[frame][0] = reader.Read<int16_t>();
                        samples[frame][1] = reader.Read<int16_t>();
                        samples[frame][2] = reader.Read<int16_t>();
                    }

                    const Fvector size = reader.ReadFvector3();
                    const Fvector init = reader.ReadFvector3();

                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        Fvector t;
                        t.x = static_cast<float>(samples[frame][0]) * size.x + init.x;
                        t.y = static_cast<float>(samples[frame][1]) * size.y + init.y;
                        t.z = static_cast<float>(samples[frame][2]) * size.z + init.z;
                        track.translations[frame] = t;
                    }
                }
                else
                {
                    std::vector<std::array<int8_t, 3>> samples(motion.frame_count);
                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        samples[frame][0] = reader.Read<int8_t>();
                        samples[frame][1] = reader.Read<int8_t>();
                        samples[frame][2] = reader.Read<int8_t>();
                    }

                    const Fvector size = reader.ReadFvector3();
                    const Fvector init = reader.ReadFvector3();

                    for (u32 frame = 0; frame < motion.frame_count; ++frame)
                    {
                        Fvector t;
                        t.x = static_cast<float>(samples[frame][0]) * size.x + init.x;
                        t.y = static_cast<float>(samples[frame][1]) * size.y + init.y;
                        t.z = static_cast<float>(samples[frame][2]) * size.z + init.z;
                        track.translations[frame] = t;
                    }
                }
            }
            else
            {
                const Fvector init = reader.ReadFvector3();
                std::fill(track.translations.begin(), track.translations.end(), init);
            }

            motion.tracks[track_idx] = std::move(track);
        }

        if (reader.offset != reader.size)
            throw std::runtime_error("unexpected data remaining in motion chunk");

        output.motions.emplace_back(std::move(motion));
    }
}

LegacyAnimationData ParseLegacyAnimationData(const LegacyChunkMap& chunks, const std::vector<LegacyBoneRecord>& bones)
{
    const auto params_it = chunks.find(OGF_S_SMPARAMS);
    if (params_it == chunks.end())
        throw std::runtime_error("OMF missing params chunk");

    const auto motions_it = chunks.find(OGF_S_MOTIONS);
    if (motions_it == chunks.end())
        throw std::runtime_error("OMF missing motions chunk");

    LegacyAnimationData data;
    ParseLegacySmparams(params_it->second, bones, data);
    ParseLegacyMotions(motions_it->second, data, data);
    return data;
}

std::string ReadOzzString(ozz::io::IArchive& archive)
{
    uint32_t length = 0;
    archive >> length;
    std::string value(length, '\0');
    if (length > 0)
        archive >> ozz::io::MakeArray(value.data(), length);
    return value;
}

struct SerializedMotionInterval
{
    float first = 0.f;
    float second = 0.f;
};

struct SerializedMotionMark
{
    std::string name;
    std::vector<SerializedMotionInterval> intervals;
};

struct SerializedBoneMotion
{
    uint16_t bone_id = BI_NONE;
    uint8_t flags = 0;
    uint8_t translation_format = 0;
    uint32_t rotation_crc = 0;
    uint32_t translation_crc = 0;
    std::vector<CKeyQR> rotation_keys;
    std::vector<CKeyQT8> translation_keys8;
    std::vector<CKeyQT16> translation_keys16;
    Fvector translation_size{};
    Fvector translation_init{};
};

struct SerializedMotionMetadata
{
    std::string name;
    uint32_t flags = 0;
    uint16_t bone_or_part = 0;
    uint16_t motion_id = 0;
    float speed = 0.f;
    float power = 0.f;
    float accrue = 0.f;
    float falloff = 0.f;
    std::vector<SerializedMotionMark> marks;
    uint32_t frame_count = 0;
    std::vector<SerializedBoneMotion> bone_motions;
};

SerializedMotionMetadata ReadOzzMotionMetadata(ozz::io::IArchive& archive)
{
    SerializedMotionMetadata metadata;

    metadata.name = ReadOzzString(archive);

    archive >> metadata.flags;
    archive >> metadata.bone_or_part;
    archive >> metadata.motion_id;
    archive >> metadata.speed;
    archive >> metadata.power;
    archive >> metadata.accrue;
    archive >> metadata.falloff;

    uint32_t mark_count = 0;
    archive >> mark_count;
    metadata.marks.reserve(mark_count);
    for (uint32_t mark_idx = 0; mark_idx < mark_count; ++mark_idx)
    {
        SerializedMotionMark mark;
        mark.name = ReadOzzString(archive);
        uint32_t interval_count = 0;
        archive >> interval_count;
        mark.intervals.reserve(interval_count);
        for (uint32_t interval_idx = 0; interval_idx < interval_count; ++interval_idx)
        {
            SerializedMotionInterval interval;
            archive >> interval.first;
            archive >> interval.second;
            mark.intervals.push_back(interval);
        }
        metadata.marks.emplace_back(std::move(mark));
    }

    archive >> metadata.frame_count;

    uint32_t bone_motion_count = 0;
    archive >> bone_motion_count;
    metadata.bone_motions.resize(bone_motion_count);

    for (uint32_t bone_index = 0; bone_index < bone_motion_count; ++bone_index)
    {
        SerializedBoneMotion bone;
        archive >> bone.bone_id;
        archive >> bone.flags;
        archive >> bone.translation_format;
        archive >> bone.rotation_crc;
        archive >> bone.translation_crc;

        uint32_t rotation_key_count = 0;
        archive >> rotation_key_count;
        bone.rotation_keys.resize(rotation_key_count);
        for (uint32_t key_index = 0; key_index < rotation_key_count; ++key_index)
        {
            archive >> bone.rotation_keys[key_index].x;
            archive >> bone.rotation_keys[key_index].y;
            archive >> bone.rotation_keys[key_index].z;
            archive >> bone.rotation_keys[key_index].w;
        }

        uint32_t translation_key_count = 0;
        archive >> translation_key_count;
        switch (bone.translation_format)
        {
        case 1:
            bone.translation_keys8.resize(translation_key_count);
            for (uint32_t key_index = 0; key_index < translation_key_count; ++key_index)
            {
                archive >> bone.translation_keys8[key_index].x1;
                archive >> bone.translation_keys8[key_index].y1;
                archive >> bone.translation_keys8[key_index].z1;
            }
            break;
        case 2:
            bone.translation_keys16.resize(translation_key_count);
            for (uint32_t key_index = 0; key_index < translation_key_count; ++key_index)
            {
                archive >> bone.translation_keys16[key_index].x1;
                archive >> bone.translation_keys16[key_index].y1;
                archive >> bone.translation_keys16[key_index].z1;
            }
            break;
        default:
            for (uint32_t key_index = 0; key_index < translation_key_count; ++key_index)
            {
                int16_t x = 0;
                int16_t y = 0;
                int16_t z = 0;
                archive >> x;
                archive >> y;
                archive >> z;
            }
            break;
        }

        archive >> bone.translation_size.x;
        archive >> bone.translation_size.y;
        archive >> bone.translation_size.z;
        archive >> bone.translation_init.x;
        archive >> bone.translation_init.y;
        archive >> bone.translation_init.z;

        metadata.bone_motions[bone_index] = std::move(bone);
    }

    return metadata;
}

std::optional<BindPoseSample> LoadLegacyBindPoseSample(const std::filesystem::path& ogf_path, const std::filesystem::path& omf_path)
{
    (void)omf_path;

    try
    {
        const auto data = LoadBinaryFile(ogf_path);
        const auto chunks = ParseChunks(data.data(), data.size());

        const auto bone_names_it = chunks.find(OGF_S_BONE_NAMES);
        if (bone_names_it == chunks.end())
            throw std::runtime_error("OGF missing bone names chunk: " + ogf_path.string());

        const auto ik_it = chunks.find(OGF_S_IKDATA);
        if (ik_it == chunks.end())
            throw std::runtime_error("OGF missing IK data chunk: " + ogf_path.string());

        auto bones = ReadBoneNames(bone_names_it->second);
        ReadIkData(ik_it->second, bones);
        ResolveHierarchy(bones);
        BuildLocalTransforms(bones);
        BuildGlobalTransforms(bones);

        BindPoseSample sample;
        sample.world_space_transforms.reserve(bones.size());
        for (const auto& bone : bones)
            sample.world_space_transforms.emplace(bone.name, bone.global_transform);

        return sample;
    }
    catch (const std::exception& ex)
    {
        ADD_FAILURE() << "Legacy bind-pose load failed: " << ex.what();
        return std::nullopt;
    }
}

std::optional<AnimationSample> LoadLegacyAnimationSample(const std::filesystem::path& ogf_path, const std::filesystem::path& omf_path,
    std::string_view motion_name, float sample_time_seconds)
{
    try
    {
        const auto ogf_data = LoadBinaryFile(ogf_path);
        const auto ogf_chunks = ParseChunks(ogf_data.data(), ogf_data.size());

        const auto bone_names_it = ogf_chunks.find(OGF_S_BONE_NAMES);
        if (bone_names_it == ogf_chunks.end())
            throw std::runtime_error("OGF missing bone names chunk for animation sampling");

        const auto ik_it = ogf_chunks.find(OGF_S_IKDATA);
        if (ik_it == ogf_chunks.end())
            throw std::runtime_error("OGF missing IK data chunk for animation sampling");

        auto bones = ReadBoneNames(bone_names_it->second);
        ReadIkData(ik_it->second, bones);
        ResolveHierarchy(bones);
        BuildLocalTransforms(bones);

        const auto omf_data = LoadBinaryFile(omf_path);
        const auto omf_chunks = ParseChunks(omf_data.data(), omf_data.size());
        LegacyAnimationData animation_data = ParseLegacyAnimationData(omf_chunks, bones);

        const std::string target = ToLowerCopy(motion_name);
        const LegacyMotion* motion = nullptr;
        for (const auto& candidate : animation_data.motions)
        {
            if (ToLowerCopy(candidate.name) == target)
            {
                motion = &candidate;
                break;
            }
        }

        if (!motion)
            throw std::runtime_error("motion not found in OMF: " + std::string(motion_name));

        if (motion->frame_count == 0)
            throw std::runtime_error("motion has zero frames: " + motion->name);

        const float clamped_time = std::max(0.0f, sample_time_seconds);
        const size_t frame_index = std::min(static_cast<size_t>(motion->frame_count - 1), static_cast<size_t>(clamped_time / SAMPLE_SPF));
        const float applied_time = static_cast<float>(frame_index) * SAMPLE_SPF;

        std::vector<Fmatrix> locals(bones.size());
        for (size_t idx = 0; idx < bones.size(); ++idx)
            locals[idx] = bones[idx].local_transform;

        for (size_t track_idx = 0; track_idx < animation_data.bone_remap.size(); ++track_idx)
        {
            const uint16_t bone_index = animation_data.bone_remap[track_idx];
            if (bone_index == std::numeric_limits<uint16_t>::max() || bone_index >= bones.size())
                continue;

            const LegacyAnimationTrack& track = motion->tracks[track_idx];
            if (frame_index >= track.rotations.size() || frame_index >= track.translations.size())
                continue;

            Fmatrix local;
            local.mk_xform(track.rotations[frame_index], track.translations[frame_index]);
            locals[bone_index] = local;
        }

        const std::vector<Fmatrix> world = BuildWorldTransformsFromLocals(bones, locals);

        AnimationSample sample;
        sample.world_space_transforms.reserve(bones.size());
        for (size_t idx = 0; idx < bones.size(); ++idx)
            sample.world_space_transforms.emplace(bones[idx].name, world[idx]);
        sample.applied_time_seconds = applied_time;

        return sample;
    }
    catch (const std::exception& ex)
    {
        ADD_FAILURE() << "Legacy animation sampling failed: " << ex.what();
        return std::nullopt;
    }
}

std::optional<AnimationSample> LoadOzzAnimationSample(const std::filesystem::path& skeleton_path, const std::filesystem::path& animation_path,
    std::string_view motion_name, float sample_time_seconds)
{
    try
    {
        ozz::animation::Skeleton skeleton;
        {
            ozz::io::File file(skeleton_path.string().c_str(), "rb");
            if (!file.opened())
                throw std::runtime_error("failed to open skeleton: " + skeleton_path.string());
            ozz::io::IArchive archive(&file);
            if (!archive.TestTag<ozz::animation::Skeleton>())
                throw std::runtime_error("file is not an ozz skeleton: " + skeleton_path.string());
            archive >> skeleton;
        }

        std::unique_ptr<ozz::animation::Animation> selected_animation;
        {
            ozz::io::File file(animation_path.string().c_str(), "rb");
            if (!file.opened())
                throw std::runtime_error("failed to open animation: " + animation_path.string());
            ozz::io::IArchive archive(&file);

            uint32_t animation_count = 0;
            archive >> animation_count;

            const std::string target = ToLowerCopy(motion_name);

            for (uint32_t index = 0; index < animation_count; ++index)
            {
                auto animation = std::make_unique<ozz::animation::Animation>();
                archive >> *animation;

                SerializedMotionMetadata metadata = ReadOzzMotionMetadata(archive);
                if (!selected_animation && ToLowerCopy(metadata.name) == target)
                    selected_animation = std::move(animation);
            }
        }

        if (!selected_animation)
            throw std::runtime_error("animation motion not found: " + std::string(motion_name));

        const float duration = selected_animation->duration();
        const float clamped_time = std::max(0.0f, sample_time_seconds);

        size_t frame_count = 1;
        if (duration > 0.f)
        {
            frame_count = static_cast<size_t>(std::round(duration / SAMPLE_SPF)) + 1;
            frame_count = std::max<size_t>(frame_count, 1);
        }

        const size_t frame_index = frame_count > 1 ? std::min(frame_count - 1, static_cast<size_t>(clamped_time / SAMPLE_SPF)) : 0;
        const float applied_time = static_cast<float>(frame_index) * SAMPLE_SPF;
        const float ratio = frame_count > 1 ? static_cast<float>(frame_index) / static_cast<float>(frame_count - 1) : 0.f;

        std::vector<ozz::math::SoaTransform> locals(static_cast<size_t>(skeleton.num_soa_joints()));
        std::vector<ozz::math::Float4x4> models(static_cast<size_t>(skeleton.num_joints()));

        ozz::animation::SamplingJob::Context context;
        context.Resize(selected_animation->num_tracks());

        ozz::animation::SamplingJob sampling_job;
        sampling_job.animation = selected_animation.get();
        sampling_job.context = &context;
        sampling_job.ratio = ratio;
        sampling_job.output = ozz::span<ozz::math::SoaTransform>(locals.data(), locals.size());
        if (!sampling_job.Run())
            throw std::runtime_error("ozz sampling job failed");

        ozz::animation::LocalToModelJob ltm_job;
        ltm_job.skeleton = &skeleton;
        ltm_job.input = ozz::span<const ozz::math::SoaTransform>(locals.data(), locals.size());
        ltm_job.output = ozz::span<ozz::math::Float4x4>(models.data(), models.size());
        if (!ltm_job.Run())
            throw std::runtime_error("ozz local-to-model job failed");

        AnimationSample sample;
        const auto joint_names = skeleton.joint_names();
        for (int joint = 0; joint < skeleton.num_joints(); ++joint)
        {
            const size_t index = static_cast<size_t>(joint);
            const char* name = (index < joint_names.size()) ? joint_names[index] : nullptr;
            if (!name || !name[0])
                continue;

            sample.world_space_transforms.emplace(name, ConvertOzzMatrixToXRay(models[index]));
        }
        sample.applied_time_seconds = applied_time;

        return sample;
    }
    catch (const std::exception& ex)
    {
        ADD_FAILURE() << "Ozz animation sampling failed: " << ex.what();
        return std::nullopt;
    }
}

std::optional<BindPoseSample> LoadOzzBindPoseSample(const std::filesystem::path& skeleton_path)
{
    if (!std::filesystem::exists(skeleton_path))
        return std::nullopt;

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
        return std::nullopt;

    kinematics.CalculateBones(TRUE);

    BindPoseSample sample;
    const u16 bone_count = kinematics.LL_BoneCount();
    sample.world_space_transforms.reserve(bone_count);

    for (u16 bone = 0; bone < bone_count; ++bone)
    {
        const char* bone_name = kinematics.LL_BoneName_dbg(bone);
        if (!bone_name || !bone_name[0])
            continue;

        Fmatrix transform = kinematics.LL_GetTransform(bone);
        sample.world_space_transforms.emplace(std::string(bone_name), transform);
    }

    return sample;
}

ozz::math::Transform ExtractTransformLane(const ozz::math::SoaTransform& soa, int lane)
{
    R_ASSERT(lane >= 0 && lane < 4);

    ozz::math::SimdFloat4 translations[4];
    ozz::math::Transpose3x4(&soa.translation.x, translations);

    ozz::math::SimdFloat4 rotations[4];
    ozz::math::Transpose4x4(&soa.rotation.x, rotations);

    ozz::math::SimdFloat4 scales[4];
    ozz::math::Transpose3x4(&soa.scale.x, scales);

    ozz::math::Transform result;
    ozz::math::Store3PtrU(translations[lane], &result.translation.x);
    ozz::math::StorePtrU(rotations[lane], &result.rotation.x);
    ozz::math::Store3PtrU(scales[lane], &result.scale.x);
    return result;
}
} // namespace

using XRay::Animation::OzzKinematics;

bool TestOzzKinematicsBootstrapMatchesJointCountWithReferenceSkeleton()
{
    const std::filesystem::path skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(skeleton_path))
    {
        ADD_FAILURE() << "Missing sample skeleton at " << skeleton_path;
        return false;
    }

    ozz::animation::Skeleton reference_skeleton;
    {
        ozz::io::File file(skeleton_path.string().c_str(), "rb");
        if (!file.opened())
        {
            ADD_FAILURE() << "Failed to open " << skeleton_path;
            return false;
        }
        ozz::io::IArchive archive(&file);
        archive >> reference_skeleton;
    }

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "OzzKinematics failed to load skeleton";
        return false;
    }

    const int expected = reference_skeleton.num_joints();
    const int actual = kinematics.LL_BoneCount();
    EXPECT_EQ(expected, actual) << "Loaded joint count mismatch";
    return expected == actual;
}

bool TestOzzKinematicsBootstrapInitializesFromOzzxBundleSkeleton()
{
    const std::filesystem::path bundle_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero.ozzx");
    if (!std::filesystem::exists(bundle_path))
    {
        ADD_FAILURE() << "Missing sample bundle at " << bundle_path;
        return false;
    }

    XRay::Animation::OzzxBundle bundle;
    if (!XRay::Animation::ReadOzzxBundle(bundle_path, bundle))
    {
        ADD_FAILURE() << "Failed to read bundle at " << bundle_path;
        return false;
    }

    if (bundle.skeleton.empty())
    {
        ADD_FAILURE() << "Bundle missing skeleton payload";
        return false;
    }

    const std::filesystem::path reference_skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(reference_skeleton_path))
    {
        ADD_FAILURE() << "Missing reference skeleton at " << reference_skeleton_path;
        return false;
    }

    ozz::animation::Skeleton reference_skeleton;
    {
        ozz::io::File file(reference_skeleton_path.string().c_str(), "rb");
        if (!file.opened())
        {
            ADD_FAILURE() << "Failed to open " << reference_skeleton_path;
            return false;
        }
        ozz::io::IArchive archive(&file);
        archive >> reference_skeleton;
    }

    OzzKinematics kinematics;
    const ozz::span<const std::byte> skeleton_span(reinterpret_cast<const std::byte*>(bundle.skeleton.data()), bundle.skeleton.size());
    if (!kinematics.InitializeFromOzzBuffer(skeleton_span))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematics from bundle skeleton payload";
        return false;
    }

    const int expected = reference_skeleton.num_joints();
    const int actual = kinematics.LL_BoneCount();
    EXPECT_EQ(expected, actual);
    return expected == actual;
}

bool TestOzzKinematicsBootstrapInitializesFromMemoryBuffer()
{
    const std::filesystem::path skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(skeleton_path))
    {
        ADD_FAILURE() << "Missing sample skeleton at " << skeleton_path;
        return false;
    }

    const auto skeleton_bytes = LoadBinaryFile(skeleton_path);
    if (skeleton_bytes.empty())
    {
        ADD_FAILURE() << "Skeleton payload unexpectedly empty";
        return false;
    }

    ozz::animation::Skeleton reference_skeleton;
    {
        ozz::io::File file(skeleton_path.string().c_str(), "rb");
        if (!file.opened())
        {
            ADD_FAILURE() << "Failed to open " << skeleton_path;
            return false;
        }
        ozz::io::IArchive archive(&file);
        archive >> reference_skeleton;
    }

    OzzKinematics file_initialized;
    if (!file_initialized.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematics from file";
        return false;
    }

    OzzKinematics buffer_initialized;
    const ozz::span<const std::byte> skeleton_span(skeleton_bytes.data(), skeleton_bytes.size());
    if (!buffer_initialized.InitializeFromOzzBuffer(skeleton_span))
    {
        ADD_FAILURE() << "Failed to initialize from memory buffer";
        return false;
    }

    bool ok = true;

    const int expected_joints = reference_skeleton.num_joints();
    const int buffer_joints = buffer_initialized.LL_BoneCount();
    EXPECT_EQ(expected_joints, buffer_joints);
    if (expected_joints != buffer_joints)
        ok = false;

    const int file_joints = file_initialized.LL_BoneCount();
    EXPECT_EQ(file_joints, buffer_joints);
    if (file_joints != buffer_joints)
        ok = false;

    if (buffer_joints <= 0)
    {
        ADD_FAILURE() << "Buffer-initialized skeleton reported zero bones";
        return false;
    }

    const u16 sample_bone = 0;
    EXPECT_STREQ(file_initialized.LL_BoneName_dbg(sample_bone), buffer_initialized.LL_BoneName_dbg(sample_bone));
    if (std::strcmp(file_initialized.LL_BoneName_dbg(sample_bone), buffer_initialized.LL_BoneName_dbg(sample_bone)) != 0)
        ok = false;

    const bool visible = buffer_initialized.LL_GetBoneVisible(sample_bone);
    EXPECT_TRUE(visible);
    if (!visible)
        ok = false;

    return ok;
}

bool TestOzzKinematicsBootstrapBoneNameLookupsAndVisibilityDefaults()
{
    const std::filesystem::path skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(skeleton_path))
    {
        ADD_FAILURE() << "Missing sample skeleton at " << skeleton_path;
        return false;
    }

    ozz::animation::Skeleton reference_skeleton;
    {
        ozz::io::File file(skeleton_path.string().c_str(), "rb");
        if (!file.opened())
        {
            ADD_FAILURE() << "Failed to open " << skeleton_path;
            return false;
        }
        ozz::io::IArchive archive(&file);
        archive >> reference_skeleton;
    }

    const int joint_count = reference_skeleton.num_joints();
    if (joint_count <= 0)
    {
        ADD_FAILURE() << "Reference skeleton has no joints";
        return false;
    }

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "OzzKinematics failed to load skeleton";
        return false;
    }

    bool ok = true;

    EXPECT_EQ(joint_count, kinematics.LL_BoneCount());
    if (joint_count != kinematics.LL_BoneCount())
        ok = false;

    const auto joint_names = reference_skeleton.joint_names();
    for (int joint = 0; joint < joint_count; ++joint)
    {
        const char* joint_name = (static_cast<size_t>(joint) < joint_names.size()) ? joint_names[joint] : nullptr;
        const std::string_view name_view = joint_name ? std::string_view(joint_name) : std::string_view();

        if (!name_view.empty())
        {
            const u16 expected_id = static_cast<u16>(joint);

            EXPECT_EQ(expected_id, kinematics.LL_BoneID(joint_name)) << "Name lookup mismatch for joint " << joint << " (" << joint_name << ")";
            if (expected_id != kinematics.LL_BoneID(joint_name))
                ok = false;

            const shared_str shared_name(joint_name);
            EXPECT_EQ(expected_id, kinematics.LL_BoneID(shared_name)) << "shared_str lookup mismatch for joint " << joint << " (" << joint_name << ")";
            if (expected_id != kinematics.LL_BoneID(shared_name))
                ok = false;

            EXPECT_STREQ(joint_name, kinematics.LL_BoneName_dbg(expected_id)) << "Debug name mismatch for bone " << joint;
            if (std::strcmp(joint_name, kinematics.LL_BoneName_dbg(expected_id)) != 0)
                ok = false;
        }
    }

    const u16 visible_limit = std::min<u16>(kinematics.LL_BoneCount(), 64);
    for (u16 bone = 0; bone < visible_limit; ++bone)
    {
        const bool visible = kinematics.LL_GetBoneVisible(bone);
        EXPECT_TRUE(visible) << "Bone " << bone << " should be visible by default";
        if (!visible)
            ok = false;
    }

    const u64 mask = kinematics.LL_GetBonesVisible();
    if (kinematics.LL_BoneCount() == 0)
    {
        EXPECT_EQ(0u, mask);
        if (mask != 0u)
            ok = false;
    }
    else if (kinematics.LL_BoneCount() >= 64)
    {
        EXPECT_EQ(u64(-1), mask);
        if (mask != u64(-1))
            ok = false;
    }
    else
    {
        const u64 expected_mask = (u64(1) << kinematics.LL_BoneCount()) - 1;
        EXPECT_EQ(expected_mask, mask);
        if (expected_mask != mask)
            ok = false;
    }

    return ok;
}

bool TestOzzKinematicsPoseMatchesLegacyBindPoseTranslations()
{
    const auto skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    const auto ogf_path = ResolveProjectPath("res/testdata/npc/stalker_hero_1.ogf");
    const auto omf_path = ResolveProjectPath("res/testdata/npc/critical_hit_grup_1.omf");

    if (!std::filesystem::exists(skeleton_path) || !std::filesystem::exists(ogf_path) || !std::filesystem::exists(omf_path))
    {
        ADD_FAILURE() << "Missing bind pose prerequisites";
        return false;
    }

    const auto legacy_sample = LoadLegacyBindPoseSample(ogf_path, omf_path);
    if (!legacy_sample.has_value())
    {
        ADD_FAILURE() << "Legacy bind-pose sample unavailable";
        return false;
    }

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematics from skeleton";
        return false;
    }
    kinematics.CalculateBones(TRUE);

    const size_t legacy_count = legacy_sample->world_space_transforms.size();
    const size_t ozz_count = static_cast<size_t>(kinematics.LL_BoneCount());
    if (legacy_count != ozz_count)
    {
        ADD_FAILURE() << "Bone count mismatch between legacy sample and OzzKinematics";
        return false;
    }

    bool ok = true;

    for (const auto& [bone_name, expected] : legacy_sample->world_space_transforms)
    {
        const u16 bone_id = kinematics.LL_BoneID(bone_name.c_str());
        if (bone_id == BI_NONE)
        {
            ADD_FAILURE() << "OzzKinematics missing bone: " << bone_name;
            ok = false;
            continue;
        }

        const Fmatrix& transform = kinematics.LL_GetTransform(bone_id);

        SCOPED_TRACE(::testing::Message() << "bone=" << bone_name);

        EXPECT_NEAR(transform.c.x, expected.c.x, 1e-4f) << "Bone: " << bone_name;
        if (std::fabs(transform.c.x - expected.c.x) > 1e-4f)
            ok = false;

        EXPECT_NEAR(transform.c.y, expected.c.y, 1e-4f) << "Bone: " << bone_name;
        if (std::fabs(transform.c.y - expected.c.y) > 1e-4f)
            ok = false;

        EXPECT_NEAR(transform.c.z, expected.c.z, 1e-4f) << "Bone: " << bone_name;
        if (std::fabs(transform.c.z - expected.c.z) > 1e-4f)
            ok = false;
    }

    return ok;
}

bool TestOzzKinematicsPoseSetPoseLocalsOverridesSingleBone()
{
    const auto skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(skeleton_path))
    {
        ADD_FAILURE() << "Missing sample skeleton";
        return false;
    }

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematics";
        return false;
    }

    kinematics.CalculateBones(TRUE);

    constexpr u16 target_bone = 0;
    if (target_bone >= kinematics.LL_BoneCount())
    {
        ADD_FAILURE() << "Target bone index out of range";
        return false;
    }

    const Fmatrix baseline_target = kinematics.LL_GetTransform(target_bone);

    const ozz::animation::Skeleton& skeleton = kinematics.Skeleton();
    const auto rest_span = skeleton.joint_rest_poses();
    std::vector<ozz::math::SoaTransform> locals(rest_span.begin(), rest_span.end());

    const int soa_index = static_cast<int>(target_bone / 4);
    const int lane_index = static_cast<int>(target_bone % 4);
    if (soa_index >= static_cast<int>(locals.size()))
    {
        ADD_FAILURE() << "SOA index out of range";
        return false;
    }

    const Fvector delta{ 0.05f, -0.03f, 0.f };
    const float delta_ozz_x = -delta.x;
    const float delta_ozz_y = delta.y;
    const float delta_ozz_z = -delta.z;
    ozz::math::SoaFloat3 soa_delta = ozz::math::SoaFloat3::zero();
    soa_delta.x = ozz::math::SetI(soa_delta.x, ozz::math::simd_float4::Load1(delta_ozz_x), lane_index);
    soa_delta.y = ozz::math::SetI(soa_delta.y, ozz::math::simd_float4::Load1(delta_ozz_y), lane_index);
    soa_delta.z = ozz::math::SetI(soa_delta.z, ozz::math::simd_float4::Load1(delta_ozz_z), lane_index);

    locals[static_cast<size_t>(soa_index)].translation = locals[static_cast<size_t>(soa_index)].translation + soa_delta;

    if (!kinematics.SetPoseLocals(ozz::span<const ozz::math::SoaTransform>(locals.data(), locals.size())))
    {
        ADD_FAILURE() << "SetPoseLocals failed";
        return false;
    }
    kinematics.CalculateBones(TRUE);

    bool ok = true;

    const Fmatrix sampled_target = kinematics.LL_GetTransform(target_bone);
    EXPECT_NEAR(baseline_target.c.x + delta.x, sampled_target.c.x, 1e-4f);
    if (std::fabs((baseline_target.c.x + delta.x) - sampled_target.c.x) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_target.c.y + delta.y, sampled_target.c.y, 1e-4f);
    if (std::fabs((baseline_target.c.y + delta.y) - sampled_target.c.y) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_target.c.z + delta.z, sampled_target.c.z, 1e-4f);
    if (std::fabs((baseline_target.c.z + delta.z) - sampled_target.c.z) > 1e-4f)
        ok = false;

    kinematics.ClearPose();
    kinematics.CalculateBones(TRUE);

    const Fmatrix restored_target = kinematics.LL_GetTransform(target_bone);
    EXPECT_NEAR(baseline_target.c.x, restored_target.c.x, 1e-4f);
    if (std::fabs(baseline_target.c.x - restored_target.c.x) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_target.c.y, restored_target.c.y, 1e-4f);
    if (std::fabs(baseline_target.c.y - restored_target.c.y) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_target.c.z, restored_target.c.z, 1e-4f);
    if (std::fabs(baseline_target.c.z - restored_target.c.z) > 1e-4f)
        ok = false;

    return ok;
}

bool TestOzzKinematicsPoseAdditionalBoneTransformsAffectSingleBone()
{
    const auto skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(skeleton_path))
    {
        ADD_FAILURE() << "Missing sample skeleton";
        return false;
    }

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematics";
        return false;
    }

    kinematics.CalculateBones(TRUE);

    constexpr u16 sample_bone = 0;
    if (sample_bone >= kinematics.LL_BoneCount())
    {
        ADD_FAILURE() << "Sample bone index out of range";
        return false;
    }

    const Fmatrix baseline_transform = kinematics.LL_GetTransform(sample_bone);
    const Fvector baseline_translation = baseline_transform.c;

    KinematicsABT::additional_bone_transform offset;
    offset.m_bone_id = sample_bone;

    const float offset_x = 0.1f;
    const float offset_y = -0.05f;
    const float offset_z = 0.02f;
    offset.setPosOffset(offset_x, offset_y, offset_z);

    kinematics.LL_AddTransformToBone(offset);
    kinematics.CalculateBones(TRUE);

    bool ok = true;

    const Fmatrix offset_transform = kinematics.LL_GetTransform(sample_bone);
    EXPECT_NEAR(baseline_translation.x + offset_x, offset_transform.c.x, 1e-4f);
    if (std::fabs((baseline_translation.x + offset_x) - offset_transform.c.x) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_translation.y + offset_y, offset_transform.c.y, 1e-4f);
    if (std::fabs((baseline_translation.y + offset_y) - offset_transform.c.y) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_translation.z + offset_z, offset_transform.c.z, 1e-4f);
    if (std::fabs((baseline_translation.z + offset_z) - offset_transform.c.z) > 1e-4f)
        ok = false;

    kinematics.LL_ClearAdditionalTransform(sample_bone);
    kinematics.CalculateBones(TRUE);

    const Fmatrix restored_transform = kinematics.LL_GetTransform(sample_bone);
    EXPECT_NEAR(baseline_translation.x, restored_transform.c.x, 1e-4f);
    if (std::fabs(baseline_translation.x - restored_transform.c.x) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_translation.y, restored_transform.c.y, 1e-4f);
    if (std::fabs(baseline_translation.y - restored_transform.c.y) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_translation.z, restored_transform.c.z, 1e-4f);
    if (std::fabs(baseline_translation.z - restored_transform.c.z) > 1e-4f)
        ok = false;

    return ok;
}

bool TestOzzKinematicsPoseBuildsSkinningPaletteMatchesTransforms()
{
    const auto skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(skeleton_path))
    {
        ADD_FAILURE() << "Missing sample skeleton";
        return false;
    }

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematics";
        return false;
    }

    kinematics.CalculateBones(TRUE);

    xr_vector<Fmatrix> local_palette;
    xr_vector<Fmatrix> render_palette;
    kinematics.BuildSkinningPalette(local_palette, false);
    kinematics.BuildSkinningPalette(render_palette, true);

    const u16 bone_count = kinematics.LL_BoneCount();
    if (static_cast<size_t>(bone_count) != local_palette.size() || static_cast<size_t>(bone_count) != render_palette.size())
    {
        ADD_FAILURE() << "Palette sizes mismatch";
        return false;
    }

    bool ok = true;

    for (u16 bone = 0; bone < bone_count; ++bone)
    {
        const Fmatrix& local = kinematics.LL_GetTransform(bone);
        const Fmatrix& render = kinematics.LL_GetTransform_R(bone);

        SCOPED_TRACE(::testing::Message() << "bone=" << bone);

        auto compare_component = [&](float expected, float actual)
        {
            EXPECT_NEAR(expected, actual, 1e-5f);
            if (std::fabs(expected - actual) > 1e-5f)
                ok = false;
        };

        compare_component(local.i.x, local_palette[bone].i.x);
        compare_component(local.i.y, local_palette[bone].i.y);
        compare_component(local.i.z, local_palette[bone].i.z);
        compare_component(local.j.x, local_palette[bone].j.x);
        compare_component(local.j.y, local_palette[bone].j.y);
        compare_component(local.j.z, local_palette[bone].j.z);
        compare_component(local.k.x, local_palette[bone].k.x);
        compare_component(local.k.y, local_palette[bone].k.y);
        compare_component(local.k.z, local_palette[bone].k.z);
        compare_component(local.c.x, local_palette[bone].c.x);
        compare_component(local.c.y, local_palette[bone].c.y);
        compare_component(local.c.z, local_palette[bone].c.z);

        compare_component(render.i.x, render_palette[bone].i.x);
        compare_component(render.i.y, render_palette[bone].i.y);
        compare_component(render.i.z, render_palette[bone].i.z);
        compare_component(render.j.x, render_palette[bone].j.x);
        compare_component(render.j.y, render_palette[bone].j.y);
        compare_component(render.j.z, render_palette[bone].j.z);
        compare_component(render.k.x, render_palette[bone].k.x);
        compare_component(render.k.y, render_palette[bone].k.y);
        compare_component(render.k.z, render_palette[bone].k.z);
        compare_component(render.c.x, render_palette[bone].c.x);
        compare_component(render.c.y, render_palette[bone].c.y);
        compare_component(render.c.z, render_palette[bone].c.z);
    }

    return ok;
}

bool TestOzzKinematicsVisibilityBoneVisibilityToggleZeroesTransforms()
{
    const auto skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(skeleton_path))
    {
        ADD_FAILURE() << "Missing sample skeleton";
        return false;
    }

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematics";
        return false;
    }

    kinematics.CalculateBones(TRUE);

    constexpr u16 sample_bone = 0;
    if (sample_bone >= kinematics.LL_BoneCount())
    {
        ADD_FAILURE() << "Sample bone index out of range";
        return false;
    }

    const Fmatrix baseline = kinematics.LL_GetTransform(sample_bone);

    auto expect_translation = [](const Fmatrix& transform, const Fvector& expected, float epsilon, bool& ok)
    {
        EXPECT_NEAR(expected.x, transform.c.x, epsilon);
        EXPECT_NEAR(expected.y, transform.c.y, epsilon);
        EXPECT_NEAR(expected.z, transform.c.z, epsilon);
        if (std::fabs(expected.x - transform.c.x) > epsilon || std::fabs(expected.y - transform.c.y) > epsilon || std::fabs(expected.z - transform.c.z) > epsilon)
        {
            ok = false;
        }
    };

    bool ok = true;
    Fvector baseline_translation = baseline.c;

    kinematics.LL_SetBoneVisible(sample_bone, FALSE, FALSE);
    kinematics.CalculateBones(TRUE);

    const Fmatrix hidden = kinematics.LL_GetTransform(sample_bone);
    expect_translation(hidden, { 0.f, 0.f, 0.f }, 1e-5f, ok);

    Fmatrix pre_callback{};
    kinematics.Bone_GetAnimPos(pre_callback, sample_bone, 0, TRUE);
    expect_translation(pre_callback, { 0.f, 0.f, 0.f }, 1e-5f, ok);

    kinematics.LL_SetBoneVisible(sample_bone, TRUE, FALSE);
    kinematics.CalculateBones(TRUE);

    const Fmatrix restored = kinematics.LL_GetTransform(sample_bone);
    expect_translation(restored, baseline_translation, 1e-4f, ok);

    return ok;
}

bool TestOzzKinematicsVisibilitySetBonesVisibleControlsMask()
{
    const auto skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(skeleton_path))
    {
        ADD_FAILURE() << "Missing sample skeleton";
        return false;
    }

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematics";
        return false;
    }

    kinematics.CalculateBones(TRUE);

    constexpr u16 first_hidden_bone = 0;
    constexpr u16 second_hidden_bone = 1;
    constexpr u16 survivor_bone = 2;
    const u16 bone_count = kinematics.LL_BoneCount();
    if (second_hidden_bone >= bone_count || survivor_bone >= bone_count)
    {
        ADD_FAILURE() << "Visibility mask test requires at least three bones";
        return false;
    }

    const Fmatrix baseline_first = kinematics.LL_GetTransform(first_hidden_bone);
    const Fmatrix baseline_second = kinematics.LL_GetTransform(second_hidden_bone);
    const Fmatrix baseline_survivor = kinematics.LL_GetTransform(survivor_bone);

    const u64 full_mask = bone_count >= 64 ? u64(-1) : ((u64(1) << bone_count) - 1);
    const u64 hidden_mask = full_mask & ~(u64(1) << first_hidden_bone) & ~(u64(1) << second_hidden_bone);

    kinematics.LL_SetBonesVisible(hidden_mask);
    kinematics.CalculateBones(TRUE);

    bool ok = true;

    EXPECT_EQ(hidden_mask, kinematics.LL_GetBonesVisible());
    if (hidden_mask != kinematics.LL_GetBonesVisible())
        ok = false;

    const u16 expected_visible = static_cast<u16>(bone_count - 2);
    EXPECT_EQ(expected_visible, kinematics.LL_VisibleBoneCount());
    if (expected_visible != kinematics.LL_VisibleBoneCount())
        ok = false;

    auto expect_zero_translation = [&](const Fmatrix& transform)
    {
        EXPECT_NEAR(0.f, transform.c.x, 1e-5f);
        EXPECT_NEAR(0.f, transform.c.y, 1e-5f);
        EXPECT_NEAR(0.f, transform.c.z, 1e-5f);
        if (std::fabs(transform.c.x) > 1e-5f || std::fabs(transform.c.y) > 1e-5f || std::fabs(transform.c.z) > 1e-5f)
            ok = false;
    };

    expect_zero_translation(kinematics.LL_GetTransform(first_hidden_bone));
    expect_zero_translation(kinematics.LL_GetTransform(second_hidden_bone));

    const Fmatrix survivor_visible = kinematics.LL_GetTransform(survivor_bone);
    EXPECT_NEAR(baseline_survivor.c.x, survivor_visible.c.x, 1e-4f);
    if (std::fabs(baseline_survivor.c.x - survivor_visible.c.x) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_survivor.c.y, survivor_visible.c.y, 1e-4f);
    if (std::fabs(baseline_survivor.c.y - survivor_visible.c.y) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_survivor.c.z, survivor_visible.c.z, 1e-4f);
    if (std::fabs(baseline_survivor.c.z - survivor_visible.c.z) > 1e-4f)
        ok = false;

    kinematics.LL_SetBonesVisible(full_mask);
    kinematics.CalculateBones(TRUE);

    EXPECT_EQ(full_mask, kinematics.LL_GetBonesVisible());
    if (full_mask != kinematics.LL_GetBonesVisible())
        ok = false;

    const Fmatrix restored_first = kinematics.LL_GetTransform(first_hidden_bone);
    EXPECT_NEAR(baseline_first.c.x, restored_first.c.x, 1e-4f);
    if (std::fabs(baseline_first.c.x - restored_first.c.x) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_first.c.y, restored_first.c.y, 1e-4f);
    if (std::fabs(baseline_first.c.y - restored_first.c.y) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_first.c.z, restored_first.c.z, 1e-4f);
    if (std::fabs(baseline_first.c.z - restored_first.c.z) > 1e-4f)
        ok = false;

    const Fmatrix restored_second = kinematics.LL_GetTransform(second_hidden_bone);
    EXPECT_NEAR(baseline_second.c.x, restored_second.c.x, 1e-4f);
    if (std::fabs(baseline_second.c.x - restored_second.c.x) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_second.c.y, restored_second.c.y, 1e-4f);
    if (std::fabs(baseline_second.c.y - restored_second.c.y) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline_second.c.z, restored_second.c.z, 1e-4f);
    if (std::fabs(baseline_second.c.z - restored_second.c.z) > 1e-4f)
        ok = false;

    return ok;
}

bool TestOzzKinematicsAnimatedLoadsStandaloneClip()
{
    const auto skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    const auto animation_path = ResolveProjectPath("src/xrAnimation/tests/testdata/critical_hit_grup_1_single.ozz");

    if (!std::filesystem::exists(skeleton_path) || !std::filesystem::exists(animation_path))
    {
        ADD_FAILURE() << "Missing animation playback test assets";
        return false;
    }

    XRay::Animation::OzzKinematicsAnimated kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematicsAnimated with skeleton";
        return false;
    }

    bool ok = true;

    EXPECT_FALSE(kinematics.HasLoadedAnimation());
    if (kinematics.HasLoadedAnimation())
        ok = false;
    EXPECT_FALSE(kinematics.AdvanceAnimation(0.016f));
    if (kinematics.AdvanceAnimation(0.016f))
        ok = false;

    if (!kinematics.LoadAnimationFromFile(animation_path))
    {
        ADD_FAILURE() << "Failed to load animation clip";
        return false;
    }

    EXPECT_TRUE(kinematics.HasLoadedAnimation());
    if (!kinematics.HasLoadedAnimation())
        ok = false;
    EXPECT_GT(kinematics.AnimationDuration(), 0.f);
    if (!(kinematics.AnimationDuration() > 0.f))
        ok = false;

    EXPECT_TRUE(kinematics.AdvanceAnimation(0.f));
    if (!kinematics.AdvanceAnimation(0.f))
        ok = false;
    kinematics.CalculateBones(TRUE);

    constexpr u16 root_bone = 0;
    const Fmatrix start_pose = kinematics.LL_GetTransform(root_bone);

    const float advance_time = std::max(0.0f, std::min(kinematics.AnimationDuration() * 0.25f, kinematics.AnimationDuration()));
    EXPECT_TRUE(kinematics.AdvanceAnimation(advance_time));
    if (!kinematics.AdvanceAnimation(advance_time))
        ok = false;
    kinematics.CalculateBones(TRUE);

    const Fmatrix mid_pose = kinematics.LL_GetTransform(root_bone);
    const float translation_delta =
        std::fabs(start_pose.c.x - mid_pose.c.x) + std::fabs(start_pose.c.y - mid_pose.c.y) + std::fabs(start_pose.c.z - mid_pose.c.z);

    EXPECT_GT(translation_delta, 1e-5f) << "Animation sampling did not update root transform";
    if (!(translation_delta > 1e-5f))
        ok = false;

    kinematics.SetLooping(false);
    EXPECT_TRUE(kinematics.AdvanceAnimation(kinematics.AnimationDuration() * 2.f));
    if (!kinematics.AdvanceAnimation(kinematics.AnimationDuration() * 2.f))
        ok = false;

    kinematics.StopAnimation();
    EXPECT_FALSE(kinematics.HasLoadedAnimation());
    if (kinematics.HasLoadedAnimation())
        ok = false;

    return ok;
}

bool TestOzzKinematicsCallbacksInvokeBoneCallbackAndHonorOverwrite()
{
    const auto skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    if (!std::filesystem::exists(skeleton_path))
    {
        ADD_FAILURE() << "Missing sample skeleton";
        return false;
    }

    OzzKinematics kinematics;
    if (!kinematics.InitializeFromOzz(skeleton_path.string().c_str()))
    {
        ADD_FAILURE() << "Failed to initialize OzzKinematics";
        return false;
    }

    kinematics.CalculateBones(TRUE);

    constexpr u16 callback_bone = 0;
    if (callback_bone >= kinematics.LL_BoneCount())
    {
        ADD_FAILURE() << "Callback bone index out of range";
        return false;
    }

    const Fmatrix baseline = kinematics.LL_GetTransform(callback_bone);

    TestBoneCallbackContext context;
    context.override_transform = baseline;
    const Fvector override_delta{ -0.12f, 0.08f, 0.04f };
    context.override_transform.c.add(override_delta);

    CBoneInstance& bone_instance = kinematics.LL_GetBoneInstance(callback_bone);
    bone_instance.set_callback(bctCustom, &TestBoneCallbackFunction, &context, TRUE);

    kinematics.CalculateBones(TRUE);

    bool ok = true;

    EXPECT_EQ(1, context.call_count);
    if (context.call_count != 1)
        ok = false;
    EXPECT_EQ(&bone_instance, context.last_instance);
    if (context.last_instance != &bone_instance)
        ok = false;

    Fmatrix pre_callback{};
    kinematics.Bone_GetAnimPos(pre_callback, callback_bone, 0, TRUE);
    EXPECT_NEAR(baseline.c.x, pre_callback.c.x, 1e-4f);
    if (std::fabs(baseline.c.x - pre_callback.c.x) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline.c.y, pre_callback.c.y, 1e-4f);
    if (std::fabs(baseline.c.y - pre_callback.c.y) > 1e-4f)
        ok = false;
    EXPECT_NEAR(baseline.c.z, pre_callback.c.z, 1e-4f);
    if (std::fabs(baseline.c.z - pre_callback.c.z) > 1e-4f)
        ok = false;

    const Fmatrix final_transform = kinematics.LL_GetTransform(callback_bone);
    EXPECT_NEAR(context.override_transform.c.x, final_transform.c.x, 1e-4f);
    if (std::fabs(context.override_transform.c.x - final_transform.c.x) > 1e-4f)
        ok = false;
    EXPECT_NEAR(context.override_transform.c.y, final_transform.c.y, 1e-4f);
    if (std::fabs(context.override_transform.c.y - final_transform.c.y) > 1e-4f)
        ok = false;
    EXPECT_NEAR(context.override_transform.c.z, final_transform.c.z, 1e-4f);
    if (std::fabs(context.override_transform.c.z - final_transform.c.z) > 1e-4f)
        ok = false;

    bone_instance.reset_callback();
    return ok;
}

bool TestOzzKinematicsParityBindPoseMatchesLegacySkeleton()
{
    const auto ogf_path = ResolveProjectPath("res/testdata/npc/stalker_hero_1.ogf");
    const auto omf_path = ResolveProjectPath("res/testdata/npc/critical_hit_grup_1.omf");
    const auto ozz_skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");

    if (!std::filesystem::exists(ogf_path) || !std::filesystem::exists(omf_path) || !std::filesystem::exists(ozz_skeleton_path))
    {
        ADD_FAILURE() << "Missing bind-pose parity assets";
        return false;
    }

    const auto legacy_sample = LoadLegacyBindPoseSample(ogf_path, omf_path);
    if (!legacy_sample.has_value())
    {
        ADD_FAILURE() << "Legacy bind-pose sampling failed for " << ogf_path.filename();
        return false;
    }

    const auto ozz_sample = LoadOzzBindPoseSample(ozz_skeleton_path);
    if (!ozz_sample.has_value())
    {
        ADD_FAILURE() << "Failed to load Ozz bind-pose sample from " << ozz_skeleton_path;
        return false;
    }

    if (legacy_sample->world_space_transforms.size() != ozz_sample->world_space_transforms.size())
    {
        ADD_FAILURE() << "Bone count mismatch between legacy and Ozz skeletons";
        return false;
    }

    bool ok = true;

    for (const auto& [bone_name, legacy_transform] : legacy_sample->world_space_transforms)
    {
        const auto ozz_iter = ozz_sample->world_space_transforms.find(bone_name);
        if (ozz_iter == ozz_sample->world_space_transforms.end())
        {
            ADD_FAILURE() << "Ozz skeleton missing bone " << bone_name;
            ok = false;
            continue;
        }

        const Fmatrix& ozz_transform = ozz_iter->second;

        EXPECT_NEAR(legacy_transform.c.x, ozz_transform.c.x, 1e-4f) << "Bind-pose X mismatch for bone " << bone_name;
        if (std::fabs(legacy_transform.c.x - ozz_transform.c.x) > 1e-4f)
            ok = false;
        EXPECT_NEAR(legacy_transform.c.y, ozz_transform.c.y, 1e-4f) << "Bind-pose Y mismatch for bone " << bone_name;
        if (std::fabs(legacy_transform.c.y - ozz_transform.c.y) > 1e-4f)
            ok = false;
        EXPECT_NEAR(legacy_transform.c.z, ozz_transform.c.z, 1e-4f) << "Bind-pose Z mismatch for bone " << bone_name;
        if (std::fabs(legacy_transform.c.z - ozz_transform.c.z) > 1e-4f)
            ok = false;
    }

    return ok;
}

bool TestOzzKinematicsParityAnimationPoseMatchesLegacySkeleton()
{
    const auto ogf_path = ResolveProjectPath("res/testdata/npc/stalker_hero_1.ogf");
    const auto omf_path = ResolveProjectPath("res/testdata/npc/critical_hit_grup_1.omf");
    const auto ozz_skeleton_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    const auto ozz_animation_path = ResolveProjectPath("src/xrAnimation/tests/testdata/critical_hit_grup_1.ozz");

    if (!std::filesystem::exists(ogf_path) ||
        !std::filesystem::exists(omf_path) ||
        !std::filesystem::exists(ozz_skeleton_path) ||
        !std::filesystem::exists(ozz_animation_path))
    {
        ADD_FAILURE() << "Missing animation parity assets";
        return false;
    }

    constexpr std::string_view kMotionName = "norm_2_critical_hit_hend_left_0";
    constexpr float kSampleTimeSeconds = 0.2f;

    const auto legacy_sample = LoadLegacyAnimationSample(ogf_path, omf_path, kMotionName, kSampleTimeSeconds);
    if (!legacy_sample.has_value())
    {
        ADD_FAILURE() << "Legacy animation sampling failed for " << kMotionName;
        return false;
    }

    const auto ozz_sample = LoadOzzAnimationSample(ozz_skeleton_path, ozz_animation_path, kMotionName, legacy_sample->applied_time_seconds);
    if (!ozz_sample.has_value())
    {
        ADD_FAILURE() << "Ozz animation sampling failed for " << kMotionName;
        return false;
    }

    if (legacy_sample->world_space_transforms.size() != ozz_sample->world_space_transforms.size())
    {
        ADD_FAILURE() << "Bone count mismatch between sampled poses";
        return false;
    }

    bool ok = true;
    constexpr float kAnimationTolerance = 5e-4f;

    for (const auto& [bone_name, legacy_transform] : legacy_sample->world_space_transforms)
    {
        const auto ozz_it = ozz_sample->world_space_transforms.find(bone_name);
        if (ozz_it == ozz_sample->world_space_transforms.end())
        {
            ADD_FAILURE() << "Ozz sample missing bone " << bone_name;
            ok = false;
            continue;
        }

        const Fmatrix& ozz_transform = ozz_it->second;

        const float dx = legacy_transform.c.x - ozz_transform.c.x;
        const float dy = legacy_transform.c.y - ozz_transform.c.y;
        const float dz = legacy_transform.c.z - ozz_transform.c.z;
        const float max_abs = std::max({ std::fabs(dx), std::fabs(dy), std::fabs(dz) });
        SCOPED_TRACE(::testing::Message() << "bone=" << bone_name << " legacy_t=" << legacy_sample->applied_time_seconds << " diff(x,y,z)=(" << dx << ", " << dy
                                          << ", " << dz << ") max=" << max_abs);

        EXPECT_NEAR(legacy_transform.c.x, ozz_transform.c.x, kAnimationTolerance) << "Animation X mismatch for bone " << bone_name;
        if (std::fabs(dx) > kAnimationTolerance)
            ok = false;
        EXPECT_NEAR(legacy_transform.c.y, ozz_transform.c.y, kAnimationTolerance) << "Animation Y mismatch for bone " << bone_name;
        if (std::fabs(dy) > kAnimationTolerance)
            ok = false;
        EXPECT_NEAR(legacy_transform.c.z, ozz_transform.c.z, kAnimationTolerance) << "Animation Z mismatch for bone " << bone_name;
        if (std::fabs(dz) > kAnimationTolerance)
            ok = false;
    }

    return ok;
}

bool TestOzzBundleRuntimeHydratesKinematicsAndMeshPayload()
{
    const auto bundle_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero.ozzx");
    if (!std::filesystem::exists(bundle_path))
    {
        ADD_FAILURE() << "Missing test bundle: " << bundle_path;
        return false;
    }

    XRay::Animation::OzzxBundle bundle;
    if (!XRay::Animation::ReadOzzxBundle(bundle_path, bundle))
    {
        ADD_FAILURE() << "Failed to read bundle";
        return false;
    }

    if (bundle.skeleton.empty())
    {
        ADD_FAILURE() << "Bundle missing skeleton payload";
        return false;
    }

    if (bundle.mesh.empty())
    {
        ADD_FAILURE() << "Bundle missing mesh payload";
        return false;
    }

    XRay::Animation::OzzKinematics kinematics;
    ozz::span<const std::byte> skeleton_span(reinterpret_cast<const std::byte*>(bundle.skeleton.data()), bundle.skeleton.size());
    if (!kinematics.InitializeFromOzzBuffer(skeleton_span))
    {
        ADD_FAILURE() << "Failed to initialize kinematics from bundle";
        return false;
    }

    bool ok = true;
    if (!(kinematics.LL_BoneCount() > 0))
    {
        ADD_FAILURE() << "Kinematics reported zero bones";
        ok = false;
    }

    kinematics.CalculateBones(TRUE);
    xr_vector<Fmatrix> palette;
    kinematics.BuildSkinningPalette(palette, true);
    EXPECT_EQ(palette.size(), kinematics.LL_BoneCount()) << "Palette bone count mismatch";
    if (palette.size() != static_cast<size_t>(kinematics.LL_BoneCount()))
        ok = false;

    ozz::io::MemoryStream mesh_stream;
    if (!mesh_stream.Write(bundle.mesh.data(), bundle.mesh.size()))
    {
        ADD_FAILURE() << "Failed to stage mesh payload";
        return false;
    }
    mesh_stream.Seek(0, ozz::io::Stream::kSet);

    ozz::io::IArchive archive(&mesh_stream);
    size_t mesh_count = 0;
    while (archive.TestTag<ozz::sample::Mesh>())
    {
        ozz::sample::Mesh mesh;
        archive >> mesh;
        EXPECT_GT(mesh.vertex_count(), 0);
        if (!(mesh.vertex_count() > 0))
            ok = false;
        EXPECT_FALSE(mesh.parts.empty());
        if (mesh.parts.empty())
            ok = false;
        ++mesh_count;
    }
    EXPECT_GT(mesh_count, 0u) << "Bundle contained no meshes";
    if (!(mesh_count > 0u))
        ok = false;

    return ok;
}

bool TestOzzKinematicsAppliesBoneMetadata(bool& metadata_available)
{
    metadata_available = false;
    const auto bundle_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero.ozzx");
    if (!std::filesystem::exists(bundle_path))
    {
        ADD_FAILURE() << "Missing test bundle: " << bundle_path;
        return false;
    }

    XRay::Animation::OzzxBundle bundle;
    if (!XRay::Animation::ReadOzzxBundle(bundle_path, bundle))
    {
        ADD_FAILURE() << "Failed to read bundle";
        return false;
    }

    if (bundle.skeleton.empty())
    {
        ADD_FAILURE() << "Bundle missing skeleton payload";
        return false;
    }

    if (bundle.bone_metadata.empty())
        return true;

    XRay::Animation::OzzKinematics kinematics;
    const ozz::span<const std::byte> skeleton_span(reinterpret_cast<const std::byte*>(bundle.skeleton.data()), bundle.skeleton.size());
    if (!kinematics.InitializeFromOzzBuffer(skeleton_span))
    {
        ADD_FAILURE() << "Failed to initialize kinematics from bundle";
        return false;
    }

    if (!kinematics.ApplyExtendedBoneMetadata(bundle.bone_metadata))
    {
        ADD_FAILURE() << "Failed to apply bone metadata";
        return false;
    }

    metadata_available = true;

    const size_t expected_count = bundle.bone_metadata.size();
    const int bone_count = kinematics.LL_BoneCount();
    EXPECT_EQ(expected_count, static_cast<size_t>(bone_count)) << "Bone metadata count mismatch";
    if (expected_count != static_cast<size_t>(bone_count))
        return false;

    constexpr float kEpsilon = 1e-4f;
    bool ok = true;
    for (size_t idx = 0; idx < expected_count; ++idx)
    {
        const auto& expected = bundle.bone_metadata[idx];
        const CBoneData& bone = kinematics.LL_GetData(static_cast<u16>(idx));
        const char* bone_name = bone.name.c_str();

        EXPECT_EQ(bone.shape.type, expected.shape.type) << "Shape type mismatch for bone " << bone_name;
        if (bone.shape.type != expected.shape.type)
            ok = false;

        if (std::memcmp(&bone.shape, &expected.shape, sizeof(SBoneShape)) != 0)
        {
            ADD_FAILURE() << "Shape payload mismatch for bone " << bone_name;
            ok = false;
        }

        if (std::memcmp(&bone.obb, &expected.obb, sizeof(Fobb)) != 0)
        {
            ADD_FAILURE() << "OBB payload mismatch for bone " << bone_name;
            ok = false;
        }

        EXPECT_NEAR(bone.mass, expected.mass, kEpsilon) << "Mass mismatch for bone " << bone_name;
        if (std::fabs(bone.mass - expected.mass) > kEpsilon)
            ok = false;

        EXPECT_NEAR(bone.rest_length, expected.rest_length, kEpsilon) << "Rest length mismatch for bone " << bone_name;
        if (std::fabs(bone.rest_length - expected.rest_length) > kEpsilon)
            ok = false;

        EXPECT_NEAR(bone.inertia_tensor.x, expected.inertia_tensor.x, kEpsilon) << "Inertia X mismatch for bone " << bone_name;
        if (std::fabs(bone.inertia_tensor.x - expected.inertia_tensor.x) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.inertia_tensor.y, expected.inertia_tensor.y, kEpsilon) << "Inertia Y mismatch for bone " << bone_name;
        if (std::fabs(bone.inertia_tensor.y - expected.inertia_tensor.y) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.inertia_tensor.z, expected.inertia_tensor.z, kEpsilon) << "Inertia Z mismatch for bone " << bone_name;
        if (std::fabs(bone.inertia_tensor.z - expected.inertia_tensor.z) > kEpsilon)
            ok = false;

        EXPECT_NEAR(bone.center_of_mass.x, expected.center_of_mass.x, kEpsilon) << "Center of mass X mismatch for bone " << bone_name;
        if (std::fabs(bone.center_of_mass.x - expected.center_of_mass.x) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.center_of_mass.y, expected.center_of_mass.y, kEpsilon) << "Center of mass Y mismatch for bone " << bone_name;
        if (std::fabs(bone.center_of_mass.y - expected.center_of_mass.y) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.center_of_mass.z, expected.center_of_mass.z, kEpsilon) << "Center of mass Z mismatch for bone " << bone_name;
        if (std::fabs(bone.center_of_mass.z - expected.center_of_mass.z) > kEpsilon)
            ok = false;

        EXPECT_NEAR(bone.dominant_axis.x, expected.dominant_axis.x, kEpsilon) << "Dominant axis X mismatch for bone " << bone_name;
        if (std::fabs(bone.dominant_axis.x - expected.dominant_axis.x) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.dominant_axis.y, expected.dominant_axis.y, kEpsilon) << "Dominant axis Y mismatch for bone " << bone_name;
        if (std::fabs(bone.dominant_axis.y - expected.dominant_axis.y) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.dominant_axis.z, expected.dominant_axis.z, kEpsilon) << "Dominant axis Z mismatch for bone " << bone_name;
        if (std::fabs(bone.dominant_axis.z - expected.dominant_axis.z) > kEpsilon)
            ok = false;

        EXPECT_NEAR(bone.local_aabb_min.x, expected.local_aabb_min.x, kEpsilon) << "Local AABB min X mismatch for bone " << bone_name;
        if (std::fabs(bone.local_aabb_min.x - expected.local_aabb_min.x) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.local_aabb_min.y, expected.local_aabb_min.y, kEpsilon) << "Local AABB min Y mismatch for bone " << bone_name;
        if (std::fabs(bone.local_aabb_min.y - expected.local_aabb_min.y) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.local_aabb_min.z, expected.local_aabb_min.z, kEpsilon) << "Local AABB min Z mismatch for bone " << bone_name;
        if (std::fabs(bone.local_aabb_min.z - expected.local_aabb_min.z) > kEpsilon)
            ok = false;

        EXPECT_NEAR(bone.local_aabb_max.x, expected.local_aabb_max.x, kEpsilon) << "Local AABB max X mismatch for bone " << bone_name;
        if (std::fabs(bone.local_aabb_max.x - expected.local_aabb_max.x) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.local_aabb_max.y, expected.local_aabb_max.y, kEpsilon) << "Local AABB max Y mismatch for bone " << bone_name;
        if (std::fabs(bone.local_aabb_max.y - expected.local_aabb_max.y) > kEpsilon)
            ok = false;
        EXPECT_NEAR(bone.local_aabb_max.z, expected.local_aabb_max.z, kEpsilon) << "Local AABB max Z mismatch for bone " << bone_name;
        if (std::fabs(bone.local_aabb_max.z - expected.local_aabb_max.z) > kEpsilon)
            ok = false;

        if (std::memcmp(&bone.inverse_global_transform, &expected.inverse_global_transform, sizeof(Fmatrix)) != 0)
        {
            ADD_FAILURE() << "Inverse global transform mismatch for bone " << bone_name;
            ok = false;
        }

        EXPECT_NEAR(bone.volume, expected.volume, kEpsilon) << "Volume mismatch for bone " << bone_name;
        if (std::fabs(bone.volume - expected.volume) > kEpsilon)
            ok = false;

        EXPECT_EQ(bone.collision_layers.get(), expected.collision_layers.get()) << "Collision layer mismatch for bone " << bone_name;
        if (bone.collision_layers.get() != expected.collision_layers.get())
            ok = false;

        EXPECT_EQ(bone.ground_contact_candidate, expected.ground_contact_candidate) << "Ground contact flag mismatch for bone " << bone_name;
        if (bone.ground_contact_candidate != expected.ground_contact_candidate)
            ok = false;

        EXPECT_EQ(bone.weapon_anchor_candidate, expected.weapon_anchor_candidate) << "Weapon anchor flag mismatch for bone " << bone_name;
        if (bone.weapon_anchor_candidate != expected.weapon_anchor_candidate)
            ok = false;

        EXPECT_STREQ(bone.game_mtl_name.c_str(), expected.game_material.c_str()) << "Material mismatch for bone " << bone_name;
        if (std::strcmp(bone.game_mtl_name.c_str(), expected.game_material.c_str()) != 0)
            ok = false;
    }

    return ok;
}

bool TestModelNamingNormalizesModelIdentifiers()
{
    using xray::render::detail::NormalizeModelIdentifier;

    bool ok = true;

    if (NormalizeModelIdentifier("actors\\stalker") != "actors\\stalker")
    {
        ADD_FAILURE() << "Unexpected normalization for actors\\stalker";
        ok = false;
    }

    if (NormalizeModelIdentifier("actors\\stalker.ogf") != "actors\\stalker")
    {
        ADD_FAILURE() << ".ogf suffix not stripped";
        ok = false;
    }

    if (NormalizeModelIdentifier("actors\\DEV_STALKER.OGF") != "actors\\dev_stalker")
    {
        ADD_FAILURE() << "Uppercase .ogf normalization failed";
        ok = false;
    }

    if (NormalizeModelIdentifier("actors\\dev_stalker.ozzx") != "actors\\dev_stalker.ozzx")
    {
        ADD_FAILURE() << ".ozzx suffix altered unexpectedly";
        ok = false;
    }

    if (NormalizeModelIdentifier("actors\\DEV_STALKER.OZZX") != "actors\\dev_stalker.ozzx")
    {
        ADD_FAILURE() << "Uppercase .ozzx normalization failed";
        ok = false;
    }

    if (NormalizeModelIdentifier("actors\\some_visual.dds") != "actors\\some_visual")
    {
        ADD_FAILURE() << "Texture suffix not stripped";
        ok = false;
    }

    if (NormalizeModelIdentifier("weapon") != "weapon")
    {
        ADD_FAILURE() << "Identifier without suffix changed";
        ok = false;
    }

    if (!NormalizeModelIdentifier(nullptr).empty())
    {
        ADD_FAILURE() << "Null pointer should yield empty string";
        ok = false;
    }

    if (!NormalizeModelIdentifier("").empty())
    {
        ADD_FAILURE() << "Empty identifier should remain empty";
        ok = false;
    }

    return ok;
}

TEST(OzzKinematicsBootstrap, MatchesJointCountWithReferenceSkeleton)
{
    EXPECT_TRUE(TestOzzKinematicsBootstrapMatchesJointCountWithReferenceSkeleton());
}

TEST(OzzKinematicsBootstrap, InitializesFromOzzxBundleSkeleton)
{
    EXPECT_TRUE(TestOzzKinematicsBootstrapInitializesFromOzzxBundleSkeleton());
}

TEST(OzzKinematicsBootstrap, InitializesFromMemoryBuffer)
{
    EXPECT_TRUE(TestOzzKinematicsBootstrapInitializesFromMemoryBuffer());
}

TEST(OzzKinematicsBootstrap, BoneNameLookupsAndVisibilityDefaults)
{
    EXPECT_TRUE(TestOzzKinematicsBootstrapBoneNameLookupsAndVisibilityDefaults());
}

TEST(OzzKinematicsPose, MatchesLegacyBindPoseTranslations)
{
    EXPECT_TRUE(TestOzzKinematicsPoseMatchesLegacyBindPoseTranslations());
}

TEST(OzzKinematicsPose, SetPoseLocalsOverridesSingleBone)
{
    EXPECT_TRUE(TestOzzKinematicsPoseSetPoseLocalsOverridesSingleBone());
}

TEST(OzzKinematicsPose, AdditionalBoneTransformsAffectSingleBone)
{
    EXPECT_TRUE(TestOzzKinematicsPoseAdditionalBoneTransformsAffectSingleBone());
}

TEST(OzzKinematicsPose, BuildsSkinningPaletteMatchesTransforms)
{
    EXPECT_TRUE(TestOzzKinematicsPoseBuildsSkinningPaletteMatchesTransforms());
}

TEST(OzzKinematicsVisibility, BoneVisibilityToggleZeroesTransforms)
{
    EXPECT_TRUE(TestOzzKinematicsVisibilityBoneVisibilityToggleZeroesTransforms());
}

TEST(OzzKinematicsVisibility, SetBonesVisibleControlsMask)
{
    EXPECT_TRUE(TestOzzKinematicsVisibilitySetBonesVisibleControlsMask());
}

TEST(OzzKinematicsAnimated, LoadsStandaloneClip)
{
    EXPECT_TRUE(TestOzzKinematicsAnimatedLoadsStandaloneClip());
}

TEST(OzzKinematicsCallbacks, InvokesBoneCallbackAndHonorsOverwrite)
{
    EXPECT_TRUE(TestOzzKinematicsCallbacksInvokeBoneCallbackAndHonorOverwrite());
}

TEST(OzzKinematicsParity, BindPoseMatchesLegacySkeleton)
{
    EXPECT_TRUE(TestOzzKinematicsParityBindPoseMatchesLegacySkeleton());
}

TEST(OzzKinematicsParity, AnimationPoseMatchesLegacySkeleton)
{
    EXPECT_TRUE(TestOzzKinematicsParityAnimationPoseMatchesLegacySkeleton());
}

TEST(OzzBundleRuntime, HydratesKinematicsAndMeshPayload)
{
    EXPECT_TRUE(TestOzzBundleRuntimeHydratesKinematicsAndMeshPayload());
}

TEST(OzzKinematicsParity, BoneMetadataHydratedByRuntime)
{
    bool metadata_available = false;
    const bool result = TestOzzKinematicsAppliesBoneMetadata(metadata_available);
    if (!metadata_available)
        return;
    EXPECT_TRUE(result);
}

TEST(ModelNaming, NormalizesModelIdentifiers)
{
    EXPECT_TRUE(TestModelNamingNormalizesModelIdentifiers());
}
