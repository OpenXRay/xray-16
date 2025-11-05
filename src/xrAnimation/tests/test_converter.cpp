#include "Common/Platform.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

#include "xrCore/xrCore.h"

#ifndef PROJECT_ROOT
#    define PROJECT_ROOT ""
#endif

#ifdef _WIN32
#    ifndef NOMINMAX
#        define NOMINMAX // prevent Windows headers from defining min/max macros
#    endif
#    include <windows.h>
#else
#    include <sys/wait.h>
#endif

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/transform.h"

#include "LegacyOgfConverter.h"
#include "LegacyOmfConverter.h"
#include "../../../Externals/ozz-animation/samples/framework/mesh.h"
#include "../../../Externals/ozz-animation/src/animation/offline/gltf/extern/json.hpp"

#include "OzzBundle.h"

namespace fs = std::filesystem;

namespace
{
using Json = nlohmann::json;

std::string PathToString(const fs::path& path)
{
    return path.generic_string();
}

fs::path ProjectRoot()
{
    static const fs::path root = []
    {
#ifdef PROJECT_ROOT
        fs::path from_macro(PROJECT_ROOT);
        if (!from_macro.empty())
        {
#ifdef _WIN32
            from_macro.make_preferred();
#endif
            return from_macro;
        }
#endif

        fs::path from_file(__FILE__);
        auto root_path = from_file.parent_path();
        root_path = root_path.parent_path();
        root_path = root_path.parent_path();
        root_path = root_path.parent_path();
#ifdef _WIN32
        root_path.make_preferred();
#endif
        return root_path;
    }();
    return root;
}

fs::path ResolveProjectPath(const std::string& relative)
{
    fs::path path = ProjectRoot() / relative;
#ifdef _WIN32
    path.make_preferred();
#endif
    return path;
}

struct ChunkView
{
    uint32_t id = 0;
    const uint8_t* data = nullptr;
    size_t size = 0;
};

struct OgfSurfaceStats
{
    uint32_t vertex_count = 0;
    uint32_t face_count = 0;
};

struct OgfSurfaceGeometry
{
    uint32_t vertex_count = 0;
    std::vector<uint32_t> indices;
};

constexpr uint32_t OGF_HEADER = 1;
constexpr uint32_t OGF_VERTICES = 3;
constexpr uint32_t OGF_INDICES = 4;
constexpr uint32_t OGF_CHILDREN = 9;
constexpr uint32_t OGF_SWIDATA = 6;

struct ProgressiveWindow
{
    uint32_t offset = 0;
    uint16_t num_tris = 0;
    uint16_t num_verts = 0;
};

std::optional<ProgressiveWindow> ParseHighestDetailWindow(const uint8_t* data, size_t size)
{
    const size_t header_bytes = sizeof(uint32_t) * 5;
    const size_t entry_bytes = sizeof(uint32_t) + sizeof(uint16_t) * 2;
    if (!data || size < header_bytes + entry_bytes)
        return std::nullopt;

    size_t offset = sizeof(uint32_t) * 4;
    uint32_t window_count = 0;
    std::memcpy(&window_count, data + offset, sizeof(uint32_t));
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
        std::memcpy(&current.offset, data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        std::memcpy(&current.num_tris, data + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        std::memcpy(&current.num_verts, data + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        if (!best_valid || current.offset > best.offset)
        {
            best = current;
            best_valid = true;
        }
    }

    return best_valid ? std::optional<ProgressiveWindow>(best) : std::nullopt;
}

bool ReadBinaryFile(const fs::path& path, std::vector<uint8_t>& out_buffer)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
    {
        std::cerr << "failed to open binary file: " << path << std::endl;
        return false;
    }

    const auto size = stream.tellg();
    if (size <= 0)
    {
        std::cerr << "binary file empty: " << path << std::endl;
        return false;
    }

    out_buffer.resize(static_cast<size_t>(size));
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(out_buffer.data()), out_buffer.size());
    if (!stream)
    {
        std::cerr << "failed to read binary file: " << path << std::endl;
        return false;
    }

    return true;
}

bool ParseChunkSequence(const uint8_t* data, size_t size, std::vector<ChunkView>& out_chunks)
{
    out_chunks.clear();

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
        {
            std::cerr << "chunk extends beyond parent size" << std::endl;
            return false;
        }

        out_chunks.push_back(ChunkView{ id, data + offset, static_cast<size_t>(chunk_size) });
        offset += chunk_size;
    }

    return true;
}

bool ExtractSurfaceStatsFromSections(const std::vector<ChunkView>& sections, OgfSurfaceStats& stats)
{
    bool has_vertices = false;
    bool has_indices = false;
    const ChunkView* swidata_chunk = nullptr;
    for (const ChunkView& section : sections)
    {
        if (section.id == OGF_VERTICES)
        {
            size_t offset = 0;
            if (section.size < sizeof(uint32_t) * 2)
            {
                std::cerr << "vertex chunk too small" << std::endl;
                return false;
            }

            offset += sizeof(uint32_t); // skip vertex format / type
            uint32_t vertex_count = 0;
            std::memcpy(&vertex_count, section.data + offset, sizeof(uint32_t));
            stats.vertex_count = vertex_count;
            has_vertices = true;
        }
        else if (section.id == OGF_INDICES)
        {
            if (section.size < sizeof(uint32_t))
            {
                std::cerr << "index chunk too small" << std::endl;
                return false;
            }

            uint32_t index_count = 0;
            std::memcpy(&index_count, section.data, sizeof(uint32_t));
            if (index_count % 3 != 0)
            {
                std::cerr << "index count not divisible by 3" << std::endl;
                return false;
            }
            stats.face_count = index_count / 3;
            has_indices = true;
        }
        else if (section.id == OGF_SWIDATA)
        {
            swidata_chunk = &section;
        }
    }

    if (has_vertices && has_indices && swidata_chunk)
    {
        if (auto window = ParseHighestDetailWindow(swidata_chunk->data, swidata_chunk->size))
        {
            if (window->num_tris > 0)
                stats.face_count = window->num_tris;
        }
    }

    return has_vertices && has_indices;
}

bool ExtractSurfaceGeometryFromSections(const std::vector<ChunkView>& sections, OgfSurfaceGeometry& geometry)
{
    bool has_vertices = false;
    bool has_indices = false;
    const ChunkView* swidata_chunk = nullptr;
    geometry.vertex_count = 0;
    geometry.indices.clear();

    for (const ChunkView& section : sections)
    {
        if (section.id == OGF_VERTICES)
        {
            size_t offset = 0;
            if (section.size < sizeof(uint32_t) * 2)
            {
                std::cerr << "vertex chunk too small" << std::endl;
                return false;
            }

            offset += sizeof(uint32_t);
            uint32_t vertex_count = 0;
            std::memcpy(&vertex_count, section.data + offset, sizeof(uint32_t));
            geometry.vertex_count = vertex_count;
            has_vertices = true;
        }
        else if (section.id == OGF_INDICES)
        {
            if (section.size < sizeof(uint32_t))
            {
                std::cerr << "index chunk too small" << std::endl;
                return false;
            }

            uint32_t index_count = 0;
            std::memcpy(&index_count, section.data, sizeof(uint32_t));
            const size_t payload_bytes = section.size - sizeof(uint32_t);

            const size_t expected_u16 = static_cast<size_t>(index_count) * sizeof(uint16_t);
            const size_t expected_u32 = static_cast<size_t>(index_count) * sizeof(uint32_t);

            const uint8_t* cursor = section.data + sizeof(uint32_t);
            geometry.indices.resize(index_count);

            if (payload_bytes == expected_u16)
            {
                for (uint32_t i = 0; i < index_count; ++i)
                {
                    uint16_t value = 0;
                    std::memcpy(&value, cursor + i * sizeof(uint16_t), sizeof(uint16_t));
                    geometry.indices[i] = value;
                }
            }
            else if (payload_bytes == expected_u32)
            {
                for (uint32_t i = 0; i < index_count; ++i)
                {
                    uint32_t value = 0;
                    std::memcpy(&value, cursor + i * sizeof(uint32_t), sizeof(uint32_t));
                    geometry.indices[i] = value;
                }
            }
            else
            {
                std::cerr << "unexpected index payload size: " << payload_bytes << " bytes for " << index_count << " indices" << std::endl;
                return false;
            }

            has_indices = true;
        }
        else if (section.id == OGF_SWIDATA)
        {
            swidata_chunk = &section;
        }
    }

    if (has_vertices && has_indices && swidata_chunk)
    {
        if (auto window = ParseHighestDetailWindow(swidata_chunk->data, swidata_chunk->size))
        {
            const size_t index_offset = static_cast<size_t>(window->offset);
            const size_t index_count = static_cast<size_t>(window->num_tris) * 3u;

            if (index_count > 0)
            {
                if (index_offset + index_count > geometry.indices.size())
                {
                    std::cerr << "progressive mesh window exceeds index buffer" << std::endl;
                    return false;
                }

                std::vector<uint32_t> trimmed(geometry.indices.begin() + index_offset, geometry.indices.begin() + index_offset + index_count);
                geometry.indices.swap(trimmed);
            }
        }
    }

    return has_vertices && has_indices;
}

bool LoadOgfSurfaceStats(const fs::path& path, std::vector<OgfSurfaceStats>& surfaces)
{
    std::vector<uint8_t> buffer;
    if (!ReadBinaryFile(path, buffer))
        return false;

    std::vector<ChunkView> root_chunks;
    if (!ParseChunkSequence(buffer.data(), buffer.size(), root_chunks))
        return false;

    surfaces.clear();

    const auto children_it = std::find_if(root_chunks.begin(), root_chunks.end(),
        [](const ChunkView& chunk)
        {
            return chunk.id == OGF_CHILDREN;
        });

    if (children_it != root_chunks.end())
    {
        std::vector<ChunkView> child_chunks;
        if (!ParseChunkSequence(children_it->data, children_it->size, child_chunks))
            return false;

        for (const ChunkView& child : child_chunks)
        {
            std::vector<ChunkView> sections;
            if (!ParseChunkSequence(child.data, child.size, sections))
                return false;

            OgfSurfaceStats stats;
            if (!ExtractSurfaceStatsFromSections(sections, stats))
                return false;
            surfaces.push_back(stats);
        }

        if (!surfaces.empty())
            return true;
    }

    OgfSurfaceStats root_surface;
    if (ExtractSurfaceStatsFromSections(root_chunks, root_surface))
    {
        surfaces.push_back(root_surface);
        return true;
    }

    std::cerr << "failed to locate any surfaces in OGF file: " << path << std::endl;
    return false;
}

bool LoadOgfSurfaceGeometry(const fs::path& path, std::vector<OgfSurfaceGeometry>& surfaces)
{
    std::vector<uint8_t> buffer;
    if (!ReadBinaryFile(path, buffer))
        return false;

    std::vector<ChunkView> root_chunks;
    if (!ParseChunkSequence(buffer.data(), buffer.size(), root_chunks))
        return false;

    surfaces.clear();

    const auto children_it = std::find_if(root_chunks.begin(), root_chunks.end(),
        [](const ChunkView& chunk)
        {
            return chunk.id == OGF_CHILDREN;
        });

    if (children_it != root_chunks.end())
    {
        std::vector<ChunkView> child_chunks;
        if (!ParseChunkSequence(children_it->data, children_it->size, child_chunks))
            return false;

        for (const ChunkView& child : child_chunks)
        {
            std::vector<ChunkView> sections;
            if (!ParseChunkSequence(child.data, child.size, sections))
                return false;

            OgfSurfaceGeometry geometry;
            if (!ExtractSurfaceGeometryFromSections(sections, geometry))
                return false;
            surfaces.push_back(std::move(geometry));
        }

        if (!surfaces.empty())
            return true;
    }

    OgfSurfaceGeometry root_geometry;
    if (ExtractSurfaceGeometryFromSections(root_chunks, root_geometry))
    {
        surfaces.push_back(std::move(root_geometry));
        return true;
    }

    std::cerr << "failed to locate any geometry in OGF file: " << path << std::endl;
    return false;
}

bool LoadMeshArchive(const fs::path& path, std::vector<ozz::sample::Mesh>& meshes)
{
    ozz::io::File file(path.string().c_str(), "rb");
    if (!file.opened())
    {
        std::cerr << "failed to open mesh archive: " << path << std::endl;
        return false;
    }

    ozz::io::IArchive archive(&file);
    meshes.clear();

    while (archive.TestTag<ozz::sample::Mesh>())
    {
        meshes.emplace_back();
        archive >> meshes.back();
    }

    if (meshes.empty())
    {
        std::cerr << "mesh archive contained no meshes: " << path << std::endl;
        return false;
    }

    return true;
}

fs::path TestArtifactsDir()
{
    return ProjectRoot() / "src" / "xrAnimation" / "tests" / "testdata";
}

fs::path SkeletonInputPath()
{
    return ProjectRoot() / "res" / "testdata" / "npc" / "stalker_hero_1.ogf";
}

fs::path AnimationInputPath()
{
    return ProjectRoot() / "res" / "testdata" / "npc" / "critical_hit_grup_1.omf";
}

fs::path SkeletonOutputPath()
{
    return TestArtifactsDir() / "stalker_hero_1.ozz";
}

fs::path SkeletonCsvPath()
{
    return TestArtifactsDir() / "stalker_hero_bind_pose.csv";
}

fs::path AnimationOutputPath()
{
    return TestArtifactsDir() / "critical_hit_grup_1.ozz";
}

fs::path AnimationMetadataPath()
{
    return TestArtifactsDir() / "critical_hit_grup_1.json";
}

fs::path SingleAnimationOutputPath()
{
    return TestArtifactsDir() / "critical_hit_grup_1_single.ozz";
}

fs::path SingleAnimationOptimizedOutputPath()
{
    return TestArtifactsDir() / "critical_hit_grup_1_single_optimized.ozz";
}

fs::path MeshOutputPath()
{
    return TestArtifactsDir() / "stalker_hero_mesh.ozz";
}

fs::path BundleOutputPath()
{
    return TestArtifactsDir() / "stalker_hero.ozzx";
}

constexpr float kMeshPositionTolerance = 1e-4f;
constexpr float kMeshWeightTolerance = 5e-4f;
constexpr float kMatrixTolerance = 1e-4f;
constexpr int kExpectedStalkerHeroVertexCount = 3237; // Blender export baseline

struct BaselineVertex
{
    std::array<double, 3> position{}; // Ozz basis
    std::vector<std::pair<int, double>> weights;
    int mesh_local_index = -1;
};

int64_t QuantizeToScaledInt(double value, double scale)
{
    return static_cast<int64_t>(std::llround(value * scale));
}

const std::array<const char*, 4> kExpectedMultiMotionNames = {
    {
     "norm_2_critical_hit_hend_left_0", "norm_2_critical_hit_hend_right_0",
     "norm_2_critical_hit_torso_0", "norm_2_critical_hit_torso_1",
     }
};

std::string ReadString(ozz::io::IArchive& archive)
{
    uint32_t length = 0;
    archive >> length;
    std::string value;
    value.resize(length);
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

bool ReadSerializedMotionMetadata(ozz::io::IArchive& archive, SerializedMotionMetadata* metadata_out)
{
    SerializedMotionMetadata metadata;

    metadata.name = ReadString(archive);

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
    for (uint32_t mark_index = 0; mark_index < mark_count; ++mark_index)
    {
        SerializedMotionMark mark;
        mark.name = ReadString(archive);

        uint32_t interval_count = 0;
        archive >> interval_count;
        mark.intervals.reserve(interval_count);
        for (uint32_t interval_index = 0; interval_index < interval_count; ++interval_index)
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

    if (metadata_out)
        *metadata_out = std::move(metadata);

    return true;
}

uint8_t DetermineTranslationFormat(const XRay::Animation::ConvertedBoneMotion& bone)
{
    if (!bone.translation_keys16.empty())
        return 2;
    if (!bone.translation_keys8.empty())
        return 1;
    return 0;
}

bool CompareBoneMotion(const SerializedBoneMotion& metadata_bone, const XRay::Animation::ConvertedBoneMotion& expected_bone,
    const std::string& motion_name)
{
    bool ok = true;
    const uint16_t bone_id = metadata_bone.bone_id;

    if (metadata_bone.flags != expected_bone.flags)
    {
        std::cerr << "motion '" << motion_name << "' bone " << bone_id << " flags mismatch: metadata="
                  << static_cast<int>(metadata_bone.flags) << " expected=" << static_cast<int>(expected_bone.flags) << std::endl;
        ok = false;
    }

    if (metadata_bone.rotation_crc != expected_bone.rotation_crc)
    {
        std::cerr << "motion '" << motion_name << "' bone " << bone_id << " rotation CRC mismatch: metadata="
                  << metadata_bone.rotation_crc << " expected=" << expected_bone.rotation_crc << std::endl;
        ok = false;
    }

    if (metadata_bone.translation_crc != expected_bone.translation_crc)
    {
        std::cerr << "motion '" << motion_name << "' bone " << bone_id << " translation CRC mismatch: metadata="
                  << metadata_bone.translation_crc << " expected=" << expected_bone.translation_crc << std::endl;
        ok = false;
    }

    const uint8_t expected_format = DetermineTranslationFormat(expected_bone);
    if (metadata_bone.translation_format != expected_format)
    {
        std::cerr << "motion '" << motion_name << "' bone " << bone_id << " translation format mismatch: metadata="
                  << static_cast<int>(metadata_bone.translation_format) << " expected=" << static_cast<int>(expected_format) << std::endl;
        ok = false;
    }

    if (metadata_bone.rotation_keys.size() != expected_bone.rotation_keys.size())
    {
        std::cerr << "motion '" << motion_name << "' bone " << bone_id << " rotation key count mismatch: metadata="
                  << metadata_bone.rotation_keys.size() << " expected=" << expected_bone.rotation_keys.size() << std::endl;
        ok = false;
    }
    else
    {
        for (size_t idx = 0; idx < metadata_bone.rotation_keys.size(); ++idx)
        {
            const CKeyQR& actual = metadata_bone.rotation_keys[idx];
            const CKeyQR& expected = expected_bone.rotation_keys[idx];
            if (actual.x != expected.x || actual.y != expected.y || actual.z != expected.z || actual.w != expected.w)
            {
                std::cerr << "motion '" << motion_name << "' bone " << bone_id << " rotation key mismatch at index " << idx << std::endl;
                ok = false;
                break;
            }
        }
    }

    auto compare_translation_keys8 = [&](const std::vector<CKeyQT8>& metadata_keys, const xr_vector<CKeyQT8>& expected_keys)
    {
        if (metadata_keys.size() != expected_keys.size())
        {
            std::cerr << "motion '" << motion_name << "' bone " << bone_id << " translation key count mismatch (8-bit): metadata="
                      << metadata_keys.size() << " expected=" << expected_keys.size() << std::endl;
            ok = false;
            return;
        }
        for (size_t idx = 0; idx < metadata_keys.size(); ++idx)
        {
            const CKeyQT8& actual = metadata_keys[idx];
            const CKeyQT8& expected = expected_keys[idx];
            if (actual.x1 != expected.x1 || actual.y1 != expected.y1 || actual.z1 != expected.z1)
            {
                std::cerr << "motion '" << motion_name << "' bone " << bone_id << " translation key mismatch (8-bit) at index " << idx << std::endl;
                ok = false;
                break;
            }
        }
    };

    auto compare_translation_keys16 = [&](const std::vector<CKeyQT16>& metadata_keys, const xr_vector<CKeyQT16>& expected_keys)
    {
        if (metadata_keys.size() != expected_keys.size())
        {
            std::cerr << "motion '" << motion_name << "' bone " << bone_id << " translation key count mismatch (16-bit): metadata="
                      << metadata_keys.size() << " expected=" << expected_keys.size() << std::endl;
            ok = false;
            return;
        }
        for (size_t idx = 0; idx < metadata_keys.size(); ++idx)
        {
            const CKeyQT16& actual = metadata_keys[idx];
            const CKeyQT16& expected = expected_keys[idx];
            if (actual.x1 != expected.x1 || actual.y1 != expected.y1 || actual.z1 != expected.z1)
            {
                std::cerr << "motion '" << motion_name << "' bone " << bone_id << " translation key mismatch (16-bit) at index " << idx << std::endl;
                ok = false;
                break;
            }
        }
    };

    switch (expected_format)
    {
    case 1:
        compare_translation_keys8(metadata_bone.translation_keys8, expected_bone.translation_keys8);
        if (!metadata_bone.translation_keys16.empty())
        {
            std::cerr << "motion '" << motion_name << "' bone " << bone_id << " unexpected 16-bit translation keys present" << std::endl;
            ok = false;
        }
        break;
    case 2:
        compare_translation_keys16(metadata_bone.translation_keys16, expected_bone.translation_keys16);
        if (!metadata_bone.translation_keys8.empty())
        {
            std::cerr << "motion '" << motion_name << "' bone " << bone_id << " unexpected 8-bit translation keys present" << std::endl;
            ok = false;
        }
        break;
    default:
        if (!metadata_bone.translation_keys8.empty() || !metadata_bone.translation_keys16.empty())
        {
            std::cerr << "motion '" << motion_name << "' bone " << bone_id << " unexpected translation keys present for static track" << std::endl;
            ok = false;
        }
        break;
    }

    constexpr float kFloatTolerance = 1e-4f;
    auto check_float = [&](float lhs, float rhs, const char* label)
    {
        if (std::fabs(lhs - rhs) > kFloatTolerance)
        {
            std::cerr << "motion '" << motion_name << "' bone " << bone_id << " " << label << " mismatch: metadata=" << lhs << " expected=" << rhs
                      << std::endl;
            ok = false;
        }
    };

    check_float(metadata_bone.translation_size.x, expected_bone.translation_size.x, "translation_size.x");
    check_float(metadata_bone.translation_size.y, expected_bone.translation_size.y, "translation_size.y");
    check_float(metadata_bone.translation_size.z, expected_bone.translation_size.z, "translation_size.z");
    check_float(metadata_bone.translation_init.x, expected_bone.translation_init.x, "translation_init.x");
    check_float(metadata_bone.translation_init.y, expected_bone.translation_init.y, "translation_init.y");
    check_float(metadata_bone.translation_init.z, expected_bone.translation_init.z, "translation_init.z");

    return ok;
}

bool LoadAnimationByName(const fs::path& path, const std::string& motion_name, ozz::animation::Animation& animation_out)
{
    ozz::io::File file(path.string().c_str(), "rb");
    if (!file.opened())
    {
        std::cerr << "failed to open animation archive: " << path << std::endl;
        return false;
    }

    ozz::io::IArchive archive(&file);

    if (archive.TestTag<ozz::animation::Animation>())
    {
        ozz::animation::Animation animation;
        archive >> animation;
        const char* name = animation.name();
        const std::string actual_name = name ? name : std::string();
        if (!motion_name.empty() && actual_name != motion_name)
        {
            std::cerr << "animation '" << motion_name << "' not found in single-animation archive" << std::endl;
            return false;
        }

        animation_out = std::move(animation);
        return true;
    }

    file.Seek(0, ozz::io::File::kSet);
    archive = ozz::io::IArchive(&file);

    uint32_t animation_count = 0;
    archive >> animation_count;

    bool found = false;
    for (uint32_t i = 0; i < animation_count; ++i)
    {
        ozz::animation::Animation animation;
        archive >> animation;

        SerializedMotionMetadata metadata;
        ReadSerializedMotionMetadata(archive, &metadata);
        const std::string& metadata_name = metadata.name;

        if (!found)
        {
            const char* animation_name_cstr = animation.name();
            const std::string animation_name = animation_name_cstr ? animation_name_cstr : std::string();
            if (animation_name == motion_name || metadata_name == motion_name)
            {
                animation_out = std::move(animation);
                found = true;
            }
        }
    }

    if (!found)
    {
        std::cerr << "animation '" << motion_name << "' not found in archive" << std::endl;
    }

    return found;
}

std::string QuoteForShell(const std::string& value)
{
#ifdef _WIN32
    std::string quoted = "\"";
    for (char ch : value)
    {
        if (ch == '\\' || ch == '"')
            quoted.push_back('\\');
        quoted.push_back(ch);
    }
    quoted.push_back('"');
    return quoted;
#else
    std::string quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back('\'');
    for (char ch : value)
    {
        if (ch == '\'')
            quoted.append("'\\''");
        else
            quoted.push_back(ch);
    }
    quoted.push_back('\'');
    return quoted;
#endif
}

fs::path ResolveBinary(const std::string& executable_name)
{
    const fs::path root = ProjectRoot();
    const auto make_candidate = [&](const fs::path& dir) -> fs::path
    {
        return dir / executable_name;
    };

    const std::array<fs::path, 8> search_roots = {
        root / "ozz_utils" / "bin" / "Mixed",
        root / "ozz_utils" / "bin" / "Debug",
        root / "ozz_utils" / "bin" / "Release",
        root / "ozz_utils" / "bin",
        root / "bin" / "x86_64" / "Mixed",
        root / "bin" / "Mixed",
        root / "build" / "bin" / "Mixed",
        root / "build" / "bin" / "ReleaseMasterGold",
    };

    std::vector<fs::path> candidates;
    candidates.reserve(search_roots.size());
    for (const auto& directory : search_roots)
        candidates.emplace_back(make_candidate(directory));

    for (const auto& path : candidates)
    {
        if (fs::exists(path))
            return path;
    }

    std::ostringstream oss;
    oss << "Unable to locate binary '" << executable_name << "'. Checked:";
    for (const auto& candidate : candidates)
        oss << '\n' << "  " << candidate.string();
    throw std::runtime_error(oss.str());
}

fs::path ResolveConverterBinary()
{
#ifdef _WIN32
    return ResolveBinary("xray_to_ozz_converter.exe");
#else
    return ResolveBinary("xray_to_ozz_converter");
#endif
}

std::string BuildCommand(const fs::path& binary, const std::vector<std::string>& args)
{
    const fs::path binary_dir = binary.parent_path();

#ifdef _WIN32
    std::string command = QuoteForShell(binary.string());
    for (const std::string& arg : args)
    {
        command.push_back(' ');
        command.append(QuoteForShell(arg));
    }
    return command;
#else
    std::string command = "LD_LIBRARY_PATH=";
    command.append(QuoteForShell(binary_dir.string()));
    command.push_back(' ');
    command.append(QuoteForShell(binary.string()));
    for (const std::string& arg : args)
    {
        command.push_back(' ');
        command.append(QuoteForShell(arg));
    }
    return command;
#endif
}

int ExecuteCommand(const fs::path& binary, const std::vector<std::string>& args)
{
    const std::string command = BuildCommand(binary, args);
    const int result = std::system(command.c_str());
    if (result == -1)
        return -1;

#ifdef _WIN32
    return result;
#else
    if (WIFEXITED(result))
        return WEXITSTATUS(result);
    if (WIFSIGNALED(result))
        return 128 + WTERMSIG(result);
    return result;
#endif
}

int ExecuteConverterCommand(const std::vector<std::string>& args)
{
    return ExecuteCommand(ResolveConverterBinary(), args);
}

bool ConvertSkeleton(bool force)
{
    const fs::path output_dir = TestArtifactsDir();
    const fs::path output_file = SkeletonOutputPath();

    std::error_code ec;
    fs::create_directories(output_dir, ec);

    if (fs::exists(output_file))
        fs::remove(output_file);

    std::vector<std::string> args = { "skeleton", SkeletonInputPath().string(), output_file.string(), "--dump-bind", SkeletonCsvPath().string() };

    const int exit_code = ExecuteConverterCommand(args);
    if (exit_code != 0)
    {
        std::cerr << "xray_to_ozz_converter returned exit code " << exit_code << std::endl;
        return false;
    }

    return fs::exists(output_file);
}

bool ConvertMesh(bool force)
{
    const fs::path output_dir = TestArtifactsDir();
    const fs::path output_file = MeshOutputPath();

    std::error_code ec;
    fs::create_directories(output_dir, ec);

    if (fs::exists(output_file))
        fs::remove(output_file);

    std::vector<std::string> args = {
        "mesh",
        SkeletonInputPath().string(),
        output_file.string(),
    };

    const int exit_code = ExecuteConverterCommand(args);
    if (exit_code != 0)
    {
        std::cerr << "xray_to_ozz_converter returned exit code " << exit_code << std::endl;
        return false;
    }

    return fs::exists(output_file);
}

bool ConvertBundle(bool force)
{
    const fs::path output_file = BundleOutputPath();

    std::error_code ec;
    fs::create_directories(TestArtifactsDir(), ec);

    if (fs::exists(output_file))
        fs::remove(output_file);

    std::vector<std::string> args = { "bundle", SkeletonInputPath().string(), output_file.string() };

    const int exit_code = ExecuteConverterCommand(args);
    if (exit_code != 0)
    {
        std::cerr << "bundle conversion failed with exit code " << exit_code << std::endl;
        return false;
    }

    return fs::exists(output_file);
}

bool ConvertAnimation(bool force)
{
    if (!ConvertSkeleton(true))
        return false;

    const fs::path output_file = AnimationOutputPath();
    std::error_code ec;
    fs::create_directories(TestArtifactsDir(), ec);

    if (fs::exists(output_file))
        fs::remove(output_file);

    std::vector<std::string> args = { "animation", AnimationInputPath().string(), output_file.string(), SkeletonInputPath().string(), "--metadata",
        AnimationMetadataPath().string() };

    const int exit_code = ExecuteConverterCommand(args);
    if (exit_code != 0)
    {
        std::cerr << "animation conversion failed with exit code " << exit_code << std::endl;
        return false;
    }

    return fs::exists(output_file);
}

bool ConvertSpecificMotionInternal(const std::string& motion_name, const fs::path& output_file, bool optimize)
{
    if (!ConvertSkeleton(true))
        return false;

    std::error_code ec;
    fs::create_directories(TestArtifactsDir(), ec);

    if (fs::exists(output_file))
        fs::remove(output_file);

    std::vector<std::string> args = {
        "animation",
        AnimationInputPath().string(),
        output_file.string(),
        SkeletonInputPath().string(),
        "--motion",
        motion_name,
    };
    if (optimize)
        args.emplace_back("--optimize");

    const int exit_code = ExecuteConverterCommand(args);
    if (exit_code != 0)
    {
        std::cerr << "animation conversion for motion '" << motion_name << "' failed with exit code " << exit_code << std::endl;
        return false;
    }

    return fs::exists(output_file);
}

bool ConvertSpecificMotion(const std::string& motion_name, bool force)
{
    return ConvertSpecificMotionInternal(motion_name, SingleAnimationOutputPath(), false);
}

bool ConvertSpecificMotionOptimized(const std::string& motion_name, bool force)
{
    return ConvertSpecificMotionInternal(motion_name, SingleAnimationOptimizedOutputPath(), true);
}

template <typename T>
bool LoadOzz(const fs::path& path, T& object)
{
    ozz::io::File file(path.string().c_str(), "rb");
    if (!file.opened())
    {
        std::cerr << "failed to open ozz archive: " << path << std::endl;
        return false;
    }

    ozz::io::IArchive archive(&file);

    if constexpr (std::is_same_v<T, ozz::animation::Animation>)
    {
        if (archive.TestTag<ozz::animation::Animation>())
        {
            archive >> object;
            return true;
        }

        file.Seek(0, ozz::io::File::kSet);
        archive = ozz::io::IArchive(&file);

        uint32_t animation_count = 0;
        archive >> animation_count;
        if (animation_count == 0)
        {
            std::cerr << "animation archive contains no animations" << std::endl;
            return false;
        }

        archive >> object;
        ReadSerializedMotionMetadata(archive, nullptr);
        return true;
    }
    else
    {
        archive >> object;
        return true;
    }
}

int FindJoint(const ozz::animation::Skeleton& skeleton, const std::string& name)
{
    const int index = ozz::animation::FindJoint(skeleton, name.c_str());
    if (index < 0)
        std::cerr << "joint not found: " << name << std::endl;
    return index;
}

bool SampleLocals(const ozz::animation::Animation& animation, const ozz::animation::Skeleton& skeleton, float time, std::vector<ozz::math::Transform>& locals)
{
    if (animation.duration() <= 0.f)
        return false;

    const float clamped = std::clamp(time, 0.f, animation.duration());

    ozz::animation::SamplingJob::Context context(animation.num_tracks());
    std::vector<ozz::math::SoaTransform> soa_transforms(skeleton.num_soa_joints());

    ozz::animation::SamplingJob job;
    job.animation = &animation;
    job.context = &context;
    job.ratio = clamped / animation.duration();
    job.output = ozz::make_span(soa_transforms);
    if (!job.Run())
        return false;

    locals.resize(skeleton.num_joints());
    for (int soa_index = 0; soa_index < skeleton.num_soa_joints(); ++soa_index)
    {
        const auto& soa = soa_transforms[soa_index];

        ozz::math::SimdFloat4 translations[4];
        ozz::math::Transpose3x4(&soa.translation.x, translations);

        ozz::math::SimdFloat4 rotations[4];
        ozz::math::Transpose4x4(&soa.rotation.x, rotations);

        ozz::math::SimdFloat4 scales[4];
        ozz::math::Transpose3x4(&soa.scale.x, scales);

        for (int lane = 0; lane < 4; ++lane)
        {
            const int joint = soa_index * 4 + lane;
            if (joint >= skeleton.num_joints())
                break;

            ozz::math::Transform transform;
            ozz::math::Store3PtrU(translations[lane], &transform.translation.x);
            ozz::math::StorePtrU(rotations[lane], &transform.rotation.x);
            ozz::math::Store3PtrU(scales[lane], &transform.scale.x);
            locals[joint] = transform;
        }
    }

    return true;
}

struct ExpectedBindPose
{
    const char* joint;
    float tx;
    float ty;
    float tz;
};

const std::array<ExpectedBindPose, 5> kExpectedBindPose = {
    {
     { "root_stalker", 0.0f, 0.0f, 0.0f },
     { "bip01", 6.96513e-06f, 0.987438f, 4.50560e-06f },
     { "bip01_pelvis", 0.0f, 0.0f, 0.0f },
     { "bip01_spine", 0.102435f, 1.76455e-07f, 0.0213843f },
     { "bip01_head", 0.0559939f, 2.85225e-09f, 1.90456e-08f },
     }
};

constexpr float kTranslationTolerance = 1e-4f;
constexpr float kRotationTolerance = 1e-4f;

bool TestGenerateSkeleton()
{
    std::cout << "Generating skeleton via converter..." << std::endl;
    return ConvertSkeleton(true);
}

bool TestSkeletonWrittenToTestdataDirectory()
{
    if (!ConvertSkeleton(true))
    {
        ADD_FAILURE() << "Skeleton conversion failed";
        return false;
    }

    const auto expected_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero_1.ozz");
    const auto actual_path = SkeletonOutputPath();

    bool ok = true;

    EXPECT_EQ(PathToString(actual_path), PathToString(expected_path));
    if (PathToString(actual_path) != PathToString(expected_path))
        ok = false;

    EXPECT_TRUE(fs::exists(expected_path)) << "expected converted skeleton missing at " << PathToString(expected_path);
    if (!fs::exists(expected_path))
        ok = false;

    return ok;
}

bool TestGenerateMesh()
{
    std::cout << "Generating mesh via converter..." << std::endl;
    return ConvertMesh(true);
}

bool CompareMeshTrianglesWithSource(size_t surface_index, const OgfSurfaceGeometry& source, const ozz::sample::Mesh& mesh)
{
    if (source.indices.size() % 3 != 0)
    {
        std::cerr << "surface " << surface_index << " source index buffer not divisible by 3" << std::endl;
        return false;
    }

    if (mesh.triangle_indices.size() % 3 != 0)
    {
        std::cerr << "surface " << surface_index << " exported index buffer not divisible by 3" << std::endl;
        return false;
    }

    std::vector<std::array<uint32_t, 3>> source_triangles;
    source_triangles.reserve(source.indices.size() / 3);
    for (size_t tri = 0; tri < source.indices.size(); tri += 3)
    {
        std::array<uint32_t, 3> indices = {
            source.indices[tri + 0],
            source.indices[tri + 1],
            source.indices[tri + 2],
        };

        if (source.vertex_count > 0)
        {
            if (indices[0] >= source.vertex_count || indices[1] >= source.vertex_count || indices[2] >= source.vertex_count)
            {
                std::cerr << "surface " << surface_index << " source triangle references vertex outside range" << std::endl;
                return false;
            }
        }

        source_triangles.push_back(indices);
    }

    const size_t exported_vertex_count = static_cast<size_t>(mesh.vertex_count());
    const auto& remap = mesh.xray_metadata.remapped_to_original;
    const bool has_remap = !remap.empty();

    if (has_remap && remap.size() != exported_vertex_count)
    {
        std::cerr << "surface " << surface_index << " remap size " << remap.size() << " does not match exported vertex count "
                  << exported_vertex_count << std::endl;
        return false;
    }

    std::vector<std::array<uint32_t, 3>> mesh_triangles;
    mesh_triangles.reserve(mesh.triangle_indices.size() / 3);

    for (size_t tri = 0; tri < mesh.triangle_indices.size(); tri += 3)
    {
        const uint16_t idx0 = mesh.triangle_indices[tri + 0];
        const uint16_t idx1 = mesh.triangle_indices[tri + 1];
        const uint16_t idx2 = mesh.triangle_indices[tri + 2];

        if (idx0 >= exported_vertex_count || idx1 >= exported_vertex_count || idx2 >= exported_vertex_count)
        {
            std::cerr << "surface " << surface_index << " exported triangle references vertex outside range" << std::endl;
            return false;
        }

        const uint32_t original0 = has_remap ? remap[idx0] : static_cast<uint32_t>(idx0);
        const uint32_t original1 = has_remap ? remap[idx1] : static_cast<uint32_t>(idx1);
        const uint32_t original2 = has_remap ? remap[idx2] : static_cast<uint32_t>(idx2);

        if (source.vertex_count > 0)
        {
            if (original0 >= source.vertex_count || original1 >= source.vertex_count || original2 >= source.vertex_count)
            {
                std::cerr << "surface " << surface_index << " exported triangle remaps outside original vertex range" << std::endl;
                return false;
            }
        }

        mesh_triangles.push_back({ original0, original1, original2 });
    }

    if (mesh_triangles.size() != source_triangles.size())
    {
        std::cerr << "surface " << surface_index << " triangle count mismatch (" << mesh_triangles.size() << " vs " << source_triangles.size() << ")"
                  << std::endl;
        return false;
    }

    auto make_canonical = [](std::vector<std::array<uint32_t, 3>> triangles)
    {
        for (auto& tri : triangles)
            std::sort(tri.begin(), tri.end());
        std::sort(triangles.begin(), triangles.end());
        return triangles;
    };

    const auto canonical_source = make_canonical(source_triangles);
    const auto canonical_exported = make_canonical(mesh_triangles);

    if (canonical_source != canonical_exported)
    {
        std::vector<std::array<uint32_t, 3>> missing;
        std::vector<std::array<uint32_t, 3>> extra;

        std::set_difference(canonical_source.begin(), canonical_source.end(), canonical_exported.begin(), canonical_exported.end(),
            std::back_inserter(missing));
        std::set_difference(canonical_exported.begin(), canonical_exported.end(), canonical_source.begin(), canonical_source.end(),
            std::back_inserter(extra));

        auto triangle_to_string = [](const std::array<uint32_t, 3>& tri)
        {
            std::ostringstream oss;
            oss << '(' << tri[0] << ',' << tri[1] << ',' << tri[2] << ')';
            return oss.str();
        };

        if (!missing.empty())
        {
            std::cerr << "surface " << surface_index << " missing triangle example " << triangle_to_string(missing.front()) << std::endl;
        }

        if (!extra.empty())
        {
            std::cerr << "surface " << surface_index << " extra triangle example " << triangle_to_string(extra.front()) << std::endl;
        }

        return false;
    }

    return true;
}

bool TestMeshTrianglesMatchSource()
{
    if (!ConvertMesh(false))
        return false;

    std::vector<OgfSurfaceGeometry> ogf_geometry;
    if (!LoadOgfSurfaceGeometry(SkeletonInputPath(), ogf_geometry))
        return false;

    std::vector<ozz::sample::Mesh> meshes;
    if (!LoadMeshArchive(MeshOutputPath(), meshes))
        return false;

    if (meshes.size() != ogf_geometry.size())
    {
        std::cerr << "surface count mismatch before triangle comparison" << std::endl;
        return false;
    }

    bool ok = true;
    for (size_t i = 0; i < meshes.size(); ++i)
    {
        if (!CompareMeshTrianglesWithSource(i, ogf_geometry[i], meshes[i]))
            ok = false;
    }

    return ok;
}

bool TestMeshVertexCountsMatchSource()
{
    if (!ConvertMesh(false))
        return false;

    std::vector<OgfSurfaceStats> ogf_surfaces;
    if (!LoadOgfSurfaceStats(SkeletonInputPath(), ogf_surfaces))
        return false;

    std::vector<ozz::sample::Mesh> meshes;
    if (!LoadMeshArchive(MeshOutputPath(), meshes))
        return false;

    if (meshes.size() != ogf_surfaces.size())
    {
        std::cerr << "surface count mismatch before vertex comparison" << std::endl;
        return false;
    }

    uint64_t ogf_vertex_total = 0;
    uint64_t mesh_vertex_total = 0;
    uint64_t ogf_face_total = 0;
    uint64_t mesh_face_total = 0;

    bool ok = true;

    for (size_t i = 0; i < meshes.size(); ++i)
    {
        const ozz::sample::Mesh& mesh = meshes[i];
        const OgfSurfaceStats& source = ogf_surfaces[i];
        const uint32_t mesh_face_count = static_cast<uint32_t>(mesh.triangle_index_count() / 3);

        if (mesh.vertex_count() != static_cast<int>(source.vertex_count))
        {
            std::cerr << "surface " << i << " exported vertex count " << mesh.vertex_count() << " differs from OGF vertex count " << source.vertex_count
                      << std::endl;
            ok = false;
        }

        if (mesh_face_count != source.face_count)
        {
            std::cerr << "surface " << i << " exported face count " << mesh_face_count << " differs from OGF face count " << source.face_count << std::endl;
            ok = false;
        }

        mesh_vertex_total += static_cast<uint32_t>(mesh.vertex_count());
        mesh_face_total += mesh_face_count;
        ogf_vertex_total += source.vertex_count;
        ogf_face_total += source.face_count;
    }

    if (mesh_vertex_total != ogf_vertex_total)
    {
        std::cerr << "total exported vertices " << mesh_vertex_total << " differ from OGF vertices " << ogf_vertex_total << std::endl;
        ok = false;
    }

    if (mesh_face_total != ogf_face_total)
    {
        std::cerr << "total exported faces " << mesh_face_total << " differ from OGF faces " << ogf_face_total << std::endl;
        ok = false;
    }

    return ok;
}

bool TestMeshSurfaceCountMatchesSource()
{
    if (!ConvertMesh(false))
        return false;

    std::vector<OgfSurfaceStats> ogf_surfaces;
    if (!LoadOgfSurfaceStats(SkeletonInputPath(), ogf_surfaces))
        return false;

    std::vector<ozz::sample::Mesh> meshes;
    if (!LoadMeshArchive(MeshOutputPath(), meshes))
        return false;

    if (meshes.size() != ogf_surfaces.size())
    {
        std::cerr << "exported mesh surface count " << meshes.size() << " does not match OGF surface count " << ogf_surfaces.size() << std::endl;
        return false;
    }

    return true;
}

bool TestMeshSurfaceStatsMatchSource()
{
    if (!ConvertMesh(false))
        return false;

    std::vector<OgfSurfaceStats> ogf_surfaces;
    if (!LoadOgfSurfaceStats(SkeletonInputPath(), ogf_surfaces))
        return false;

    std::vector<ozz::sample::Mesh> meshes;
    if (!LoadMeshArchive(MeshOutputPath(), meshes))
        return false;

    if (meshes.size() != ogf_surfaces.size())
    {
        std::cerr << "surface count mismatch before stats comparison" << std::endl;
        return false;
    }

    bool ok = true;

    for (size_t i = 0; i < meshes.size(); ++i)
    {
        const ozz::sample::Mesh& mesh = meshes[i];
        const OgfSurfaceStats& source = ogf_surfaces[i];
        const ozz::sample::XRayMeshMetadata& metadata = mesh.xray_metadata;

        if (metadata.original_vertex_count != source.vertex_count)
        {
            std::cerr << "surface " << i << " original vertex count " << metadata.original_vertex_count << " (metadata) does not match OGF "
                      << source.vertex_count << std::endl;
            ok = false;
        }

        if (metadata.original_face_count != source.face_count)
        {
            std::cerr << "surface " << i << " original face count " << metadata.original_face_count << " (metadata) does not match OGF " << source.face_count
                      << std::endl;
            ok = false;
        }

        const uint32_t mesh_face_count = static_cast<uint32_t>(mesh.triangle_index_count() / 3);
        if (mesh_face_count != source.face_count)
        {
            std::cerr << "surface " << i << " exported triangle count " << mesh_face_count << " differs from OGF face count " << source.face_count << std::endl;
            ok = false;
        }

        const uint32_t kInvalidRemap = std::numeric_limits<uint32_t>::max();
        const size_t exported_vertex_count = static_cast<size_t>(mesh.vertex_count());

        if (!metadata.original_to_remapped.empty() && metadata.original_to_remapped.size() != static_cast<size_t>(source.vertex_count))
        {
            std::cerr << "surface " << i << " original_to_remapped size " << metadata.original_to_remapped.size()
                      << " does not match original vertex count " << source.vertex_count << std::endl;
            ok = false;
        }

        if (!metadata.remapped_to_original.empty() && metadata.remapped_to_original.size() != exported_vertex_count)
        {
            std::cerr << "surface " << i << " remapped_to_original size " << metadata.remapped_to_original.size()
                      << " does not match exported vertex count " << exported_vertex_count << std::endl;
            ok = false;
        }

        bool has_valid_mapping = false;
        const size_t remapped_span = metadata.remapped_to_original.size();
        for (size_t original = 0; original < metadata.original_to_remapped.size(); ++original)
        {
            const uint32_t remapped = metadata.original_to_remapped[original];
            if (remapped == kInvalidRemap)
            {
                continue;
            }
            if (remapped >= remapped_span)
            {
                std::cerr << "surface " << i << " original vertex " << original << " remaps to out-of-range index " << remapped << std::endl;
                ok = false;
                continue;
            }
            if (!metadata.remapped_to_original.empty() && metadata.remapped_to_original[remapped] != original)
            {
                std::cerr << "surface " << i << " remap inconsistency: remapped index " << remapped << " resolves to original "
                          << metadata.remapped_to_original[remapped] << " instead of " << original << std::endl;
                ok = false;
            }
            else
            {
                has_valid_mapping = true;
            }
        }

        if (!metadata.original_to_remapped.empty() && !has_valid_mapping)
        {
            std::cerr << "surface " << i << " has no valid vertex remap entries" << std::endl;
            ok = false;
        }

        if (mesh.vertex_count() != static_cast<int>(source.vertex_count))
        {
            std::cerr << "surface " << i << " exported vertex count " << mesh.vertex_count() << " differs from OGF vertex count " << source.vertex_count
                      << std::endl;
            ok = false;
        }
    }

    return ok;
}

bool TestConvertAnimationProducesFile()
{
    const bool status = ConvertAnimation(true);
    if (!status)
        return false;
    return fs::exists(AnimationOutputPath()) && fs::exists(AnimationMetadataPath());
}

bool TestAnimationCompatibleWithSkeleton()
{
    if (!ConvertAnimation(false))
        return false;

    ozz::animation::Skeleton skeleton;
    if (!LoadOzz(SkeletonOutputPath(), skeleton))
        return false;

    ozz::animation::Animation animation;
    if (!LoadAnimationByName(AnimationOutputPath(), kExpectedMultiMotionNames[0], animation))
        return false;

    if (animation.num_tracks() != skeleton.num_joints())
    {
        std::cerr << "animation track count " << animation.num_tracks() << " does not match skeleton joints " << skeleton.num_joints() << std::endl;
        return false;
    }

    return animation.duration() > 0.f;
}

bool CompareQuaternion(const ozz::math::Quaternion& actual, float qx, float qy, float qz, float qw)
{
    const float dx = std::fabs(actual.x - qx);
    const float dy = std::fabs(actual.y - qy);
    const float dz = std::fabs(actual.z - qz);
    const float dw = std::fabs(actual.w - qw);
    return dx <= kRotationTolerance && dy <= kRotationTolerance && dz <= kRotationTolerance && dw <= kRotationTolerance;
}

bool TestAnimationMatchesReference()
{
    if (!ConvertAnimation(false))
        return false;

    if (!ConvertSpecificMotion(kExpectedMultiMotionNames[0], true))
        return false;

    ozz::animation::Skeleton skeleton;
    if (!LoadOzz(SkeletonOutputPath(), skeleton))
        return false;

    ozz::animation::Animation animation;
    if (!LoadAnimationByName(AnimationOutputPath(), kExpectedMultiMotionNames[0], animation))
        return false;

    ozz::animation::Animation reference_animation;
    if (!LoadOzz(SingleAnimationOutputPath(), reference_animation))
        return false;

    const float frame_duration = 1.0f / 30.0f;
    const int total_frames = static_cast<int>(std::round(reference_animation.duration() / frame_duration)) + 1;
    if (total_frames < 2)
    {
        std::cerr << "animation frame count too small" << std::endl;
        return false;
    }

    bool ok = true;

    const std::array<int, 3> frames = { 0, total_frames / 2, total_frames - 1 };
    const std::array<const char*, 3> tracked_joints = { "bip01_pelvis", "bip01_spine", "bip01_head" };

    for (const int frame : frames)
    {
        const float time = static_cast<float>(frame) * frame_duration;

        std::vector<ozz::math::Transform> locals_multi;
        std::vector<ozz::math::Transform> locals_reference;
        if (!SampleLocals(animation, skeleton, time, locals_multi))
            return false;
        if (!SampleLocals(reference_animation, skeleton, time, locals_reference))
            return false;

        for (const char* joint_name : tracked_joints)
        {
            const int joint_index = FindJoint(skeleton, joint_name);
            if (joint_index < 0)
            {
                ok = false;
                continue;
            }

            const auto& actual = locals_multi[joint_index];
            const auto& expected = locals_reference[joint_index];
            const float dx = std::fabs(actual.translation.x - expected.translation.x);
            const float dy = std::fabs(actual.translation.y - expected.translation.y);
            const float dz = std::fabs(actual.translation.z - expected.translation.z);

            if (dx > kTranslationTolerance || dy > kTranslationTolerance || dz > kTranslationTolerance)
            {
                std::cerr << "frame " << frame << " joint '" << joint_name << "' translation mismatch\n"
                          << "  expected: [" << expected.translation.x << ", " << expected.translation.y << ", " << expected.translation.z << "]\n"
                          << "  actual:   [" << actual.translation.x << ", " << actual.translation.y << ", " << actual.translation.z << "]\n";
                ok = false;
            }

            if (!CompareQuaternion(actual.rotation, expected.rotation.x, expected.rotation.y, expected.rotation.z, expected.rotation.w))
            {
                std::cerr << "frame " << frame << " joint '" << joint_name << "' rotation mismatch\n";
                ok = false;
            }
        }
    }

    return ok;
}

bool TestOptimizedAnimationMatchesSource()
{
    const char* motion_name = kExpectedMultiMotionNames[0];

    if (!ConvertSpecificMotion(motion_name, true))
        return false;

    if (!ConvertSpecificMotionOptimized(motion_name, true))
        return false;

    const fs::path original_path = SingleAnimationOutputPath();
    const fs::path optimized_path = SingleAnimationOptimizedOutputPath();
    if (!fs::exists(original_path) || !fs::exists(optimized_path))
        return false;

    const std::uintmax_t original_size = fs::file_size(original_path);
    const std::uintmax_t optimized_size = fs::file_size(optimized_path);
    if (optimized_size >= original_size)
    {
        std::cerr << "optimized animation size " << optimized_size << " not smaller than original " << original_size << std::endl;
        return false;
    }

    if (!ConvertSkeleton(true))
        return false;

    ozz::animation::Skeleton skeleton;
    if (!LoadOzz(SkeletonOutputPath(), skeleton))
        return false;

    ozz::animation::Animation original_animation;
    if (!LoadOzz(original_path, original_animation))
        return false;

    ozz::animation::Animation optimized_animation;
    if (!LoadOzz(optimized_path, optimized_animation))
        return false;

    if (original_animation.num_tracks() != optimized_animation.num_tracks())
    {
        std::cerr << "optimized animation track count mismatch" << std::endl;
        return false;
    }

    const float duration_delta = std::fabs(original_animation.duration() - optimized_animation.duration());
    if (duration_delta > 1e-5f)
    {
        std::cerr << "optimized animation duration differs by " << duration_delta << std::endl;
        return false;
    }

    const int sample_steps = 25;
    const float translation_epsilon = 2e-3f;
    const float rotation_epsilon = 5e-4f;
    const float scale_epsilon = 1e-4f;

    std::vector<ozz::math::Transform> original_locals;
    std::vector<ozz::math::Transform> optimized_locals;

    for (int step = 0; step < sample_steps; ++step)
    {
        const float ratio = sample_steps == 1 ? 0.f : static_cast<float>(step) / static_cast<float>(sample_steps - 1);
        const float time = original_animation.duration() * ratio;

        if (!SampleLocals(original_animation, skeleton, time, original_locals))
            return false;
        if (!SampleLocals(optimized_animation, skeleton, time, optimized_locals))
            return false;

        if (original_locals.size() != optimized_locals.size())
            return false;

        for (size_t joint = 0; joint < original_locals.size(); ++joint)
        {
            const auto& lhs = original_locals[joint];
            const auto& rhs = optimized_locals[joint];

            const float dx = lhs.translation.x - rhs.translation.x;
            const float dy = lhs.translation.y - rhs.translation.y;
            const float dz = lhs.translation.z - rhs.translation.z;
            const float translation_error = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (translation_error > translation_epsilon)
            {
                std::cerr << "translation error " << translation_error << " for joint " << joint << " at sample " << step << std::endl;
                return false;
            }

            float dot = lhs.rotation.x * rhs.rotation.x + lhs.rotation.y * rhs.rotation.y + lhs.rotation.z * rhs.rotation.z + lhs.rotation.w * rhs.rotation.w;
            dot = std::fabs(dot);
            if (dot < 1.f - rotation_epsilon)
            {
                std::cerr << "rotation dot product " << dot << " below threshold for joint " << joint << " at sample " << step << std::endl;
                return false;
            }

            const float sx = std::fabs(lhs.scale.x - rhs.scale.x);
            const float sy = std::fabs(lhs.scale.y - rhs.scale.y);
            const float sz = std::fabs(lhs.scale.z - rhs.scale.z);
            if (sx > scale_epsilon || sy > scale_epsilon || sz > scale_epsilon)
            {
                std::cerr << "scale mismatch for joint " << joint << " at sample " << step << std::endl;
                return false;
            }
        }
    }

    return true;
}

bool TestMultipleAnimationConversion()
{
    if (!ConvertAnimation(true))
        return false;

    ozz::io::File file(AnimationOutputPath().string().c_str(), "rb");
    if (!file.opened())
    {
        std::cerr << "failed to open animation archive: " << AnimationOutputPath() << std::endl;
        return false;
    }

    ozz::io::IArchive archive(&file);
    if (archive.TestTag<ozz::animation::Animation>())
    {
        std::cerr << "expected multi-animation archive but found single animation" << std::endl;
        return false;
    }

    file.Seek(0, ozz::io::File::kSet);
    archive = ozz::io::IArchive(&file);

    uint32_t animation_count = 0;
    archive >> animation_count;
    if (animation_count != kExpectedMultiMotionNames.size())
    {
        std::cerr << "expected " << kExpectedMultiMotionNames.size() << " animations, found " << animation_count << std::endl;
        return false;
    }

    std::set<std::string> names;
    for (uint32_t i = 0; i < animation_count; ++i)
    {
        ozz::animation::Animation animation;
        archive >> animation;
        if (animation.num_tracks() == 0)
        {
            std::cerr << "animation " << i << " has zero tracks" << std::endl;
            return false;
        }

        const char* name = animation.name();
        if (!name || *name == '\0')
        {
            std::cerr << "animation " << i << " missing name" << std::endl;
            return false;
        }

        SerializedMotionMetadata metadata;
        ReadSerializedMotionMetadata(archive, &metadata);
        const std::string& metadata_name = metadata.name;
        if (metadata_name != name)
        {
            std::cerr << "metadata name mismatch for animation '" << name << "', metadata reports '" << metadata_name << "'" << std::endl;
            return false;
        }

        names.insert(std::string(name));
    }

    for (const char* expected : kExpectedMultiMotionNames)
    {
        if (names.count(expected) == 0)
        {
            std::cerr << "missing animation named '" << expected << "'" << std::endl;
            return false;
        }
    }

    return true;
}

bool TestAnimationMetadataIncludesBoneMotions()
{
    if (!ConvertAnimation(true))
        return false;

    ozz::animation::Skeleton skeleton;
    if (!LoadOzz(SkeletonOutputPath(), skeleton))
        return false;

    xr_vector<xr_string> bone_names;
    const auto joint_names = skeleton.joint_names();
    bone_names.reserve(joint_names.size());
    for (const char* name : joint_names)
        bone_names.emplace_back(name ? name : "");

    xr_vector<XRay::Animation::ConvertedOmfAnimation> converted;
    if (!XRay::Animation::ConvertLegacyOmf(AnimationInputPath(), bone_names, skeleton, converted))
    {
        std::cerr << "ConvertLegacyOmf failed" << std::endl;
        return false;
    }

    std::unordered_map<std::string, const XRay::Animation::ConvertedOmfAnimation*> expected_by_name;
    expected_by_name.reserve(converted.size());
    for (const auto& entry : converted)
        expected_by_name.emplace(std::string(entry.name.c_str()), &entry);

    ozz::io::File file(AnimationOutputPath().string().c_str(), "rb");
    if (!file.opened())
    {
        std::cerr << "failed to open animation archive: " << AnimationOutputPath() << std::endl;
        return false;
    }

    ozz::io::IArchive archive(&file);
    if (archive.TestTag<ozz::animation::Animation>())
    {
        std::cerr << "expected multi-animation archive but found single animation" << std::endl;
        return false;
    }

    file.Seek(0, ozz::io::File::kSet);
    archive = ozz::io::IArchive(&file);

    uint32_t animation_count = 0;
    archive >> animation_count;

    bool ok = true;

    for (uint32_t index = 0; index < animation_count; ++index)
    {
        ozz::animation::Animation animation;
        archive >> animation;

        SerializedMotionMetadata metadata;
        ReadSerializedMotionMetadata(archive, &metadata);

        auto expected_it = expected_by_name.find(metadata.name);
        if (expected_it == expected_by_name.end())
        {
            std::cerr << "unexpected metadata entry '" << metadata.name << "'" << std::endl;
            ok = false;
            continue;
        }

        const auto& expected = *expected_it->second;

        if (metadata.frame_count != expected.frame_count)
        {
            std::cerr << "motion '" << metadata.name << "' frame count mismatch: metadata=" << metadata.frame_count
                      << " expected=" << expected.frame_count << std::endl;
            ok = false;
        }

        std::unordered_map<uint16_t, const XRay::Animation::ConvertedBoneMotion*> expected_bones;
        expected_bones.reserve(expected.bone_motions.size());
        for (const auto& bone : expected.bone_motions)
            expected_bones.emplace(bone.bone_id, &bone);

        for (const auto& bone : metadata.bone_motions)
        {
            auto expected_bone_it = expected_bones.find(bone.bone_id);
            if (expected_bone_it == expected_bones.end())
            {
                std::cerr << "motion '" << metadata.name << "' unexpected bone id " << bone.bone_id << std::endl;
                ok = false;
                continue;
            }

            if (!CompareBoneMotion(bone, *expected_bone_it->second, metadata.name))
                ok = false;

            expected_bones.erase(expected_bone_it);
        }

        if (!expected_bones.empty())
        {
            for (const auto& missing : expected_bones)
                std::cerr << "motion '" << metadata.name << "' missing bone id " << missing.first << std::endl;
            ok = false;
        }
    }

    return ok;
}

bool TestAnimationNamesPreserved()
{
    if (!ConvertAnimation(true))
        return false;

    ozz::io::File file(AnimationOutputPath().string().c_str(), "rb");
    if (!file.opened())
    {
        std::cerr << "failed to open animation archive: " << AnimationOutputPath() << std::endl;
        return false;
    }

    ozz::io::IArchive archive(&file);
    if (archive.TestTag<ozz::animation::Animation>())
    {
        ozz::animation::Animation animation;
        archive >> animation;
        const char* name = animation.name();
        if (!name || std::string(name) != kExpectedMultiMotionNames[0])
        {
            std::cerr << "single animation archive missing expected name" << std::endl;
            return false;
        }
        return true;
    }

    file.Seek(0, ozz::io::File::kSet);
    archive = ozz::io::IArchive(&file);

    uint32_t animation_count = 0;
    archive >> animation_count;

    if (animation_count != kExpectedMultiMotionNames.size())
    {
        std::cerr << "expected " << kExpectedMultiMotionNames.size() << " animations, found " << animation_count << std::endl;
        return false;
    }

    std::vector<std::string> names;
    names.reserve(animation_count);

    for (uint32_t i = 0; i < animation_count; ++i)
    {
        ozz::animation::Animation animation;
        archive >> animation;

        const char* name = animation.name();
        if (!name || *name == '\0')
        {
            std::cerr << "animation index " << i << " missing runtime name" << std::endl;
            return false;
        }

        SerializedMotionMetadata metadata;
        ReadSerializedMotionMetadata(archive, &metadata);

        const std::string& metadata_name = metadata.name;
        if (metadata_name.empty())
        {
            std::cerr << "metadata missing name for animation index " << i << std::endl;
            return false;
        }

        if (metadata_name != name)
        {
            std::cerr << "metadata name mismatch for animation index " << i << " ('" << name << "' vs '" << metadata_name << "')" << std::endl;
            return false;
        }

        names.emplace_back(name);
    }

    std::set<std::string> unique_names(names.begin(), names.end());
    if (unique_names.size() != names.size())
    {
        std::cerr << "duplicate animation names detected" << std::endl;
        return false;
    }

    for (const char* expected : kExpectedMultiMotionNames)
    {
        if (unique_names.count(expected) == 0)
        {
            std::cerr << "missing animation name '" << expected << "'" << std::endl;
            return false;
        }
    }

    return true;
}

bool TestAssetBundleContainsSkeletonAndMesh()
{
    if (!TestGenerateMesh())
        return false;

    if (!ConvertBundle(true))
        return false;

    XRay::Animation::OzzxBundle bundle;
    if (!XRay::Animation::ReadOzzxBundle(BundleOutputPath(), bundle))
        return false;

    if (bundle.version != 2u)
    {
        std::cerr << "unexpected bundle version " << bundle.version << std::endl;
        return false;
    }

    if (bundle.skeleton.empty())
    {
        std::cerr << "bundle missing skeleton payload" << std::endl;
        return false;
    }

    if (bundle.mesh.empty())
    {
        std::cerr << "bundle missing mesh payload" << std::endl;
        return false;
    }

    if (bundle.motion_refs.empty() && bundle.embedded_animation_data.empty())
    {
        std::cerr << "bundle missing motion references" << std::endl;
        return false;
    }

    if (bundle.bone_metadata.empty())
    {
        std::cerr << "bundle missing bone metadata" << std::endl;
        return false;
    }

    const std::array<const char*, 4> expected_refs = {
        "actors\\stalker_animation",
        "actors\\stalker_scenario_animation",
        "actors\\stalker_scripts_animation",
        "actors\\stalker_smart_cover_animation"
    };

    auto normalize = [](xr_string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char ch)
            {
                if (ch == '\\')
                    return '/';
                return static_cast<char>(std::tolower(ch));
            });
        return value;
    };

    xr_vector<xr_string> normalized_refs;
    normalized_refs.reserve(bundle.motion_refs.size());
    for (const auto& ref : bundle.motion_refs)
        normalized_refs.push_back(normalize(ref));

    bool all_expected_present = true;
    for (const char* expected : expected_refs)
    {
        const xr_string normalized_expected = normalize(xr_string(expected));
        const bool present = std::any_of(normalized_refs.begin(), normalized_refs.end(),
            [&](const xr_string& value)
            {
                return value == normalized_expected;
            });
        if (!present)
        {
            std::cerr << "bundle motion refs missing expected entry '" << expected << "'" << std::endl;
            all_expected_present = false;
        }
    }

    if (!all_expected_present)
    {
        std::cerr << "available motion refs (" << bundle.motion_refs.size() << "):\n";
        for (const auto& ref : bundle.motion_refs)
            std::cerr << "  " << ref.c_str() << '\n';
        return false;
    }

    ozz::animation::Skeleton skeleton;
    {
        ozz::io::MemoryStream skeleton_stream;
        if (!skeleton_stream.Write(bundle.skeleton.data(), bundle.skeleton.size()))
        {
            std::cerr << "failed to seed skeleton stream" << std::endl;
            return false;
        }
        skeleton_stream.Seek(0, ozz::io::Stream::kSet);
        ozz::io::IArchive archive(&skeleton_stream);
        if (!archive.TestTag<ozz::animation::Skeleton>())
        {
            std::cerr << "skeleton payload missing Ozz tag" << std::endl;
            return false;
        }
        archive >> skeleton;
        if (skeleton.num_joints() <= 0)
        {
            std::cerr << "skeleton payload contains no joints" << std::endl;
            return false;
        }
    }

    std::vector<std::uint8_t> reference_mesh;
    if (!ReadBinaryFile(MeshOutputPath(), reference_mesh))
        return false;

    if (reference_mesh.empty())
    {
        std::cerr << "reference mesh is empty" << std::endl;
        return false;
    }

    if (bundle.mesh.size() != reference_mesh.size())
    {
        std::cerr << "mesh payload size mismatch (" << bundle.mesh.size() << " vs " << reference_mesh.size() << ")" << std::endl;
        return false;
    }

    if (bundle.mesh != reference_mesh)
    {
        std::cerr << "mesh payload differs from reference asset" << std::endl;
        return false;
    }

    return true;
}















bool TestLegacyOmfConverterConvertsCriticalHitOmf()
{
    const auto bundle_path = ResolveProjectPath("src/xrAnimation/tests/testdata/stalker_hero.ozzx");
    XRay::Animation::OzzxBundle bundle;
    if (!XRay::Animation::ReadOzzxBundle(bundle_path, bundle))
    {
        ADD_FAILURE() << "Failed to read bundle: " << PathToString(bundle_path);
        return false;
    }

    ozz::animation::Skeleton skeleton;
    ozz::io::MemoryStream skeleton_stream;
    if (!skeleton_stream.Write(bundle.skeleton.data(), bundle.skeleton.size()))
    {
        ADD_FAILURE() << "Failed to stage skeleton payload";
        return false;
    }
    skeleton_stream.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive skeleton_archive(&skeleton_stream);
    skeleton_archive >> skeleton;

    xr_vector<xr_string> bone_names;
    const auto joint_names = skeleton.joint_names();
    bone_names.reserve(joint_names.size());
    for (const char* name : joint_names)
        bone_names.emplace_back(name ? name : "");

    const auto omf_path = ResolveProjectPath("res/testdata/npc/critical_hit_grup_1.omf");
    xr_vector<XRay::Animation::ConvertedOmfAnimation> converted;
    if (!XRay::Animation::ConvertLegacyOmf(omf_path, bone_names, skeleton, converted))
    {
        ADD_FAILURE() << "ConvertLegacyOmf failed";
        return false;
    }

    if (converted.empty())
    {
        ADD_FAILURE() << "No animations converted";
        return false;
    }

    bool ok = true;
    for (const auto& entry : converted)
    {
        const ozz::animation::Animation* animation_ptr = entry.animation.get();
        if (animation_ptr == nullptr)
        {
            ADD_FAILURE() << "Converted animation missing payload";
            ok = false;
            continue;
        }

        EXPECT_EQ(animation_ptr->num_tracks(), skeleton.num_joints());
        if (animation_ptr->num_tracks() != skeleton.num_joints())
            ok = false;
    }

    return ok;
}

} // namespace

TEST(ConverterIntegration, GenerateSkeleton)
{
    EXPECT_TRUE(TestGenerateSkeleton());
}

TEST(ConverterIntegration, SkeletonWrittenToTestdataDirectory)
{
    EXPECT_TRUE(TestSkeletonWrittenToTestdataDirectory());
}

TEST(ConverterIntegration, GenerateMesh)
{
    EXPECT_TRUE(TestGenerateMesh());
}

TEST(ConverterIntegration, MeshVertexCountsMatchSource)
{
    EXPECT_TRUE(TestMeshVertexCountsMatchSource());
}

TEST(ConverterIntegration, MeshSurfaceCountMatchesSource)
{
    EXPECT_TRUE(TestMeshSurfaceCountMatchesSource());
}

TEST(ConverterIntegration, MeshSurfaceStatsMatchSource)
{
    EXPECT_TRUE(TestMeshSurfaceStatsMatchSource());
}

TEST(ConverterIntegration, MeshTrianglesMatchSource)
{
    EXPECT_TRUE(TestMeshTrianglesMatchSource());
}

TEST(ConverterRuntime, ConvertLegacyVisualToOzzBundleProducesPayloads)
{
    const auto ogf_path = ResolveProjectPath("res/testdata/npc/stalker_hero_1.ogf");
    ASSERT_TRUE(fs::exists(ogf_path)) << "Missing ogf fixture: " << ogf_path;

    XRay::Animation::LegacyVisualConversionResult result;
    XRay::Animation::LegacyVisualConversionOptions options;
    options.build_skeleton = true;
    options.build_mesh = true;

    xr_string error;
    ASSERT_TRUE(XRay::Animation::ConvertLegacyVisualToOzzBundle(ogf_path, result, options, &error)) << error.c_str();

    ASSERT_TRUE(result.skeleton);
    EXPECT_FALSE(result.skeleton_binary.empty());
    EXPECT_FALSE(result.mesh_binary.empty());
    EXPECT_FALSE(result.bone_names.empty());
    EXPECT_LT(0u, result.mesh_surface_count);
    EXPECT_FALSE(result.motion_refs.empty());
}

TEST(ConverterIntegration, ConvertAnimationProducesFile)
{
    EXPECT_TRUE(TestConvertAnimationProducesFile());
}

TEST(ConverterIntegration, AnimationCompatibleWithSkeleton)
{
    EXPECT_TRUE(TestAnimationCompatibleWithSkeleton());
}

TEST(ConverterIntegration, AnimationMatchesReference)
{
    EXPECT_TRUE(TestAnimationMatchesReference());
}

TEST(ConverterIntegration, MultipleAnimationConversion)
{
    EXPECT_TRUE(TestMultipleAnimationConversion());
}

TEST(ConverterIntegration, AnimationMetadataIncludesBoneMotions)
{
    EXPECT_TRUE(TestAnimationMetadataIncludesBoneMotions());
}

TEST(ConverterIntegration, AnimationNamesPreserved)
{
    EXPECT_TRUE(TestAnimationNamesPreserved());
}

TEST(ConverterIntegration, OptimizedAnimationMatchesSource)
{
    EXPECT_TRUE(TestOptimizedAnimationMatchesSource());
}

TEST(ConverterIntegration, AssetBundleContainsSkeletonAndMesh)
{
    EXPECT_TRUE(TestAssetBundleContainsSkeletonAndMesh());
}

TEST(LegacyOmfConverter, ConvertsCriticalHitOmf)
{
    EXPECT_TRUE(TestLegacyOmfConverterConvertsCriticalHitOmf());
}

bool TestEmbeddedAnimationsConvertedCorrectly()
{
    // Load OGF with embedded animations
    const auto ogf_path = ResolveProjectPath("res/testdata/embedded_animation/ventil_01.ogf");
    if (!fs::exists(ogf_path))
    {
        std::cerr << "Missing embedded animation test file: " << ogf_path << std::endl;
        return false;
    }

    // Convert OGF to ozz bundle
    XRay::Animation::LegacyVisualConversionResult result;
    XRay::Animation::LegacyVisualConversionOptions options;
    options.build_skeleton = true;
    options.build_mesh = true;

    xr_string error;
    if (!XRay::Animation::ConvertLegacyVisualToOzzBundle(ogf_path, result, options, &error))
    {
        std::cerr << "Conversion failed: " << error.c_str() << std::endl;
        return false;
    }

    // Count embedded animations in the conversion result
    uint32_t ogf_animation_count = 0;
    if (!result.embedded_animation_binary.empty())
    {
        ozz::io::MemoryStream stream;
        if (!stream.Write(result.embedded_animation_binary.data(), result.embedded_animation_binary.size()))
        {
            std::cerr << "Failed to stage embedded animation data" << std::endl;
            return false;
        }
        stream.Seek(0, ozz::io::Stream::kSet);

        ozz::io::IArchive archive(&stream);
        archive >> ogf_animation_count;
        std::cout << "[OGF] Found " << ogf_animation_count << " embedded animations" << std::endl;
    }
    else
    {
        std::cerr << "OGF conversion result missing embedded animation data" << std::endl;
        return false;
    }

    if (ogf_animation_count == 0)
    {
        std::cerr << "OGF should have embedded animations" << std::endl;
        return false;
    }

    // Write bundle to OZZX file
    const auto ozzx_path = ResolveProjectPath("src/xrAnimation/tests/testdata/ventil_01_test.ozzx");
    XRay::Animation::OzzxBundle bundle;
    bundle.version = 3u;
    bundle.model_type = result.model_type;
    bundle.skeleton = result.skeleton_binary;
    bundle.mesh = result.mesh_binary;
    bundle.bone_metadata = result.bone_metadata;
    bundle.user_data = result.user_data;
    bundle.embedded_animation_data = result.embedded_animation_binary;
    bundle.motion_refs = result.motion_refs;

    if (!XRay::Animation::WriteOzzxBundle(ozzx_path, bundle))
    {
        std::cerr << "Failed to write OZZX bundle" << std::endl;
        return false;
    }

    // Read bundle back
    XRay::Animation::OzzxBundle read_bundle;
    if (!XRay::Animation::ReadOzzxBundle(ozzx_path, read_bundle))
    {
        std::cerr << "Failed to read OZZX bundle" << std::endl;
        return false;
    }

    // Count embedded animations in the OZZX bundle
    uint32_t ozzx_animation_count = 0;
    if (!read_bundle.embedded_animation_data.empty())
    {
        ozz::io::MemoryStream stream;
        if (!stream.Write(read_bundle.embedded_animation_data.data(), read_bundle.embedded_animation_data.size()))
        {
            std::cerr << "Failed to stage OZZX embedded animation data" << std::endl;
            return false;
        }
        stream.Seek(0, ozz::io::Stream::kSet);

        ozz::io::IArchive archive(&stream);
        archive >> ozzx_animation_count;
        std::cout << "[OZZX] Found " << ozzx_animation_count << " embedded animations" << std::endl;
    }
    else
    {
        std::cerr << "OZZX bundle missing embedded animation data" << std::endl;
        return false;
    }

    // Compare counts
    if (ogf_animation_count != ozzx_animation_count)
    {
        std::cerr << "Animation count mismatch: OGF=" << ogf_animation_count
                  << ", OZZX=" << ozzx_animation_count << std::endl;
        return false;
    }

    // Verify the animations can be loaded
    for (uint32_t i = 0; i < ozzx_animation_count; ++i)
    {
        ozz::animation::Animation animation;
        ozz::io::MemoryStream stream;
        if (!stream.Write(read_bundle.embedded_animation_data.data(), read_bundle.embedded_animation_data.size()))
        {
            std::cerr << "Failed to stage animation data for validation" << std::endl;
            return false;
        }
        stream.Seek(0, ozz::io::Stream::kSet);
        ozz::io::IArchive archive(&stream);

        uint32_t count = 0;
        archive >> count;

        // Skip to animation i
        for (uint32_t j = 0; j <= i; ++j)
        {
            archive >> animation;

            // Skip metadata after each animation
            SerializedMotionMetadata metadata;
            if (!ReadSerializedMotionMetadata(archive, &metadata))
            {
                std::cerr << "Failed to read metadata for animation " << j << std::endl;
                return false;
            }

            if (j == i)
            {
                std::cout << "  Animation " << i << ": " << metadata.name
                          << " (" << animation.num_tracks() << " tracks, "
                          << animation.duration() << "s)" << std::endl;
            }
        }

        if (animation.num_tracks() == 0)
        {
            std::cerr << "Animation " << i << " has no tracks" << std::endl;
            return false;
        }
    }

    std::cout << "✓ Embedded animations converted correctly" << std::endl;
    return true;
}

TEST(ConverterIntegration, EmbeddedAnimationsConvertedCorrectly)
{
    EXPECT_TRUE(TestEmbeddedAnimationsConvertedCorrectly());
}
