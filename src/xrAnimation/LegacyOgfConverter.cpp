#include "stdafx.h"

#include "LegacyOgfConverter.h"

#include "ExtendedBoneMetadata.h"
#include "LegacyOmfConverter.h"
#include "OzzConversion.h"

#include "xrCore/xrCore.h"

#include "xrCore/Animation/SkeletonMotionDefs.hpp"
#include "xrCore/FMesh.hpp"

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include <ozz/base/maths/quaternion.h>
#include <ozz/base/maths/soa_transform.h>

#include "../../Externals/ozz-animation/samples/framework/mesh.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace XRay
{
namespace Animation
{
namespace
{
using Matrix4 = std::array<std::array<float, 4>, 4>;

constexpr Matrix4 kXrayToOzz = {
    std::array<float, 4>{ -1.f, 0.f,  0.f, 0.f },
    std::array<float, 4>{ 0.f, 1.f,  0.f, 0.f },
    std::array<float, 4>{ 0.f, 0.f, -1.f, 0.f },
    std::array<float, 4>{ 0.f, 0.f,  0.f, 1.f }
};

constexpr Matrix4 kOzzToXray = kXrayToOzz;

Matrix4 ToColumnMajor(const Fmatrix& source)
{
    Matrix4 result{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            result[static_cast<size_t>(row)][static_cast<size_t>(col)] = source.m[col][row];
    return result;
}

Fmatrix ToRowMajor(const Matrix4& matrix)
{
    Fmatrix result;
    result.identity();
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            result.m[col][row] = matrix[static_cast<size_t>(row)][static_cast<size_t>(col)];
    return result;
}

Matrix4 Multiply(const Matrix4& lhs, const Matrix4& rhs)
{
    Matrix4 result{};
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            float value = 0.f;
            for (int k = 0; k < 4; ++k)
                value += lhs[static_cast<size_t>(row)][static_cast<size_t>(k)] *
                         rhs[static_cast<size_t>(k)][static_cast<size_t>(col)];
            result[static_cast<size_t>(row)][static_cast<size_t>(col)] = value;
        }
    }
    return result;
}

Matrix4 ChangeBasis(const Matrix4& matrix, const Matrix4& basis, const Matrix4& basis_inverse)
{
    return Multiply(Multiply(basis, matrix), basis_inverse);
}

Matrix4 ConvertXrayLocalToOzz(const Fmatrix& matrix)
{
    return ChangeBasis(ToColumnMajor(matrix), kXrayToOzz, kOzzToXray);
}

std::array<float, 3> ApplyBasis(const Matrix4& matrix, const std::array<float, 3>& vector)
{
    std::array<float, 3> result{};
    for (int row = 0; row < 3; ++row)
    {
        result[static_cast<size_t>(row)] = matrix[static_cast<size_t>(row)][0] * vector[0] +
            matrix[static_cast<size_t>(row)][1] * vector[1] +
            matrix[static_cast<size_t>(row)][2] * vector[2];
    }
    return result;
}

std::array<float, 3> ConvertVectorXrayToOzz(const Fvector& v)
{
    const std::array<float, 3> source{ v.x, v.y, v.z };
    return ApplyBasis(kXrayToOzz, source);
}

std::array<float, 2> ConvertUV(const Fvector2& uv)
{
    return { uv.x, uv.y };
}

std::array<float, 3> Normalize(const std::array<float, 3>& v)
{
    const float len_sq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (len_sq <= std::numeric_limits<float>::epsilon())
        return v;
    const float inv_len = 1.f / std::sqrt(len_sq);
    return { v[0] * inv_len, v[1] * inv_len, v[2] * inv_len };
}

float Dot(const std::array<float, 3>& a, const std::array<float, 3>& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

std::array<float, 3> Cross(const std::array<float, 3>& a, const std::array<float, 3>& b)
{
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    };
}

Matrix4 InvertMatrix(const Matrix4& matrix)
{
    const Fmatrix row_major = ToRowMajor(matrix);
    Fmatrix inverted;
    inverted.invert(row_major);
    return ToColumnMajor(inverted);
}

ozz::math::Float4x4 ToOzzFloat4x4(const Matrix4& matrix)
{
    ozz::math::Float4x4 result;
    for (int col = 0; col < 4; ++col)
    {
        float column[4] = {
            matrix[0][col],
            matrix[1][col],
            matrix[2][col],
            matrix[3][col],
        };
        result.cols[col] = ozz::math::simd_float4::LoadPtrU(column);
    }
    return result;
}

struct Chunk
{
    const std::byte* data = nullptr;
    size_t size = 0;
};

struct BinaryReader
{
    const std::byte* data = nullptr;
    size_t size = 0;
    size_t offset = 0;

    template <class T>
    T read()
    {
        if (offset + sizeof(T) > size)
            throw std::runtime_error("unexpected end of chunk while reading typed data");

        T value;
        std::memcpy(&value, data + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    template <class T>
    T read_struct()
    {
        return read<T>();
    }

    int8_t read_int8()
    {
        if (offset + sizeof(int8_t) > size)
            throw std::runtime_error("unexpected end of chunk while reading int8");
        const int8_t value = *reinterpret_cast<const int8_t*>(data + offset);
        offset += sizeof(int8_t);
        return value;
    }

    uint8_t read_uint8()
    {
        if (offset + sizeof(uint8_t) > size)
            throw std::runtime_error("unexpected end of chunk while reading uint8");
        const uint8_t value = *reinterpret_cast<const uint8_t*>(data + offset);
        offset += sizeof(uint8_t);
        return value;
    }

    std::string read_stringz()
    {
        const auto* begin = data + offset;
        const auto* end = data + size;
        const auto* cursor = begin;
        while (cursor < end && *reinterpret_cast<const char*>(cursor) != '\0')
            ++cursor;

        if (cursor == end)
            throw std::runtime_error("unterminated string in chunk");

        std::string value(reinterpret_cast<const char*>(begin), static_cast<size_t>(cursor - begin));
        offset += static_cast<size_t>(cursor - begin) + 1; // consume null terminator
        return value;
    }

    Fvector read_fvector3()
    {
        Fvector v{};
        v.x = read<float>();
        v.y = read<float>();
        v.z = read<float>();
        return v;
    }

    void skip(size_t count)
    {
        if (offset + count > size)
            throw std::runtime_error("attempted to skip past end of chunk");
        offset += count;
    }
};

struct BoneRecord
{
    std::string name;
    std::string parent_name;
    int parent_index = -1;
    Fobb obb{};
    Fvector rest_translation{};
    Fvector rest_rotation{}; // XYZ angles in radians (engine convention)
    Fmatrix local_transform{};
    Fmatrix global_transform{};
    float mass = 0.f;
    Fvector center_of_mass{};
    SBoneShape shape{};
    SJointIKData joint_ik{};
    std::string game_material;

    float rest_length = 0.f;
    Fvector dominant_axis{};
    Fvector local_aabb_min{};
    Fvector local_aabb_max{};
    Fmatrix inverse_global_transform{};
    Fvector inertia_tensor{};
    float volume = 0.f;
    Flags32 collision_layers{};
    bool ground_contact_candidate = false;
    bool weapon_anchor_candidate = false;
};

struct MotionMark
{
    std::string name;
    std::vector<std::pair<float, float>> intervals;
};

struct MotionMetadata
{
    std::string name;
    uint32_t flags = 0;
    uint16_t bone_or_part = 0;
    uint16_t motion_id = 0;
    float speed = 0.f;
    float power = 0.f;
    float accrue = 0.f;
    float falloff = 0.f;
    std::vector<MotionMark> marks;
};

struct MeshVertex
{
    Fvector position{};
    Fvector normal{};
    Fvector tangent{};
    Fvector binormal{};
    Fvector2 uv{};
    std::array<uint16_t, 4> bones{ 0, 0, 0, 0 };
    std::array<float, 4> weights{ 0.f, 0.f, 0.f, 0.f };
    uint8_t influence_count = 0;
};

struct ConvertedVertex
{
    std::array<float, 3> position{ 0.f, 0.f, 0.f };
    std::array<float, 3> normal{ 0.f, 0.f, 0.f };
    std::array<float, 4> tangent{ 0.f, 0.f, 0.f, 1.f };
    std::array<float, 2> uv{ 0.f, 0.f };
    std::array<uint16_t, 4> joint_indices{ 0, 0, 0, 0 };
    std::array<float, 4> joint_weights{ 0.f, 0.f, 0.f, 0.f };
    uint8_t influence_count = 0;
    size_t original_index = 0;
};

struct VertexDedupKey
{
    std::array<int32_t, 3> position{ 0, 0, 0 };
    std::array<int32_t, 2> uv{ 0, 0 };
    std::array<int32_t, 3> normal{ 0, 0, 0 };
    std::array<int32_t, 3> tangent{ 0, 0, 0 };
    std::array<int32_t, 3> binormal{ 0, 0, 0 };
    std::array<uint16_t, 4> bones{ 0, 0, 0, 0 };
    std::array<int32_t, 4> weights{ 0, 0, 0, 0 };
    uint8_t influence_count = 0;
    uint8_t back_side = 0;

    bool operator==(const VertexDedupKey& other) const noexcept
    {
        return influence_count == other.influence_count &&
            back_side == other.back_side &&
            position == other.position &&
            uv == other.uv &&
            normal == other.normal &&
            tangent == other.tangent &&
            binormal == other.binormal &&
            bones == other.bones &&
            weights == other.weights;
    }
};

struct VertexDedupKeyHasher
{
    size_t operator()(const VertexDedupKey& key) const noexcept
    {
        size_t hash = 1469598103934665603ull;
        auto combine = [&hash](size_t value)
        {
            hash ^= value;
            hash *= 1099511628211ull;
        };

        combine(std::hash<uint8_t>{}(key.influence_count));
        combine(std::hash<uint8_t>{}(key.back_side));
        for (int32_t component : key.position)
            combine(std::hash<int32_t>{}(component));
        for (int32_t component : key.uv)
            combine(std::hash<int32_t>{}(component));
        for (int32_t component : key.normal)
            combine(std::hash<int32_t>{}(component));
        for (int32_t component : key.tangent)
            combine(std::hash<int32_t>{}(component));
        for (int32_t component : key.binormal)
            combine(std::hash<int32_t>{}(component));
        for (uint16_t bone : key.bones)
            combine(std::hash<uint16_t>{}(bone));
        for (int32_t weight : key.weights)
            combine(std::hash<int32_t>{}(weight));
        return hash;
    }
};

struct PositionQuantizedKey
{
    std::array<int32_t, 3> components{ 0, 0, 0 };

    bool operator==(const PositionQuantizedKey& other) const noexcept
    {
        return components == other.components;
    }
};

struct PositionQuantizedHasher
{
    size_t operator()(const PositionQuantizedKey& key) const noexcept
    {
        size_t hash = 1469598103934665603ull;
        auto combine = [&hash](size_t value)
        {
            hash ^= value;
            hash *= 1099511628211ull;
        };
        for (int32_t component : key.components)
            combine(std::hash<int32_t>{}(component));
        return hash;
    }
};

struct BoneBoundsBuilder
{
    Fvector min;
    Fvector max;
    bool has_points = false;

    BoneBoundsBuilder()
    {
        reset();
    }

    void reset()
    {
        const float max_float = std::numeric_limits<float>::max();
        min.set(max_float, max_float, max_float);
        max.set(-max_float, -max_float, -max_float);
        has_points = false;
    }

    void AddPoint(const Fvector& point)
    {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
        has_points = true;
    }
};

void SerializeString(ozz::io::OArchive& archive, const std::string& value)
{
    const uint32_t length = static_cast<uint32_t>(value.size());
    archive << length;
    if (length > 0)
        archive << ozz::io::MakeArray(value.c_str(), length);
}

void SerializeMotionMarks(ozz::io::OArchive& archive, const LegacyMotionMetadata& metadata)
{
    const uint32_t mark_count = static_cast<uint32_t>(metadata.marks.size());
    archive << mark_count;
    for (const auto& mark : metadata.marks)
    {
        SerializeString(archive, std::string(mark.name.c_str()));
        const uint32_t interval_count = static_cast<uint32_t>(mark.intervals.size());
        archive << interval_count;
        for (const auto& interval : mark.intervals)
        {
            archive << interval.first;
            archive << interval.second;
        }
    }
}

void SerializeMotionMetadata(ozz::io::OArchive& archive, const LegacyMotionMetadata& metadata)
{
    SerializeString(archive, std::string(metadata.name.c_str()));
    archive << metadata.flags;
    archive << metadata.bone_or_part;
    archive << metadata.motion_id;
    archive << metadata.speed;
    archive << metadata.power;
    archive << metadata.accrue;
    archive << metadata.falloff;
    SerializeMotionMarks(archive, metadata);
}

std::vector<std::uint8_t> SerializeEmbeddedAnimations(const xr_vector<ConvertedOmfAnimation>& animations)
{
    const auto has_animation = [](const ConvertedOmfAnimation& entry)
    {
        return static_cast<bool>(entry.animation);
    };

    if (std::none_of(animations.begin(), animations.end(), has_animation))
        return {};

    ozz::io::MemoryStream animation_stream;
    ozz::io::OArchive archive(&animation_stream);

    const uint32_t animation_count = static_cast<uint32_t>(std::count_if(animations.begin(), animations.end(), has_animation));
    archive << animation_count;

    for (const auto& entry : animations)
    {
        if (!entry.animation)
            continue;

        archive << *entry.animation;
        SerializeMotionMetadata(archive, entry.metadata);
        SerializeBoneMotions(archive, entry);
    }

    std::vector<std::uint8_t> binary(animation_stream.Size());
    animation_stream.Seek(0, ozz::io::Stream::kSet);
    if (!binary.empty())
        animation_stream.Read(binary.data(), binary.size());
    return binary;
}

xr_vector<ConvertedOmfAnimation> BuildRestPoseAnimation(const ozz::animation::Skeleton& skeleton)
{
    const int joint_count = skeleton.num_joints();
    if (joint_count <= 0)
        return {};

    ozz::animation::offline::RawAnimation raw_animation;
    raw_animation.duration = SAMPLE_SPF;
    raw_animation.name = "$editor";
    raw_animation.tracks.resize(joint_count);

    for (int joint = 0; joint < joint_count; ++joint)
    {
        auto& track = raw_animation.tracks[joint];

        track.translations.resize(1);
        track.translations[0].time = 0.f;
        track.translations[0].value = ozz::math::Float3(0.f, 0.f, 0.f);

        track.rotations.resize(1);
        track.rotations[0].time = 0.f;
        track.rotations[0].value = ozz::math::Quaternion::identity();

        track.scales.resize(1);
        track.scales[0].time = 0.f;
        track.scales[0].value = ozz::math::Float3(1.f, 1.f, 1.f);
    }

    ozz::animation::offline::AnimationBuilder builder;
    auto animation = builder(raw_animation);
    if (!animation)
        return {};

    ConvertedOmfAnimation converted;
    converted.name = raw_animation.name.c_str();
    converted.animation = std::move(animation);
    converted.frame_count = 1;
    converted.metadata.name = raw_animation.name.c_str();
    converted.metadata.flags = 0;
    converted.metadata.bone_or_part = 0;
    converted.metadata.motion_id = 0;
    converted.metadata.speed = 1.f;
    converted.metadata.power = 1.f;
    converted.metadata.accrue = 0.f;
    converted.metadata.falloff = 0.f;

    xr_vector<ConvertedOmfAnimation> animations;
    animations.emplace_back(std::move(converted));
    return animations;
}

struct RoundedNormalKey
{
    std::array<int32_t, 3> components{ 0, 0, 0 };

    bool operator==(const RoundedNormalKey& other) const noexcept
    {
        return components == other.components;
    }
};

struct RoundedNormalHasher
{
    size_t operator()(const RoundedNormalKey& key) const noexcept
    {
        size_t hash = 1469598103934665603ull;
        auto combine = [&hash](size_t value)
        {
            hash ^= value;
            hash *= 1099511628211ull;
        };
        for (int32_t component : key.components)
            combine(std::hash<int32_t>{}(component));
        return hash;
    }
};

struct SurfaceMetadata
{
    std::string texture_path;
    std::string shader_name;
    uint32_t original_vertex_count = 0;
    uint32_t original_face_count = 0;
    uint8_t ogf_type = 0;
    std::vector<std::string> lod_visuals;
    std::vector<uint8_t> lod_data;
    uint32_t progressive_collapse_count = 0;
    std::vector<uint8_t> progressive_data;
    std::vector<uint32_t> child_visual_links;
};

struct SurfaceDefinition
{
    SurfaceMetadata metadata;
    std::vector<MeshVertex> vertices;
    std::vector<uint16_t> indices;
};

enum : uint32_t
{
    kChunkHeader = OGF_HEADER,
    kChunkBoneNames = OGF_S_BONE_NAMES,
    kChunkIkData = OGF_S_IKDATA,
    kChunkVertices = OGF_VERTICES,
    kChunkIndices = OGF_INDICES,
    kChunkChildren = OGF_CHILDREN,
    kChunkTexture = OGF_TEXTURE,
    kChunkChildrenLinks = OGF_CHILDREN_L,
    kChunkLodDefinition = OGF_S_LODS,
    kChunkLodDefinition2 = OGF_LODDEF2,
    kChunkSwidata = OGF_SWIDATA,
    kChunkMotionRefs = OGF_S_MOTION_REFS,
    kChunkMotionRefs2 = OGF_S_MOTION_REFS2,
    kChunkSmparams = OGF_S_SMPARAMS,
    kChunkMotions = OGF_S_MOTIONS,
    kChunkUserData = OGF_S_USERDATA,
};

} // namespace

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

std::unordered_map<uint32_t, Chunk> ParseChunks(const std::byte* data, size_t size)
{
    std::unordered_map<uint32_t, Chunk> chunks;

    size_t offset = 0;
    while (offset + sizeof(uint32_t) * 2 <= size)
    {
        uint32_t id = 0;
        uint32_t chunk_size = 0;
        std::memcpy(&id, data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::memcpy(&chunk_size, data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        if (offset + chunk_size > size)
            throw std::runtime_error("chunk extends past end of file");

        chunks.emplace(id, Chunk{ data + offset, chunk_size });
        offset += chunk_size;
    }

    return chunks;
}

std::vector<std::pair<uint32_t, Chunk>> ParseSubchunks(const Chunk& chunk)
{
    std::vector<std::pair<uint32_t, Chunk>> subchunks;
    size_t offset = 0;
    while (offset + sizeof(uint32_t) * 2 <= chunk.size)
    {
        uint32_t id = 0;
        uint32_t sub_size = 0;
        std::memcpy(&id, chunk.data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::memcpy(&sub_size, chunk.data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        if (offset + sub_size > chunk.size)
            throw std::runtime_error("sub-chunk extends past parent chunk");

        subchunks.emplace_back(id, Chunk{ chunk.data + offset, sub_size });
        offset += sub_size;
    }
    return subchunks;
}

std::vector<std::string> ParseLodStrings(const Chunk& chunk)
{
    std::vector<std::string> results;
    std::string current;
    current.reserve(chunk.size);

    for (size_t idx = 0; idx < chunk.size; ++idx)
    {
        const char ch = static_cast<char>(chunk.data[idx]);
        if (ch == '\0' || ch == '\n' || ch == '\r')
        {
            if (!current.empty())
            {
                results.push_back(current);
                current.clear();
            }
        }
        else
        {
            current.push_back(ch);
        }
    }

    if (!current.empty())
        results.push_back(current);

    return results;
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

std::string TrimCopy(std::string_view value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    const auto end = value.find_last_not_of(" \t\r\n");
    if (begin == std::string_view::npos)
        return {};
    return std::string(value.substr(begin, end - begin + 1));
}

std::vector<std::string> SplitMotionRefString(const std::string& combined)
{
    std::vector<std::string> entries;
    size_t cursor = 0;
    while (cursor < combined.size())
    {
        const auto next = combined.find(',', cursor);
        const auto token = combined.substr(cursor, next == std::string::npos ? std::string::npos : next - cursor);
        const auto trimmed = TrimCopy(token);
        if (!trimmed.empty())
            entries.emplace_back(trimmed);
        if (next == std::string::npos)
            break;
        cursor = next + 1;
    }
    return entries;
}

void ParseTextureChunk(const Chunk& chunk, SurfaceMetadata& metadata)
{
    BinaryReader reader{ chunk.data, chunk.size };
    if (reader.offset >= reader.size)
        return;

    metadata.texture_path = reader.read_stringz();
    if (reader.offset < reader.size)
        metadata.shader_name = reader.read_stringz();
}

void ParseHeaderChunk(const Chunk& chunk, SurfaceMetadata& metadata)
{
    if (chunk.size < sizeof(ogf_header))
        return;

    ogf_header header{};
    std::memcpy(&header, chunk.data, sizeof(header));
    metadata.ogf_type = header.type;
}

void ParseLodChunk(const Chunk& chunk, SurfaceMetadata& metadata)
{
    const auto* bytes = reinterpret_cast<const uint8_t*>(chunk.data);
    metadata.lod_data.assign(bytes, bytes + chunk.size);

    auto strings = ParseLodStrings(chunk);
    metadata.lod_visuals.insert(metadata.lod_visuals.end(), strings.begin(), strings.end());
}

void ParseChildrenLinksChunk(const Chunk& chunk, SurfaceMetadata& metadata)
{
    if (chunk.size < sizeof(uint32_t))
        return;

    BinaryReader reader{ chunk.data, chunk.size };
    const uint32_t count = reader.read<u32>();
    metadata.child_visual_links.reserve(metadata.child_visual_links.size() + count);
    for (uint32_t idx = 0; idx < count && reader.offset + sizeof(uint32_t) <= reader.size; ++idx)
        metadata.child_visual_links.push_back(reader.read<u32>());
}

void ParseProgressiveChunk(const Chunk& chunk, SurfaceMetadata& metadata)
{
    const auto* bytes = reinterpret_cast<const uint8_t*>(chunk.data);
    metadata.progressive_data.assign(bytes, bytes + chunk.size);

    if (chunk.size < sizeof(uint32_t) * 5)
        return;

    BinaryReader reader{ chunk.data, chunk.size };
    reader.skip(sizeof(uint32_t) * 4);
    metadata.progressive_collapse_count = reader.read<u32>();
}

std::vector<BoneRecord> ReadBoneNames(const Chunk& chunk)
{
    BinaryReader reader{ chunk.data, chunk.size };
    const u32 bone_count = reader.read<u32>();

    std::vector<BoneRecord> bones;
    bones.reserve(bone_count);

    for (u32 idx = 0; idx < bone_count; ++idx)
    {
        BoneRecord record;
        record.name = reader.read_stringz();
        record.parent_name = reader.read_stringz();
        record.obb = reader.read_struct<Fobb>();
        bones.emplace_back(std::move(record));
    }

    return bones;
}

void ReadIkData(const Chunk& chunk, std::vector<BoneRecord>& bones)
{
    BinaryReader reader{ chunk.data, chunk.size };

    for (auto& bone : bones)
    {
        const u32 version = reader.read<u32>();

        bone.game_material = reader.read_stringz();
        bone.shape = reader.read_struct<SBoneShape>();

        bone.joint_ik.type = static_cast<EJointType>(reader.read<u32>());
        for (int axis = 0; axis < 3; ++axis)
        {
            auto& limit = bone.joint_ik.limits[axis];
            limit.limit.x = reader.read<float>();
            limit.limit.y = reader.read<float>();
            limit.spring_factor = reader.read<float>();
            limit.damping_factor = reader.read<float>();
        }

        bone.joint_ik.spring_factor = reader.read<float>();
        bone.joint_ik.damping_factor = reader.read<float>();
        bone.joint_ik.ik_flags.assign(reader.read<u32>());
        bone.joint_ik.break_force = reader.read<float>();
        bone.joint_ik.break_torque = reader.read<float>();
        bone.joint_ik.friction = version > 0 ? reader.read<float>() : 0.f;

        bone.rest_rotation = reader.read_fvector3();
        bone.rest_translation = reader.read_fvector3();
        bone.mass = reader.read<float>();
        bone.center_of_mass = reader.read_fvector3();
    }
}

void ComputeHierarchy(std::vector<BoneRecord>& bones)
{
    std::unordered_map<std::string, int> lookup;
    lookup.reserve(bones.size());
    for (size_t idx = 0; idx < bones.size(); ++idx)
        lookup.emplace(bones[idx].name, static_cast<int>(idx));

    for (auto& bone : bones)
    {
        if (bone.parent_name.empty())
        {
            bone.parent_index = -1;
            continue;
        }

        const auto it = lookup.find(bone.parent_name);
        if (it == lookup.end())
            throw std::runtime_error("bone parent not found: " + bone.parent_name + " for bone " + bone.name);
        bone.parent_index = it->second;
    }
}

void ComputeLocalTransforms(std::vector<BoneRecord>& bones)
{
    for (auto& bone : bones)
    {
        Fmatrix m;
        m.identity();
        m.setXYZi(bone.rest_rotation);
        m.translate_over(bone.rest_translation);
        bone.local_transform = m;
        bone.global_transform.identity();
    }
}

void ComputeGlobalTransforms(std::vector<BoneRecord>& bones)
{
    std::vector<std::vector<int>> children(bones.size());
    for (size_t idx = 0; idx < bones.size(); ++idx)
        if (bones[idx].parent_index >= 0)
            children[bones[idx].parent_index].push_back(static_cast<int>(idx));

    std::function<void(int, const Fmatrix&)> visit = [&](int index, const Fmatrix& parent_matrix)
    {
        auto& bone = bones[static_cast<size_t>(index)];
        bone.global_transform.mul_43(parent_matrix, bone.local_transform);
        for (int child : children[static_cast<size_t>(index)])
            visit(child, bone.global_transform);
    };

    for (size_t idx = 0; idx < bones.size(); ++idx)
        if (bones[idx].parent_index < 0)
        {
            Fmatrix identity;
            identity.identity();
            visit(static_cast<int>(idx), identity);
        }
}

constexpr float kVectorEpsilon = 1e-6f;
constexpr float kGroundHeightThreshold = 0.12f;
constexpr float kDownAlignmentThreshold = -0.7f;
constexpr float kPi = 3.14159265358979323846f;

Fvector NormalizeOrDefault(const Fvector& value, const Fvector& fallback)
{
    Fvector result = value;
    if (result.square_magnitude() <= kVectorEpsilon)
    {
        result.set(fallback.x, fallback.y, fallback.z);
        if (result.square_magnitude() <= kVectorEpsilon)
            result.set(0.f, 1.f, 0.f);
    }
    result.normalize_safe();
    return result;
}

Fmatrix ExtractRotation(const Fmatrix& source)
{
    Fmatrix rotation = source;
    rotation.c.set(0.f, 0.f, 0.f);
    rotation._44_ = 1.f;
    return rotation;
}

Fvector ComputeDominantAxis(const BoneRecord& bone)
{
    Fvector default_axis;
    default_axis.set(0.f, 1.f, 0.f);
    Fvector fallback = NormalizeOrDefault(bone.local_transform.c, default_axis);

    const SBoneShape& shape = bone.shape;
    const Fmatrix rotation = ExtractRotation(bone.local_transform);

    if (shape.type == SBoneShape::stBox)
    {
        const Fvector& halfsize = shape.box.m_halfsize;
        float extents[3] = { halfsize.x, halfsize.y, halfsize.z };
        int axis_index = 0;
        float max_extent = extents[0];
        for (int idx = 1; idx < 3; ++idx)
        {
            if (extents[idx] > max_extent)
            {
                axis_index = idx;
                max_extent = extents[idx];
            }
        }

        if (max_extent > kVectorEpsilon)
        {
            Fvector local_axis;
            switch (axis_index)
            {
            case 0: local_axis.set(shape.box.m_rotate.i); break;
            case 1: local_axis.set(shape.box.m_rotate.j); break;
            default: local_axis.set(shape.box.m_rotate.k); break;
            }

            if (local_axis.square_magnitude() > kVectorEpsilon)
            {
                Fvector oriented_axis;
                rotation.transform_dir(oriented_axis, local_axis);
                return NormalizeOrDefault(oriented_axis, fallback);
            }
        }
    }
    else if (shape.type == SBoneShape::stCylinder)
    {
        Fvector axis = shape.cylinder.m_direction;
        if (axis.square_magnitude() > kVectorEpsilon)
        {
            axis.normalize_safe();
            Fvector oriented_axis;
            rotation.transform_dir(oriented_axis, axis);
            return NormalizeOrDefault(oriented_axis, fallback);
        }
    }

    return fallback;
}

void ExpandBounds(Fvector& min, Fvector& max, const Fvector& point)
{
    min.x = std::min(min.x, point.x);
    min.y = std::min(min.y, point.y);
    min.z = std::min(min.z, point.z);
    max.x = std::max(max.x, point.x);
    max.y = std::max(max.y, point.y);
    max.z = std::max(max.z, point.z);
}

std::pair<Fvector, Fvector> ComputeLocalAabb(const BoneRecord& bone)
{
    Fvector bounds_min;
    bounds_min.set(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Fvector bounds_max;
    bounds_max.set(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

    bool has_points = false;
    auto emit_point = [&](const Fvector& local_point)
    {
        Fvector parent_point;
        bone.local_transform.transform_tiny(parent_point, local_point);
        ExpandBounds(bounds_min, bounds_max, parent_point);
        has_points = true;
    };

    const SBoneShape& shape = bone.shape;

    switch (shape.type)
    {
    case SBoneShape::stBox:
    {
        const Fvector& center = shape.box.m_translate;
        const Fvector& halfsize = shape.box.m_halfsize;
        if (halfsize.square_magnitude() > kVectorEpsilon)
        {
            const Fvector axes[3] = {
                Fvector().set(shape.box.m_rotate.i),
                Fvector().set(shape.box.m_rotate.j),
                Fvector().set(shape.box.m_rotate.k),
            };

            for (int x_sign = -1; x_sign <= 1; x_sign += 2)
            {
                for (int y_sign = -1; y_sign <= 1; y_sign += 2)
                {
                    for (int z_sign = -1; z_sign <= 1; z_sign += 2)
                    {
                        Fvector corner = center;
                        Fvector offset = axes[0];
                        offset.mul(static_cast<float>(x_sign) * halfsize.x);
                        corner.add(offset);
                        offset = axes[1];
                        offset.mul(static_cast<float>(y_sign) * halfsize.y);
                        corner.add(offset);
                        offset = axes[2];
                        offset.mul(static_cast<float>(z_sign) * halfsize.z);
                        corner.add(offset);
                        emit_point(corner);
                    }
                }
            }
        }
        break;
    }
    case SBoneShape::stSphere:
    {
        Fvector center;
        bone.local_transform.transform_tiny(center, shape.sphere.P);
        const float radius = shape.sphere.R;
        if (radius > 0.f)
        {
            Fvector radius_vec;
            radius_vec.set(radius, radius, radius);
            Fvector min_point = center;
            min_point.sub(radius_vec);
            Fvector max_point = center;
            max_point.add(radius_vec);
            ExpandBounds(bounds_min, bounds_max, min_point);
            ExpandBounds(bounds_min, bounds_max, max_point);
            has_points = true;
        }
        else
        {
            emit_point(shape.sphere.P);
        }
        break;
    }
    case SBoneShape::stCylinder:
    {
        const float radius = shape.cylinder.m_radius;
        const float height = shape.cylinder.m_height;
        Fvector dir = shape.cylinder.m_direction;
        if (radius > 0.f && height > 0.f && dir.square_magnitude() > kVectorEpsilon)
        {
            dir.normalize_safe();
            Fvector up;
            up.set(0.f, 1.f, 0.f);
            if (std::fabs(dir.dotproduct(up)) > 0.9f)
                up.set(1.f, 0.f, 0.f);

            Fvector perp1;
            perp1.crossproduct(up, dir);
            if (perp1.square_magnitude() <= kVectorEpsilon)
            {
                perp1.set(1.f, 0.f, 0.f);
                if (std::fabs(dir.dotproduct(perp1)) > 0.9f)
                    perp1.set(0.f, 0.f, 1.f);
                perp1.crossproduct(perp1, dir);
            }
            perp1.normalize_safe();

            Fvector perp2;
            perp2.crossproduct(dir, perp1);
            perp2.normalize_safe();

            const float half_height = height * 0.5f;
            const Fvector center = shape.cylinder.m_center;

            for (int axis_sign = -1; axis_sign <= 1; axis_sign += 2)
            {
                Fvector cap_center = center;
                Fvector axis_offset = dir;
                axis_offset.mul(static_cast<float>(axis_sign) * half_height);
                cap_center.add(axis_offset);

                for (int u_sign = -1; u_sign <= 1; u_sign += 2)
                {
                    for (int v_sign = -1; v_sign <= 1; v_sign += 2)
                    {
                        Fvector point = cap_center;
                        Fvector offset_u = perp1;
                        offset_u.mul(static_cast<float>(u_sign) * radius);
                        point.add(offset_u);
                        Fvector offset_v = perp2;
                        offset_v.mul(static_cast<float>(v_sign) * radius);
                        point.add(offset_v);
                        emit_point(point);
                    }
                }
            }

            emit_point(center);
        }
        else
        {
            emit_point(shape.cylinder.m_center);
        }
        break;
    }
    default:
        break;
    }

    if (!has_points)
        emit_point(bone.local_transform.c);

    return { bounds_min, bounds_max };
}

float ComputeShapeVolume(const SBoneShape& shape)
{
    switch (shape.type)
    {
    case SBoneShape::stBox:
        return 8.f * shape.box.m_halfsize.x * shape.box.m_halfsize.y * shape.box.m_halfsize.z;
    case SBoneShape::stSphere:
        return (4.f / 3.f) * kPi * shape.sphere.R * shape.sphere.R * shape.sphere.R;
    case SBoneShape::stCylinder:
        return kPi * shape.cylinder.m_radius * shape.cylinder.m_radius * shape.cylinder.m_height;
    default:
        return 0.f;
    }
}

Fvector ComputeInertiaTensor(const SBoneShape& shape, float mass)
{
    Fvector inertia;
    inertia.set(0.f, 0.f, 0.f);
    if (mass <= kVectorEpsilon)
        return inertia;

    switch (shape.type)
    {
    case SBoneShape::stBox:
    {
        const float hx = shape.box.m_halfsize.x;
        const float hy = shape.box.m_halfsize.y;
        const float hz = shape.box.m_halfsize.z;
        inertia.x = (mass / 3.f) * (hy * hy + hz * hz);
        inertia.y = (mass / 3.f) * (hx * hx + hz * hz);
        inertia.z = (mass / 3.f) * (hx * hx + hy * hy);
        break;
    }
    case SBoneShape::stSphere:
    {
        const float val = 0.4f * mass * shape.sphere.R * shape.sphere.R;
        inertia.set(val, val, val);
        break;
    }
    case SBoneShape::stCylinder:
    {
        Fvector dir = shape.cylinder.m_direction;
        if (dir.square_magnitude() <= kVectorEpsilon)
        {
            const float val = (mass / 12.f) * (3.f * shape.cylinder.m_radius * shape.cylinder.m_radius +
                                              shape.cylinder.m_height * shape.cylinder.m_height);
            inertia.set(val, val, val);
            break;
        }

        dir.normalize_safe();
        float components[3] = { std::fabs(dir.x), std::fabs(dir.y), std::fabs(dir.z) };
        int axis_index = 0;
        float max_component = components[0];
        for (int idx = 1; idx < 3; ++idx)
        {
            if (components[idx] > max_component)
            {
                max_component = components[idx];
                axis_index = idx;
            }
        }

        const float radius = shape.cylinder.m_radius;
        const float height = shape.cylinder.m_height;
        const float axial = 0.5f * mass * radius * radius;
        const float radial = (mass / 12.f) * (3.f * radius * radius + height * height);

        float* values[3] = { &inertia.x, &inertia.y, &inertia.z };
        *values[axis_index] = axial;
        *values[(axis_index + 1) % 3] = radial;
        *values[(axis_index + 2) % 3] = radial;
        break;
    }
    default:
        break;
    }

    return inertia;
}

bool ContainsToken(const std::string& text, std::string_view token)
{
    return text.find(token) != std::string::npos;
}

u32 SynthesizeCollisionLayers(const BoneRecord& bone)
{
    std::string name_lower = ToLowerCopy(bone.name);
    std::string material_lower = ToLowerCopy(bone.game_material);

    u32 layers = BoneCollisionLayerNone;

    if (ContainsToken(name_lower, "weapon") || ContainsToken(name_lower, "wpn") || ContainsToken(material_lower, "weapon"))
        layers |= BoneCollisionLayerWeapon;

    if (ContainsToken(name_lower, "spine") || ContainsToken(name_lower, "pelvis") || ContainsToken(name_lower, "torso") ||
        ContainsToken(name_lower, "thigh") || ContainsToken(name_lower, "calf") || ContainsToken(name_lower, "clavicle") ||
        ContainsToken(name_lower, "upperarm") || ContainsToken(name_lower, "forearm") || ContainsToken(material_lower, "metal") ||
        ContainsToken(material_lower, "rigid"))
    {
        layers |= BoneCollisionLayerRigidBody;
    }

    if (ContainsToken(name_lower, "arm") || ContainsToken(name_lower, "leg") || ContainsToken(name_lower, "hand") ||
        ContainsToken(name_lower, "head") || ContainsToken(name_lower, "neck") || ContainsToken(material_lower, "flesh") ||
        ContainsToken(material_lower, "skin") || ContainsToken(material_lower, "cloth"))
    {
        layers |= BoneCollisionLayerSoftTissue;
    }

    if (bone.mass > 1.f)
        layers |= BoneCollisionLayerRigidBody;
    else if (bone.mass <= 0.2f)
        layers |= BoneCollisionLayerSoftTissue;

    if (layers == BoneCollisionLayerNone)
        layers = BoneCollisionLayerSoftTissue;

    return layers;
}

bool IsGroundContactCandidate(const BoneRecord& bone, float min_global_y)
{
    Fvector down;
    down.set(0.f, -1.f, 0.f);

    Fvector axis = bone.dominant_axis;
    axis.normalize_safe();

    const bool aligned_down = axis.dotproduct(down) <= kDownAlignmentThreshold;
    const bool near_ground = std::fabs(bone.global_transform.c.y - min_global_y) <= kGroundHeightThreshold;

    std::string name_lower = ToLowerCopy(bone.name);
    const bool name_match = ContainsToken(name_lower, "foot") || ContainsToken(name_lower, "feet") ||
                            ContainsToken(name_lower, "toe") || ContainsToken(name_lower, "paw") ||
                            ContainsToken(name_lower, "hoof") || ContainsToken(name_lower, "ankle");

    return near_ground && (name_match || aligned_down);
}

bool IsWeaponAnchorCandidate(const BoneRecord& bone)
{
    std::string name_lower = ToLowerCopy(bone.name);
    std::string material_lower = ToLowerCopy(bone.game_material);

    const bool name_match = ContainsToken(name_lower, "hand") || ContainsToken(name_lower, "weapon") ||
        ContainsToken(name_lower, "wpn") || ContainsToken(name_lower, "grip") || ContainsToken(name_lower, "palm");

    const bool material_match = ContainsToken(material_lower, "weapon") || ContainsToken(material_lower, "metal");

    return name_match || material_match;
}

void ComputeExtendedBoneMetadata(std::vector<BoneRecord>& bones)
{
    if (bones.empty())
        return;

    float min_global_y = std::numeric_limits<float>::max();
    for (const auto& bone : bones)
        min_global_y = std::min(min_global_y, bone.global_transform.c.y);

    for (auto& bone : bones)
    {
        bone.rest_length = bone.local_transform.c.magnitude();
        bone.dominant_axis = ComputeDominantAxis(bone);

        const auto [aabb_min, aabb_max] = ComputeLocalAabb(bone);
        bone.local_aabb_min = aabb_min;
        bone.local_aabb_max = aabb_max;

        bone.inverse_global_transform = bone.global_transform;
        if (!bone.inverse_global_transform.invert_b(bone.global_transform))
            bone.inverse_global_transform.identity();

        bone.volume = ComputeShapeVolume(bone.shape);
        bone.inertia_tensor = ComputeInertiaTensor(bone.shape, bone.mass);

        bone.collision_layers.assign(SynthesizeCollisionLayers(bone));
        bone.ground_contact_candidate = IsGroundContactCandidate(bone, min_global_y);
        bone.weapon_anchor_candidate = IsWeaponAnchorCandidate(bone);
    }
}

std::vector<BoneRecord> LoadSkeletonBonesFromOgf(const std::byte* data, size_t size)
{
    const auto chunks = ParseChunks(data, size);

    auto bone_names_it = chunks.find(kChunkBoneNames);
    if (bone_names_it == chunks.end())
        throw std::runtime_error("OGF file missing bone names chunk");

    auto ik_data_it = chunks.find(kChunkIkData);
    if (ik_data_it == chunks.end())
        throw std::runtime_error("OGF file missing IK data chunk");

    auto bones = ReadBoneNames(bone_names_it->second);
    ReadIkData(ik_data_it->second, bones);
    ComputeHierarchy(bones);
    ComputeLocalTransforms(bones);
    ComputeGlobalTransforms(bones);
    ComputeExtendedBoneMetadata(bones);
    return bones;
}

ozz::animation::offline::RawSkeleton BuildRawSkeleton(const std::vector<BoneRecord>& bones)
{
    std::vector<std::vector<int>> children(bones.size());
    std::vector<int> roots;
    roots.reserve(bones.size());

    for (size_t idx = 0; idx < bones.size(); ++idx)
    {
        if (bones[idx].parent_index >= 0)
            children[bones[idx].parent_index].push_back(static_cast<int>(idx));
        else
            roots.push_back(static_cast<int>(idx));
    }

    ozz::animation::offline::RawSkeleton skeleton;
    skeleton.roots.resize(roots.size());

    std::function<void(int, ozz::animation::offline::RawSkeleton::Joint&)> populate;
    populate = [&](int index, ozz::animation::offline::RawSkeleton::Joint& joint)
    {
        const auto& bone = bones[static_cast<size_t>(index)];
        joint.name = bone.name.c_str();
        const auto ozz_matrix = ConvertXrayLocalToOzz(bone.local_transform);
        joint.transform.translation = detail::ExtractTranslation(ozz_matrix);
        joint.transform.rotation = detail::ExtractQuaternion(ozz_matrix);
        joint.transform.scale = { 1.0f, 1.0f, 1.0f };

        const auto& child_indices = children[static_cast<size_t>(index)];
        joint.children.resize(child_indices.size());
        for (size_t child_idx = 0; child_idx < child_indices.size(); ++child_idx)
            populate(child_indices[child_idx], joint.children[child_idx]);
    };

    for (size_t root_idx = 0; root_idx < roots.size(); ++root_idx)
        populate(roots[root_idx], skeleton.roots[root_idx]);

    return skeleton;
}

std::vector<std::string> LoadMotionRefsFromOgf(const std::byte* data, size_t size)
{
    const auto chunks = ParseChunks(data, size);

    std::vector<std::string> references;

    const auto refs2_it = chunks.find(OGF_S_MOTION_REFS2);
    if (refs2_it != chunks.end())
    {
        BinaryReader reader{ refs2_it->second.data, refs2_it->second.size };
        const u32 count = reader.read<u32>();
        references.reserve(count);
        for (u32 index = 0; index < count; ++index)
        {
            std::string entry = reader.read_stringz();
            entry = TrimCopy(entry);
            if (!entry.empty())
                references.emplace_back(std::move(entry));
        }
        return references;
    }

    const auto refs_it = chunks.find(OGF_S_MOTION_REFS);
    if (refs_it == chunks.end())
        return references;

    BinaryReader reader{ refs_it->second.data, refs_it->second.size };
    if (reader.size == 0)
        return references;

    std::string combined = reader.read_stringz();
    references = SplitMotionRefString(combined);
    return references;
}

void finalize_influences(MeshVertex& vertex, const std::array<uint16_t, 4>& bone_ids, const std::array<float, 4>& weights, uint32_t link_type)
{
    std::map<uint16_t, float> accum;
    for (uint32_t idx = 0; idx < link_type && idx < 4; ++idx)
    {
        const uint16_t bone = bone_ids[idx];
        const float weight = weights[idx];
        accum[bone] += weight;
    }

std::vector<std::pair<uint16_t, float>> combined(accum.begin(), accum.end());
    combined.erase(std::remove_if(combined.begin(), combined.end(),
                       [](const auto& entry)
                       {
                           return std::fabs(entry.second) <= std::numeric_limits<float>::epsilon();
                       }),
        combined.end());

    if (combined.empty())
    {
        combined.emplace_back(bone_ids[0], 1.f);
    }

    if (combined.size() > 4)
    {
        std::stable_sort(combined.begin(), combined.end(),
            [](const auto& lhs, const auto& rhs)
            {
                return lhs.second > rhs.second;
            });
        combined.resize(4);
    }

    std::sort(combined.begin(), combined.end(),
        [](const auto& lhs, const auto& rhs)
        {
            if (lhs.first != rhs.first)
                return lhs.first < rhs.first;
            return lhs.second > rhs.second;
        });

    float sum = 0.f;
    for (const auto& entry : combined)
        sum += entry.second;

    if (sum <= std::numeric_limits<float>::epsilon())
    {
        combined.clear();
        combined.emplace_back(bone_ids[0], 1.f);
        sum = 1.f;
    }

    const float inv_sum = 1.f / sum;
    for (auto& entry : combined)
        entry.second *= inv_sum;

    vertex.influence_count = static_cast<uint8_t>(combined.size());
    for (size_t idx = 0; idx < combined.size(); ++idx)
    {
        vertex.bones[idx] = combined[idx].first;
        vertex.weights[idx] = combined[idx].second;
    }

    for (size_t idx = combined.size(); idx < 4; ++idx)
    {
        vertex.bones[idx] = vertex.bones[0];
        vertex.weights[idx] = idx == 0 ? 1.f : 0.f;
    }
}

std::optional<uint32_t> decode_vertex_format_influence_count(uint32_t vertex_format)
{
    switch (vertex_format)
    {
    case OGF_VERTEXFORMAT_FVF_NL:
        return 0u;
    case OGF_VERTEXFORMAT_FVF_1L:
    case 1u: // Clear Sky / CoP compact encoding
        return 1u;
    case OGF_VERTEXFORMAT_FVF_2L:
    case 2u:
        return 2u;
    case OGF_VERTEXFORMAT_FVF_3L:
    case 3u:
        return 3u;
    case OGF_VERTEXFORMAT_FVF_4L:
    case 4u:
        return 4u;
    default:
        return std::nullopt;
    }
}

std::vector<MeshVertex> read_mesh_vertices(const Chunk& chunk)
{
    BinaryReader reader{ chunk.data, chunk.size };
    const uint32_t vertex_format = reader.read<uint32_t>();
    const uint32_t vertex_count = reader.read<uint32_t>();

    const auto link_type_opt = decode_vertex_format_influence_count(vertex_format);
    if (!link_type_opt)
    {
        std::ostringstream message;
        message << "unsupported vertex format in OGF mesh (0x" << std::hex << std::uppercase
                << vertex_format << ")";
        throw std::runtime_error(message.str());
    }
    const uint32_t link_type = *link_type_opt;

    const auto compute_vertex_size = [](uint32_t influences, bool include_tangent_basis) -> size_t
    {
        constexpr size_t kFloat3 = sizeof(float) * 3;
        constexpr size_t kFloat2 = sizeof(float) * 2;
        const size_t tangent_basis = include_tangent_basis ? kFloat3 * 2 : 0;

        switch (influences)
        {
        case 0:
            return kFloat3 + kFloat3 + tangent_basis + kFloat2;
        case 1:
            return kFloat3 + kFloat3 + tangent_basis + kFloat2 + sizeof(uint32_t);
        case 2:
            return sizeof(uint16_t) * 2 + kFloat3 + kFloat3 + tangent_basis + sizeof(float) + kFloat2;
        case 3:
        case 4:
            return sizeof(uint16_t) * influences + kFloat3 + kFloat3 + tangent_basis + sizeof(float) * (influences - 1) + kFloat2;
        default: return 0;
        }
    };

    const size_t payload_bytes = reader.size > reader.offset ? reader.size - reader.offset : 0;
    size_t bytes_per_vertex = 0;
    if (vertex_count > 0)
    {
        if (payload_bytes % vertex_count != 0)
            throw std::runtime_error("unexpected vertex payload size in OGF mesh");
        bytes_per_vertex = payload_bytes / vertex_count;
    }

    const size_t min_expected = compute_vertex_size(link_type, false);
    const size_t max_expected = compute_vertex_size(link_type, true);

    if (bytes_per_vertex < min_expected)
        throw std::runtime_error("vertex payload smaller than expected for OGF mesh");

    const bool has_tangent_basis = bytes_per_vertex >= max_expected;
    const size_t bytes_to_read = has_tangent_basis ? max_expected : min_expected;
    const size_t extra_bytes_per_vertex = bytes_per_vertex > bytes_to_read ? bytes_per_vertex - bytes_to_read : 0;

    std::vector<MeshVertex> vertices;
    vertices.reserve(vertex_count);

    for (uint32_t idx = 0; idx < vertex_count; ++idx)
    {
        MeshVertex vertex;
        std::array<uint16_t, 4> bone_ids{ 0, 0, 0, 0 };
        std::array<float, 4> raw_weights{ 0.f, 0.f, 0.f, 0.f };

        if (link_type == 0)
        {
            vertex.position = reader.read<Fvector>();
            vertex.normal = reader.read<Fvector>();
            if (has_tangent_basis)
            {
                vertex.tangent = reader.read<Fvector>();
                vertex.binormal = reader.read<Fvector>();
            }
            else
            {
                vertex.tangent.set(0.f, 0.f, 0.f);
                vertex.binormal.set(0.f, 0.f, 0.f);
            }
            vertex.uv = reader.read<Fvector2>();
        }
        else if (link_type == 1)
        {
            vertex.position = reader.read<Fvector>();
            vertex.normal = reader.read<Fvector>();
            if (has_tangent_basis)
            {
                vertex.tangent = reader.read<Fvector>();
                vertex.binormal = reader.read<Fvector>();
            }
            else
            {
                vertex.tangent.set(0.f, 0.f, 0.f);
                vertex.binormal.set(0.f, 0.f, 0.f);
            }
            vertex.uv = reader.read<Fvector2>();
            bone_ids[0] = static_cast<uint16_t>(reader.read<uint32_t>());
            raw_weights[0] = 1.f;
        }
        else if (link_type == 2)
        {
            bone_ids[0] = reader.read<uint16_t>();
            bone_ids[1] = reader.read<uint16_t>();
            vertex.position = reader.read<Fvector>();
            vertex.normal = reader.read<Fvector>();
            if (has_tangent_basis)
            {
                vertex.tangent = reader.read<Fvector>();
                vertex.binormal = reader.read<Fvector>();
            }
            else
            {
                vertex.tangent.set(0.f, 0.f, 0.f);
                vertex.binormal.set(0.f, 0.f, 0.f);
            }
            const float secondary = reader.read<float>();
            raw_weights[1] = secondary;
            raw_weights[0] = 1.f - secondary;
            vertex.uv = reader.read<Fvector2>();
        }
        else if (link_type == 3 || link_type == 4)
        {
            for (uint32_t influence = 0; influence < link_type; ++influence)
                bone_ids[influence] = reader.read<uint16_t>();

            vertex.position = reader.read<Fvector>();
            vertex.normal = reader.read<Fvector>();
            if (has_tangent_basis)
            {
                vertex.tangent = reader.read<Fvector>();
                vertex.binormal = reader.read<Fvector>();
            }
            else
            {
                vertex.tangent.set(0.f, 0.f, 0.f);
                vertex.binormal.set(0.f, 0.f, 0.f);
            }

            for (uint32_t influence = 0; influence < link_type - 1; ++influence)
                raw_weights[influence] = reader.read<float>();

            float sum = 0.f;
            for (uint32_t influence = 0; influence < link_type - 1; ++influence)
                sum += raw_weights[influence];
            raw_weights[link_type - 1] = std::max(0.f, 1.f - sum);
            vertex.uv = reader.read<Fvector2>();
        }

        finalize_influences(vertex, bone_ids, raw_weights, link_type);

        if (extra_bytes_per_vertex > 0)
            reader.skip(extra_bytes_per_vertex);

        vertices.push_back(vertex);
    }

    return vertices;
}

std::vector<uint16_t> read_mesh_indices(const Chunk& chunk)
{
    BinaryReader reader{ chunk.data, chunk.size };
    const uint32_t index_count = reader.read<u32>();
    std::vector<uint16_t> indices;
    indices.reserve(index_count);
    for (uint32_t idx = 0; idx < index_count; ++idx)
        indices.push_back(reader.read<u16>());
    return indices;
}

void rebuild_tangent_frames(std::vector<MeshVertex>& vertices, const std::vector<uint16_t>& indices)
{
    std::vector<Fvector> accumulated_tangents(vertices.size(), { 0, 0, 0 });
    std::vector<Fvector> accumulated_binormals(vertices.size(), { 0, 0, 0 });

    for (auto& value : accumulated_tangents)
        value.set(0.f, 0.f, 0.f);
    for (auto& value : accumulated_binormals)
        value.set(0.f, 0.f, 0.f);

    for (size_t tri = 0; tri + 2 < indices.size(); tri += 3)
    {
        const uint16_t i0 = indices[tri];
        const uint16_t i1 = indices[tri + 1];
        const uint16_t i2 = indices[tri + 2];

        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size())
            continue;

        const MeshVertex& v0 = vertices[i0];
        const MeshVertex& v1 = vertices[i1];
        const MeshVertex& v2 = vertices[i2];

        Fvector edge1;
        edge1.sub(v1.position, v0.position);
        Fvector edge2;
        edge2.sub(v2.position, v0.position);

        const float du1 = v1.uv.x - v0.uv.x;
        const float dv1 = v1.uv.y - v0.uv.y;
        const float du2 = v2.uv.x - v0.uv.x;
        const float dv2 = v2.uv.y - v0.uv.y;

        const float determinant = du1 * dv2 - dv1 * du2;
        if (std::fabs(determinant) <= std::numeric_limits<float>::epsilon())
            continue;

        const float inv = 1.f / determinant;

        Fvector tangent;
        tangent.mul(edge1, dv2);
        Fvector temp;
        temp.mul(edge2, dv1);
        tangent.sub(temp);
        tangent.mul(inv);

        Fvector binormal;
        binormal.mul(edge2, du1);
        temp.mul(edge1, du2);
        binormal.sub(temp);
        binormal.mul(inv);

        accumulated_tangents[i0].add(tangent);
        accumulated_tangents[i1].add(tangent);
        accumulated_tangents[i2].add(tangent);

        accumulated_binormals[i0].add(binormal);
        accumulated_binormals[i1].add(binormal);
        accumulated_binormals[i2].add(binormal);
    }

    for (size_t idx = 0; idx < vertices.size(); ++idx)
    {
        Fvector& tangent = accumulated_tangents[idx];
        Fvector& binormal = accumulated_binormals[idx];

        const float tangent_length = tangent.magnitude();
        if (tangent_length > std::numeric_limits<float>::epsilon())
        {
            tangent.mul(1.f / tangent_length);
        }
        else
        {
            tangent.set(0.f, 0.f, 0.f);
        }

        const float binormal_length = binormal.magnitude();
        if (binormal_length > std::numeric_limits<float>::epsilon())
        {
            binormal.mul(1.f / binormal_length);
        }
        else
        {
            binormal.set(0.f, 0.f, 0.f);
        }

        if (!std::isfinite(tangent.x) || !std::isfinite(tangent.y) || !std::isfinite(tangent.z))
            tangent.set(0.f, 0.f, 0.f);
        if (!std::isfinite(binormal.x) || !std::isfinite(binormal.y) || !std::isfinite(binormal.z))
            binormal.set(0.f, 0.f, 0.f);

        vertices[idx].tangent = tangent;
        vertices[idx].binormal = binormal;
    }
}

constexpr float kPositionQuantizeScale = 100000.f;
constexpr float kUVQuantizeScale = 100000.f;
constexpr float kWeightQuantizeScale = 1000000.f;
constexpr float kNormalQuantizeScale = 1000.f;

PositionQuantizedKey quantize_position(const Fvector& position)
{
    PositionQuantizedKey key;
    key.components[0] = static_cast<int32_t>(std::llround(position.x * kPositionQuantizeScale));
    key.components[1] = static_cast<int32_t>(std::llround(position.y * kPositionQuantizeScale));
    key.components[2] = static_cast<int32_t>(std::llround(position.z * kPositionQuantizeScale));
    return key;
}

RoundedNormalKey round_normal(const Fvector& normal)
{
    RoundedNormalKey key;
    key.components[0] = static_cast<int32_t>(std::llround(normal.x * kNormalQuantizeScale));
    key.components[1] = static_cast<int32_t>(std::llround(normal.y * kNormalQuantizeScale));
    key.components[2] = static_cast<int32_t>(std::llround(normal.z * kNormalQuantizeScale));
    return key;
}

VertexDedupKey make_dedup_key(const MeshVertex& vertex, uint8_t back_side_flag)
{
    VertexDedupKey key;
    key.position = quantize_position(vertex.position).components;
    key.influence_count = vertex.influence_count;
    key.back_side = back_side_flag;
    key.uv[0] = static_cast<int32_t>(std::llround(vertex.uv.x * kUVQuantizeScale));
    key.uv[1] = static_cast<int32_t>(std::llround(vertex.uv.y * kUVQuantizeScale));
    key.normal = round_normal(vertex.normal).components;
    key.tangent = round_normal(vertex.tangent).components;
    key.binormal = round_normal(vertex.binormal).components;

    for (size_t idx = 0; idx < key.bones.size(); ++idx)
    {
        if (idx < vertex.influence_count)
        {
            key.bones[idx] = vertex.bones[idx];
            key.weights[idx] = static_cast<int32_t>(std::llround(vertex.weights[idx] * kWeightQuantizeScale));
        }
        else
        {
            key.bones[idx] = 0;
            key.weights[idx] = 0;
        }
    }

    return key;
}

std::vector<uint8_t> compute_back_face_flags(const std::vector<MeshVertex>& vertices)
{
    std::unordered_map<PositionQuantizedKey, std::vector<size_t>, PositionQuantizedHasher> position_groups;
    position_groups.reserve(vertices.size());

    for (size_t index = 0; index < vertices.size(); ++index)
    {
        position_groups[quantize_position(vertices[index].position)].push_back(index);
    }

    std::vector<uint8_t> back_flags(vertices.size(), 0);

    for (const auto& group : position_groups)
    {
        const auto& indices = group.second;
        std::unordered_set<RoundedNormalKey, RoundedNormalHasher> reference_normals;
        reference_normals.reserve(indices.size());

        for (size_t vertex_index : indices)
        {
            const Fvector& normal = vertices[vertex_index].normal;
            const RoundedNormalKey rounded = round_normal(normal);
            RoundedNormalKey opposite = rounded;
            opposite.components[0] = -opposite.components[0];
            opposite.components[1] = -opposite.components[1];
            opposite.components[2] = -opposite.components[2];

            if (reference_normals.find(opposite) != reference_normals.end())
            {
                back_flags[vertex_index] = 1;
            }
            else
            {
                back_flags[vertex_index] = 0;
                reference_normals.insert(rounded);
            }
        }
    }

    return back_flags;
}

void deduplicate_vertices(std::vector<MeshVertex>& vertices, std::vector<uint16_t>& indices)
{
    if (vertices.empty())
        return;

    const size_t original_vertex_count = vertices.size();

    const std::vector<uint8_t> back_flags = compute_back_face_flags(vertices);

    std::unordered_map<VertexDedupKey, uint32_t, VertexDedupKeyHasher> map;
    map.reserve(vertices.size());

    std::vector<MeshVertex> deduplicated;
    deduplicated.reserve(vertices.size());

    std::vector<uint16_t> remap(vertices.size(), 0);
    std::vector<uint32_t> merge_counts;
    merge_counts.reserve(vertices.size());

    for (size_t idx = 0; idx < vertices.size(); ++idx)
    {
        const VertexDedupKey key = make_dedup_key(vertices[idx], back_flags[idx]);
        const auto [it, inserted] = map.emplace(key, static_cast<uint32_t>(deduplicated.size()));

        if (inserted)
        {
            deduplicated.push_back(vertices[idx]);
            merge_counts.push_back(1);
        }
        else
        {
            const uint32_t target = it->second;
            MeshVertex& existing = deduplicated[target];
            const uint32_t count = ++merge_counts[target];

            const float inv = 1.f / static_cast<float>(count);
            const float prev = static_cast<float>(count - 1);

            existing.position.x = (existing.position.x * prev + vertices[idx].position.x) * inv;
            existing.position.y = (existing.position.y * prev + vertices[idx].position.y) * inv;
            existing.position.z = (existing.position.z * prev + vertices[idx].position.z) * inv;

            existing.normal.x = (existing.normal.x * prev + vertices[idx].normal.x) * inv;
            existing.normal.y = (existing.normal.y * prev + vertices[idx].normal.y) * inv;
            existing.normal.z = (existing.normal.z * prev + vertices[idx].normal.z) * inv;

            existing.tangent.x = (existing.tangent.x * prev + vertices[idx].tangent.x) * inv;
            existing.tangent.y = (existing.tangent.y * prev + vertices[idx].tangent.y) * inv;
            existing.tangent.z = (existing.tangent.z * prev + vertices[idx].tangent.z) * inv;

            existing.binormal.x = (existing.binormal.x * prev + vertices[idx].binormal.x) * inv;
            existing.binormal.y = (existing.binormal.y * prev + vertices[idx].binormal.y) * inv;
            existing.binormal.z = (existing.binormal.z * prev + vertices[idx].binormal.z) * inv;

            existing.uv.x = (existing.uv.x * prev + vertices[idx].uv.x) * inv;
            existing.uv.y = (existing.uv.y * prev + vertices[idx].uv.y) * inv;
        }

        remap[idx] = it->second;
    }

    for (auto& vertex : deduplicated)
    {
        const float normal_length = std::sqrt(vertex.normal.x * vertex.normal.x + vertex.normal.y * vertex.normal.y + vertex.normal.z * vertex.normal.z);
        if (normal_length > std::numeric_limits<float>::epsilon())
        {
            const float inv = 1.f / normal_length;
            vertex.normal.x *= inv;
            vertex.normal.y *= inv;
            vertex.normal.z *= inv;
        }

        const float tangent_length = std::sqrt(vertex.tangent.x * vertex.tangent.x + vertex.tangent.y * vertex.tangent.y + vertex.tangent.z * vertex.tangent.z);
        if (tangent_length > std::numeric_limits<float>::epsilon())
        {
            const float inv = 1.f / tangent_length;
            vertex.tangent.x *= inv;
            vertex.tangent.y *= inv;
            vertex.tangent.z *= inv;
        }

        const float binormal_length = std::sqrt(vertex.binormal.x * vertex.binormal.x + vertex.binormal.y * vertex.binormal.y + vertex.binormal.z * vertex.binormal.z);
        if (binormal_length > std::numeric_limits<float>::epsilon())
        {
            const float inv = 1.f / binormal_length;
            vertex.binormal.x *= inv;
            vertex.binormal.y *= inv;
            vertex.binormal.z *= inv;
        }
    }

    for (uint16_t& index : indices)
    {
        if (index >= remap.size())
            throw std::runtime_error("index references vertex outside range during deduplication");
        index = remap[index];
    }

    vertices.swap(deduplicated);

    if (vertices.size() != original_vertex_count)
    {
        Msg("[ozz] Deduplicated mesh vertices: %zu -> %zu", original_vertex_count, vertices.size());
    }
}

struct ProgressiveWindow
{
    uint32_t offset = 0;
    uint16_t num_tris = 0;
    uint16_t num_verts = 0;
};

std::optional<ProgressiveWindow> ParseHighestDetailWindow(const std::vector<uint8_t>& data)
{
    const uint8_t* bytes = data.data();
    const size_t size = data.size();

    const size_t header_bytes = sizeof(uint32_t) * 5;
    const size_t entry_bytes = sizeof(uint32_t) + sizeof(uint16_t) * 2;
    if (size < header_bytes + entry_bytes)
        return std::nullopt;

    size_t offset = sizeof(uint32_t) * 4;
    uint32_t window_count = 0;
    std::memcpy(&window_count, bytes + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (window_count == 0)
        return std::nullopt;

    if (offset + entry_bytes * static_cast<size_t>(window_count) > size)
        return std::nullopt;

    ProgressiveWindow best{};
    bool best_valid = false;
    for (uint32_t idx = 0; idx < window_count; ++idx)
    {
        ProgressiveWindow current{};
        std::memcpy(&current.offset, bytes + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        std::memcpy(&current.num_tris, bytes + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        std::memcpy(&current.num_verts, bytes + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        if (!best_valid || current.offset > best.offset)
        {
            best = current;
            best_valid = true;
        }
    }

    return best_valid ? std::optional<ProgressiveWindow>(best) : std::nullopt;
}

void ApplyProgressiveWindow(SurfaceDefinition& surface)
{
    if (surface.metadata.progressive_data.empty())
        return;

    const auto window = ParseHighestDetailWindow(surface.metadata.progressive_data);
    if (!window)
        return;

    // Trim to the highest-detail window by keeping the index range described by the window
    const size_t index_offset = static_cast<size_t>(window->offset);
    const size_t index_count_size = static_cast<size_t>(window->num_tris) * 3u;
    if (index_count_size == 0)
        return;

    if (index_offset + index_count_size > surface.indices.size())
        throw std::runtime_error("progressive mesh window exceeds index buffer");

    std::vector<uint16_t> trimmed(surface.indices.begin() + index_offset,
        surface.indices.begin() + index_offset + index_count_size);
    surface.indices.swap(trimmed);
}

std::optional<SurfaceDefinition> BuildSurfaceFromChunkList(const std::vector<std::pair<uint32_t, Chunk>>& chunk_list)
{
    SurfaceDefinition surface;
    const Chunk* vertices_chunk = nullptr;
    const Chunk* indices_chunk = nullptr;

    for (const auto& entry : chunk_list)
    {
        const uint32_t id = entry.first;
        const Chunk& chunk = entry.second;

        switch (id)
        {
        case kChunkVertices:       vertices_chunk = &chunk; break;
        case kChunkIndices:        indices_chunk = &chunk; break;
        case kChunkTexture:        ParseTextureChunk(chunk, surface.metadata); break;
        case kChunkHeader:         ParseHeaderChunk(chunk, surface.metadata); break;
        case kChunkLodDefinition:
        case kChunkLodDefinition2: ParseLodChunk(chunk, surface.metadata); break;
        case kChunkChildrenLinks:  ParseChildrenLinksChunk(chunk, surface.metadata); break;
        case kChunkSwidata:        ParseProgressiveChunk(chunk, surface.metadata); break;
        default:                   break;
        }
    }

    if (!vertices_chunk || !indices_chunk)
        return std::nullopt;

    surface.vertices = read_mesh_vertices(*vertices_chunk);
    surface.indices = read_mesh_indices(*indices_chunk);

    ApplyProgressiveWindow(surface);

    surface.metadata.original_vertex_count = static_cast<uint32_t>(surface.vertices.size());
    surface.metadata.original_face_count = static_cast<uint32_t>(surface.indices.size() / 3);

    rebuild_tangent_frames(surface.vertices, surface.indices);

    return surface;
}

std::vector<SurfaceDefinition> CollectSurfaces(const std::byte* data, size_t size)
{
    std::vector<SurfaceDefinition> surfaces;

    const auto chunks = ParseChunks(data, size);

    if (const auto children_it = chunks.find(kChunkChildren); children_it != chunks.end())
    {
        const Chunk& children_chunk = children_it->second;
        BinaryReader reader{ children_chunk.data, children_chunk.size };
        while (reader.offset + sizeof(uint32_t) * 2 <= children_chunk.size)
        {
            reader.read<uint32_t>();
            const uint32_t child_size = reader.read<uint32_t>();
            if (reader.offset + child_size > children_chunk.size)
                break;

            Chunk child_chunk{ children_chunk.data + reader.offset, child_size };
            reader.offset += child_size;

            const auto child_subchunks = ParseSubchunks(child_chunk);
            if (auto surface = BuildSurfaceFromChunkList(child_subchunks))
                surfaces.push_back(std::move(*surface));
        }
    }

    if (surfaces.empty())
    {
        std::vector<std::pair<uint32_t, Chunk>> root_chunks;
        const auto add_if_present = [&](uint32_t id)
        {
            const auto it = chunks.find(id);
            if (it != chunks.end())
                root_chunks.emplace_back(id, it->second);
        };

        add_if_present(kChunkHeader);
        add_if_present(kChunkTexture);
        add_if_present(kChunkLodDefinition);
        add_if_present(kChunkLodDefinition2);
        add_if_present(kChunkChildrenLinks);
        add_if_present(kChunkSwidata);
        add_if_present(kChunkVertices);
        add_if_present(kChunkIndices);

        if (auto surface = BuildSurfaceFromChunkList(root_chunks))
            surfaces.push_back(std::move(*surface));
    }

    return surfaces;
}

ozz::sample::Mesh build_mesh(const std::vector<MeshVertex>& vertices,
    const std::vector<uint16_t>& indices,
    const std::vector<BoneRecord>& bones,
    const SurfaceMetadata& metadata,
    std::vector<BoneBoundsBuilder>* bone_bounds)
{
    std::unordered_map<uint16_t, uint16_t> joint_map;
    std::vector<uint16_t> joint_remaps;
    std::vector<ozz::math::Float4x4> inverse_bind_poses;

    std::vector<ConvertedVertex> converted_vertices(vertices.size());

    for (size_t vertex_index = 0; vertex_index < vertices.size(); ++vertex_index)
    {
        const MeshVertex& source = vertices[vertex_index];
        ConvertedVertex converted;
        converted.original_index = vertex_index;

        const auto position = ConvertVectorXrayToOzz(source.position);
        const auto normal = Normalize(ConvertVectorXrayToOzz(source.normal));
        const auto tangent = Normalize(ConvertVectorXrayToOzz(source.tangent));
        const auto binormal = Normalize(ConvertVectorXrayToOzz(source.binormal));

        converted.position = position;
        converted.normal = normal;

        const float handedness = Dot(Cross(normal, tangent), binormal) < 0.f ? -1.f : 1.f;
        converted.tangent = { tangent[0], tangent[1], tangent[2], handedness };
        const auto uv = ConvertUV(source.uv);
        converted.uv = uv;

        uint8_t influence_count = std::max<uint8_t>(1, source.influence_count);
        float weight_sum = 0.f;
        for (uint8_t influence = 0; influence < influence_count; ++influence)
        {
            const uint16_t bone_index = source.bones[influence];
            const float weight = source.weights[influence];
            weight_sum += weight;

            const auto [it, inserted] = joint_map.try_emplace(bone_index, static_cast<uint16_t>(joint_remaps.size()));
            if (inserted)
            {
                joint_remaps.push_back(bone_index);
                const auto global_matrix = ConvertXrayLocalToOzz(bones[bone_index].global_transform);
                const auto inverse_matrix = InvertMatrix(global_matrix);
                inverse_bind_poses.push_back(ToOzzFloat4x4(inverse_matrix));
            }

            converted.joint_indices[influence] = it->second;
            converted.joint_weights[influence] = weight;

            if (bone_bounds && bone_index < bones.size() && weight > std::numeric_limits<float>::epsilon())
            {
                const Fmatrix& inverse_global = bones[bone_index].inverse_global_transform;
                Fvector local_point;
                inverse_global.transform_tiny(local_point, source.position);
                (*bone_bounds)[bone_index].AddPoint(local_point);
            }
        }

        if (weight_sum <= std::numeric_limits<float>::epsilon())
        {
            converted.joint_weights[0] = 1.f;
            influence_count = 1;
        }
        else
        {
            const float inv_sum = 1.f / weight_sum;
            for (uint8_t influence = 0; influence < influence_count; ++influence)
                converted.joint_weights[influence] *= inv_sum;
        }

        converted.influence_count = std::min<uint8_t>(4, influence_count);
        converted_vertices[vertex_index] = converted;
    }

    ozz::sample::Mesh mesh;
    const size_t vertex_count = converted_vertices.size();

    ozz::sample::Mesh::Part part;
    part.positions.resize(vertex_count * ozz::sample::Mesh::Part::kPositionsCpnts);
    part.normals.resize(vertex_count * ozz::sample::Mesh::Part::kNormalsCpnts);
    part.tangents.resize(vertex_count * ozz::sample::Mesh::Part::kTangentsCpnts);
    part.uvs.resize(vertex_count * ozz::sample::Mesh::Part::kUVsCpnts);
    part.colors.resize(vertex_count * ozz::sample::Mesh::Part::kColorsCpnts, 0u);
    part.joint_indices.resize(vertex_count * 4u);
    part.joint_weights.resize(vertex_count * 3u);

    for (size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
    {
        const ConvertedVertex& v = converted_vertices[vertex_index];

        part.positions[vertex_index * 3 + 0] = v.position[0];
        part.positions[vertex_index * 3 + 1] = v.position[1];
        part.positions[vertex_index * 3 + 2] = v.position[2];

        part.normals[vertex_index * 3 + 0] = v.normal[0];
        part.normals[vertex_index * 3 + 1] = v.normal[1];
        part.normals[vertex_index * 3 + 2] = v.normal[2];

        part.tangents[vertex_index * 4 + 0] = v.tangent[0];
        part.tangents[vertex_index * 4 + 1] = v.tangent[1];
        part.tangents[vertex_index * 4 + 2] = v.tangent[2];
        part.tangents[vertex_index * 4 + 3] = v.tangent[3];

        part.uvs[vertex_index * 2 + 0] = v.uv[0];
        part.uvs[vertex_index * 2 + 1] = v.uv[1];

        for (int influence = 0; influence < 4; ++influence)
        {
            part.joint_indices[vertex_index * 4 + influence] = v.joint_indices[static_cast<size_t>(influence)];
            if (influence < 3)
            {
                part.joint_weights[vertex_index * 3 + influence] = v.joint_weights[static_cast<size_t>(influence)];
            }
        }
    }

    mesh.parts.push_back(std::move(part));

    mesh.triangle_indices.resize(indices.size());
    for (size_t idx = 0; idx < indices.size(); ++idx)
    {
        const uint16_t original = indices[idx];
        if (original >= vertex_count)
            throw std::runtime_error("index references vertex outside range");
        mesh.triangle_indices[idx] = original;
    }

    mesh.joint_remaps.resize(joint_remaps.size());
    std::copy(joint_remaps.begin(), joint_remaps.end(), mesh.joint_remaps.begin());

    mesh.inverse_bind_poses.resize(inverse_bind_poses.size());
    std::copy(inverse_bind_poses.begin(), inverse_bind_poses.end(), mesh.inverse_bind_poses.begin());

    mesh.xray_metadata.texture_path = metadata.texture_path;
    mesh.xray_metadata.shader_name = metadata.shader_name;
    mesh.xray_metadata.texture_link_present = false;
    mesh.xray_metadata.texture_link = 0;
    mesh.xray_metadata.shader_link_present = false;
    mesh.xray_metadata.shader_link = 0;
    mesh.xray_metadata.original_vertex_count = metadata.original_vertex_count;
    mesh.xray_metadata.original_face_count = metadata.original_face_count;
    mesh.xray_metadata.ogf_type = metadata.ogf_type;
    mesh.xray_metadata.lod_visuals = metadata.lod_visuals;
    mesh.xray_metadata.lod_data = metadata.lod_data;
    mesh.xray_metadata.progressive_collapse_count = metadata.progressive_collapse_count;
    mesh.xray_metadata.progressive_data = metadata.progressive_data;
    mesh.xray_metadata.child_visual_links = metadata.child_visual_links;
    mesh.xray_metadata.original_to_remapped.resize(vertex_count);
    std::iota(mesh.xray_metadata.original_to_remapped.begin(), mesh.xray_metadata.original_to_remapped.end(), 0u);
    mesh.xray_metadata.remapped_to_original.resize(vertex_count);
    std::iota(mesh.xray_metadata.remapped_to_original.begin(), mesh.xray_metadata.remapped_to_original.end(), 0u);

    return mesh;
}

void serialize_mesh(const ozz::sample::Mesh& mesh, std::vector<uint8_t>& out_binary)
{
    ozz::io::MemoryStream stream;
    ozz::io::OArchive archive(&stream);
    archive << mesh;

    out_binary.resize(stream.Size());
    stream.Seek(0, ozz::io::Stream::kSet);
    if (!out_binary.empty())
        stream.Read(out_binary.data(), out_binary.size());
}

void serialize_skeleton(const ozz::animation::Skeleton& skeleton, std::vector<uint8_t>& out_binary)
{
    ozz::io::MemoryStream stream;
    ozz::io::OArchive archive(&stream);
    archive << skeleton;

    out_binary.resize(stream.Size());
    stream.Seek(0, ozz::io::Stream::kSet);
    if (!out_binary.empty())
        stream.Read(out_binary.data(), out_binary.size());
}

void ConvertLegacyVisualToOzzBundleImpl(const LegacyVisualInput& input,
    const LegacyVisualConversionOptions& options,
    LegacyVisualConversionResult& out_result)
{
    if (input.buffer.empty())
        throw std::runtime_error("legacy visual buffer is empty");

    const std::byte* data = input.buffer.data();
    const size_t size = input.buffer.size();

    const auto chunks = ParseChunks(data, size);

    std::vector<BoneRecord> bones;
    const bool need_skeleton = options.build_skeleton || options.build_mesh;

    if (need_skeleton)
    {
        bones = LoadSkeletonBonesFromOgf(data, size);
        auto raw_skeleton = BuildRawSkeleton(bones);

        ozz::animation::offline::SkeletonBuilder skeleton_builder;
        auto skeleton = skeleton_builder(raw_skeleton);
        if (!skeleton)
            throw std::runtime_error("ozz skeleton build failed");

        out_result.skeleton = std::move(skeleton);

        out_result.bone_names.clear();
        out_result.bone_names.reserve(bones.size());
        for (const auto& bone : bones)
            out_result.bone_names.emplace_back(bone.name.c_str());

        out_result.bones.clear();
        out_result.bones.reserve(bones.size());
        for (const auto& bone : bones)
        {
            LegacyOgfBone entry;
            entry.name = bone.name.c_str();
            entry.parent_name = bone.parent_name.c_str();
            entry.local_transform = bone.local_transform;
            entry.global_transform = bone.global_transform;
            out_result.bones.emplace_back(std::move(entry));
        }

        out_result.bone_metadata.clear();
        out_result.bone_metadata.reserve(bones.size());
        for (const auto& bone : bones)
        {
            ExtendedBoneMetadata metadata;
            metadata.shape = bone.shape;
            metadata.obb = bone.obb;
            metadata.joint = bone.joint_ik;
            metadata.game_material = bone.game_material.c_str();
            metadata.mass = bone.mass;
            metadata.center_of_mass = bone.center_of_mass;
            metadata.rest_length = bone.rest_length;
            metadata.dominant_axis = bone.dominant_axis;
            metadata.local_aabb_min = bone.local_aabb_min;
            metadata.local_aabb_max = bone.local_aabb_max;
            metadata.inverse_global_transform = bone.inverse_global_transform;
            metadata.inertia_tensor = bone.inertia_tensor;
            metadata.volume = bone.volume;
            metadata.collision_layers.assign(bone.collision_layers);
            metadata.ground_contact_candidate = bone.ground_contact_candidate;
            metadata.weapon_anchor_candidate = bone.weapon_anchor_candidate;
            out_result.bone_metadata.emplace_back(std::move(metadata));
        }

        if (options.build_skeleton)
        {
            serialize_skeleton(*out_result.skeleton, out_result.skeleton_binary);
        }
        else
        {
            out_result.skeleton_binary.clear();
        }
    }
    else
    {
        out_result.skeleton.reset();
        out_result.bone_names.clear();
        out_result.bones.clear();
        out_result.skeleton_binary.clear();
        out_result.bone_metadata.clear();
    }

    if (options.build_mesh)
    {
        auto surfaces = CollectSurfaces(data, size);
        if (surfaces.empty())
            throw std::runtime_error("OGF file does not contain any mesh surfaces");

        std::vector<ozz::sample::Mesh> meshes;
        meshes.reserve(surfaces.size());

        std::vector<BoneBoundsBuilder> mesh_bounds;
        if (!bones.empty())
            mesh_bounds.resize(bones.size());

        for (auto& surface : surfaces)
        {
            if (options.deduplicate_vertices)
                deduplicate_vertices(surface.vertices, surface.indices);

            meshes.push_back(build_mesh(surface.vertices, surface.indices, bones, surface.metadata,
                mesh_bounds.empty() ? nullptr : &mesh_bounds));
        }

        if (!mesh_bounds.empty())
        {
            constexpr float kMinHalfExtent = 0.001f; // 1 mm minimum to keep physics happy
            for (size_t idx = 0; idx < bones.size(); ++idx)
            {
                BoneRecord& bone = bones[idx];
                const BoneBoundsBuilder& bounds = mesh_bounds[idx];
                if (!bounds.has_points)
                    continue;

                Fobb obb;
                obb.m_rotate.identity();
                obb.m_translate.add(bounds.min, bounds.max);
                obb.m_translate.mul(0.5f);
                obb.m_halfsize.sub(bounds.max, bounds.min);
                obb.m_halfsize.mul(0.5f);
                obb.m_halfsize.x = std::max(obb.m_halfsize.x, kMinHalfExtent);
                obb.m_halfsize.y = std::max(obb.m_halfsize.y, kMinHalfExtent);
                obb.m_halfsize.z = std::max(obb.m_halfsize.z, kMinHalfExtent);

                bone.obb = obb;
                if (idx < out_result.bone_metadata.size())
                    out_result.bone_metadata[idx].obb = obb;
            }
        }

        out_result.mesh_surface_count = meshes.size();

        ozz::io::MemoryStream mesh_stream;
        ozz::io::OArchive mesh_archive(&mesh_stream);
        for (const auto& mesh : meshes)
            mesh_archive << mesh;

        out_result.mesh_binary.resize(mesh_stream.Size());
        mesh_stream.Seek(0, ozz::io::Stream::kSet);
        if (!out_result.mesh_binary.empty())
            mesh_stream.Read(out_result.mesh_binary.data(), out_result.mesh_binary.size());
    }
    else
    {
        out_result.mesh_binary.clear();
        out_result.mesh_surface_count = 0;
    }

    const auto motion_refs = LoadMotionRefsFromOgf(data, size);
    out_result.motion_refs.clear();
    out_result.motion_refs.reserve(motion_refs.size());
    for (const auto& ref : motion_refs)
        out_result.motion_refs.emplace_back(ref.c_str());

    if (const auto user_data_it = chunks.find(kChunkUserData); user_data_it != chunks.end())
    {
        const Chunk& chunk = user_data_it->second;
        out_result.user_data.resize(chunk.size);
        if (chunk.size > 0)
            std::memcpy(out_result.user_data.data(), chunk.data, chunk.size);
    }
    else
    {
        out_result.user_data.clear();
    }

    out_result.embedded_animation_binary.clear();
    const auto smparams_it = chunks.find(kChunkSmparams);
    const auto motions_it = chunks.find(kChunkMotions);
    if (smparams_it != chunks.end() && motions_it != chunks.end() && out_result.skeleton)
    {
        xr_vector<ConvertedOmfAnimation> embedded;
        if (ConvertLegacyOmf(data, size, out_result.bone_names, *out_result.skeleton, embedded))
        {
            embedded.erase(std::remove_if(embedded.begin(), embedded.end(),
                            [](const ConvertedOmfAnimation& entry)
                            {
                                return !entry.animation;
                            }),
                embedded.end());

            Msg("create legacy animation for %s", input.source_path.value().generic_string().c_str());
            out_result.embedded_animation_binary = SerializeEmbeddedAnimations(embedded);
        }
    }

    // Determine model type based on original OGF header type
    std::uint8_t original_ogf_type = 0;
    if (const auto header_it = chunks.find(kChunkHeader); header_it != chunks.end())
    {
        const Chunk& header_chunk = header_it->second;
        if (header_chunk.size >= sizeof(ogf_header))
        {
            ogf_header header{};
            std::memcpy(&header, header_chunk.data, sizeof(header));
            original_ogf_type = header.type;
        }
    }

    // Map legacy MT types to ozz MT types
    if (original_ogf_type == MT_SKELETON_ANIM)
    {
        out_result.model_type = MT_OZZ_ANIMATED;
    }
    else if (original_ogf_type == MT_SKELETON_RIGID)
    {
        out_result.model_type = MT_OZZ_STATIC;
    }
    else
    {
        // Fallback: check if we have animations
        const bool has_animations = !out_result.motion_refs.empty() || !out_result.embedded_animation_binary.empty();
        out_result.model_type = has_animations ? MT_OZZ_ANIMATED : MT_OZZ_STATIC;
    }

}

bool ConvertLegacyVisualToOzzBundle(const LegacyVisualInput& input,
                                    LegacyVisualConversionResult& out_result,
                                    const LegacyVisualConversionOptions& options,
                                    xr_string* out_error)
{
    try
    {
        ConvertLegacyVisualToOzzBundleImpl(input, options, out_result);
        if (out_error)
            out_error->clear();
        return true;
    }
    catch (const std::exception& ex)
    {
        if (out_error)
            *out_error = ex.what();
        return false;
    }
}

bool ConvertLegacyVisualToOzzBundle(const std::filesystem::path& ogf_path,
                                    LegacyVisualConversionResult& out_result,
                                    const LegacyVisualConversionOptions& options,
                                    xr_string* out_error)
{
    try
    {
        const auto file_bytes = LoadFileBytes(ogf_path);
        LegacyVisualInput input;
        input.source_path = ogf_path;
        input.buffer = ozz::span<const std::byte>(file_bytes.data(), file_bytes.size());

        ConvertLegacyVisualToOzzBundleImpl(input, options, out_result);
        if (out_error)
            out_error->clear();
        return true;
    }
    catch (const std::exception& ex)
    {
        if (out_error)
            *out_error = ex.what();
        return false;
    }
}

} // namespace Animation
} // namespace XRay
