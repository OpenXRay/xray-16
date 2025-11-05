#include "stdafx.h"

#include "OzzBundle.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>

namespace XRay
{
namespace Animation
{
namespace
{
constexpr char kOzzxMagic[8] = { 'O', 'Z', 'Z', 'X', 'P', 'A', 'C', 'K' };
constexpr char kBoneMetadataMagic[4] = { 'B', 'M', 'D', 'T' };
constexpr std::uint32_t kBoneMetadataVersion = 2u;
constexpr char kEmbeddedAnimationsMagic[4] = { 'A', 'N', 'I', 'M' };
constexpr char kUserDataMagic[4] = { 'U', 'D', 'T', 'A' };

void LogReadFailure(const std::filesystem::path& path, const char* stage, std::streampos position, const std::string& context = {})
{
    std::cerr << "[OzzBundle] " << stage;
    if (!context.empty())
        std::cerr << ": " << context;
    if (position != std::streampos(-1))
        std::cerr << " (offset " << static_cast<std::streamoff>(position) << ")";
    std::cerr << " — " << path << std::endl;
}
}

static bool ReadFully(std::ifstream& stream, void* buffer, std::size_t size)
{
    stream.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(stream.gcount()) == size;
}

static bool WriteFully(std::ofstream& stream, const void* buffer, std::size_t size)
{
    stream.write(reinterpret_cast<const char*>(buffer), static_cast<std::streamsize>(size));
    return static_cast<bool>(stream);
}

static bool ReadBoneMetadataBlock(std::ifstream& stream, const std::filesystem::path& path, ExtendedBoneMetadataCollection& out_metadata)
{
    const std::streampos checkpoint = stream.tellg();

    char magic[sizeof(kBoneMetadataMagic)] = {};
    if (!ReadFully(stream, magic, sizeof(magic)))
    {
        stream.clear();
        stream.seekg(checkpoint, std::ios::beg);
        out_metadata.clear();
        return true;
    }

    if (std::memcmp(magic, kBoneMetadataMagic, sizeof(kBoneMetadataMagic)) != 0)
    {
        stream.seekg(checkpoint, std::ios::beg);
        out_metadata.clear();
        return true;
    }

    std::uint32_t metadata_version = 0;
    const auto version_offset = stream.tellg();
    if (!ReadFully(stream, &metadata_version, sizeof(metadata_version)))
    {
        LogReadFailure(path, "read bone metadata version", version_offset);
        return false;
    }

    if (metadata_version != 1u && metadata_version != kBoneMetadataVersion)
    {
        std::cerr << "Unsupported bone metadata version in .ozzx bundle: " << metadata_version << std::endl;
        return false;
    }

    std::uint32_t count = 0;
    const auto count_offset = stream.tellg();
    if (!ReadFully(stream, &count, sizeof(count)))
    {
        LogReadFailure(path, "read bone metadata count", count_offset);
        return false;
    }

    out_metadata.clear();
    out_metadata.resize(count);

    for (std::uint32_t index = 0; index < count; ++index)
    {
        auto& entry = out_metadata[index];

        auto field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.shape, sizeof(entry.shape)))
        {
            LogReadFailure(path, "read bone metadata shape", field_offset);
            return false;
        }
        if (metadata_version >= 2u)
        {
            field_offset = stream.tellg();
            if (!ReadFully(stream, &entry.obb, sizeof(entry.obb)))
            {
                LogReadFailure(path, "read bone metadata obb", field_offset);
                return false;
            }
        }
        else
        {
            entry.obb.invalidate();
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.joint, sizeof(entry.joint)))
        {
            LogReadFailure(path, "read bone metadata joint", field_offset);
            return false;
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.mass, sizeof(entry.mass)))
        {
            LogReadFailure(path, "read bone metadata mass", field_offset);
            return false;
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.center_of_mass, sizeof(entry.center_of_mass)))
        {
            LogReadFailure(path, "read bone metadata center_of_mass", field_offset);
            return false;
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.rest_length, sizeof(entry.rest_length)))
        {
            LogReadFailure(path, "read bone metadata rest_length", field_offset);
            return false;
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.dominant_axis, sizeof(entry.dominant_axis)))
        {
            LogReadFailure(path, "read bone metadata dominant_axis", field_offset);
            return false;
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.local_aabb_min, sizeof(entry.local_aabb_min)))
        {
            LogReadFailure(path, "read bone metadata local_aabb_min", field_offset);
            return false;
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.local_aabb_max, sizeof(entry.local_aabb_max)))
        {
            LogReadFailure(path, "read bone metadata local_aabb_max", field_offset);
            return false;
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.inverse_global_transform, sizeof(entry.inverse_global_transform)))
        {
            LogReadFailure(path, "read bone metadata inverse_global_transform", field_offset);
            return false;
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.inertia_tensor, sizeof(entry.inertia_tensor)))
        {
            LogReadFailure(path, "read bone metadata inertia_tensor", field_offset);
            return false;
        }
        field_offset = stream.tellg();
        if (!ReadFully(stream, &entry.volume, sizeof(entry.volume)))
        {
            LogReadFailure(path, "read bone metadata volume", field_offset);
            return false;
        }

        std::uint32_t layers = 0;
        field_offset = stream.tellg();
        if (!ReadFully(stream, &layers, sizeof(layers)))
        {
            LogReadFailure(path, "read bone metadata collision layers", field_offset);
            return false;
        }
        entry.collision_layers.assign(layers);

        std::uint8_t ground = 0;
        std::uint8_t weapon = 0;
        field_offset = stream.tellg();
        if (!ReadFully(stream, &ground, sizeof(ground)) || !ReadFully(stream, &weapon, sizeof(weapon)))
        {
            LogReadFailure(path, "read bone metadata contact flags", field_offset);
            return false;
        }
        entry.ground_contact_candidate = ground != 0;
        entry.weapon_anchor_candidate = weapon != 0;

        std::uint32_t material_length = 0;
        field_offset = stream.tellg();
        if (!ReadFully(stream, &material_length, sizeof(material_length)))
        {
            LogReadFailure(path, "read bone metadata material length", field_offset);
            return false;
        }

        xr_string material;
        material.resize(material_length);
        if (material_length > 0)
        {
            field_offset = stream.tellg();
            if (!ReadFully(stream, material.data(), material_length))
            {
                LogReadFailure(path, "read bone metadata material payload", field_offset);
                return false;
            }
        }
        entry.game_material = std::move(material);
    }

    return true;
}

static bool WriteBoneMetadataBlock(std::ofstream& stream, const ExtendedBoneMetadataCollection& metadata)
{
    if (metadata.size() > static_cast<size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        std::cerr << "Too many bones for metadata block in .ozzx bundle: " << metadata.size() << std::endl;
        return false;
    }

    if (!WriteFully(stream, kBoneMetadataMagic, sizeof(kBoneMetadataMagic)))
        return false;

    const std::uint32_t version = kBoneMetadataVersion;
    if (!WriteFully(stream, &version, sizeof(version)))
        return false;

    const std::uint32_t count = static_cast<std::uint32_t>(metadata.size());
    if (!WriteFully(stream, &count, sizeof(count)))
        return false;

    for (const auto& entry : metadata)
    {
        if (!WriteFully(stream, &entry.shape, sizeof(entry.shape)))
            return false;
        if (!WriteFully(stream, &entry.obb, sizeof(entry.obb)))
            return false;
        if (!WriteFully(stream, &entry.joint, sizeof(entry.joint)))
            return false;
        if (!WriteFully(stream, &entry.mass, sizeof(entry.mass)))
            return false;
        if (!WriteFully(stream, &entry.center_of_mass, sizeof(entry.center_of_mass)))
            return false;
        if (!WriteFully(stream, &entry.rest_length, sizeof(entry.rest_length)))
            return false;
        if (!WriteFully(stream, &entry.dominant_axis, sizeof(entry.dominant_axis)))
            return false;
        if (!WriteFully(stream, &entry.local_aabb_min, sizeof(entry.local_aabb_min)))
            return false;
        if (!WriteFully(stream, &entry.local_aabb_max, sizeof(entry.local_aabb_max)))
            return false;
        if (!WriteFully(stream, &entry.inverse_global_transform, sizeof(entry.inverse_global_transform)))
            return false;
        if (!WriteFully(stream, &entry.inertia_tensor, sizeof(entry.inertia_tensor)))
            return false;
        if (!WriteFully(stream, &entry.volume, sizeof(entry.volume)))
            return false;

        const std::uint32_t layers = entry.collision_layers.get();
        if (!WriteFully(stream, &layers, sizeof(layers)))
            return false;

        const std::uint8_t ground = entry.ground_contact_candidate ? 1u : 0u;
        const std::uint8_t weapon = entry.weapon_anchor_candidate ? 1u : 0u;
        if (!WriteFully(stream, &ground, sizeof(ground)) || !WriteFully(stream, &weapon, sizeof(weapon)))
            return false;

        const xr_string& material = entry.game_material;
        if (material.size() > static_cast<size_t>(std::numeric_limits<std::uint32_t>::max()))
        {
            std::cerr << "Bone material string too large for .ozzx bundle" << std::endl;
            return false;
        }

        const std::uint32_t length = static_cast<std::uint32_t>(material.size());
        if (!WriteFully(stream, &length, sizeof(length)))
            return false;
        if (length > 0 && !WriteFully(stream, material.data(), length))
            return false;
    }

    return true;
}

static bool ReadUserDataBlock(std::ifstream& stream, const std::filesystem::path& path, std::vector<std::uint8_t>& out_user_data)
{
    const std::streampos checkpoint = stream.tellg();

    char magic[sizeof(kUserDataMagic)] = {};
    if (!ReadFully(stream, magic, sizeof(magic)))
    {
        stream.clear();
        stream.seekg(checkpoint, std::ios::beg);
        out_user_data.clear();
        return true;
    }

    if (std::memcmp(magic, kUserDataMagic, sizeof(kUserDataMagic)) != 0)
    {
        stream.seekg(checkpoint, std::ios::beg);
        out_user_data.clear();
        return true;
    }

    std::uint32_t data_size = 0;
    const auto size_offset = stream.tellg();
    if (!ReadFully(stream, &data_size, sizeof(data_size)))
    {
        LogReadFailure(path, "read user data size", size_offset);
        return false;
    }

    out_user_data.clear();
    out_user_data.resize(data_size);
    if (data_size > 0)
    {
        const auto payload_offset = stream.tellg();
        if (!ReadFully(stream, out_user_data.data(), data_size))
        {
            LogReadFailure(path, "read user data payload", payload_offset);
            return false;
        }
    }

    return true;
}

static bool WriteUserDataBlock(std::ofstream& stream, const std::vector<std::uint8_t>& user_data)
{
    if (user_data.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        std::cerr << "User data payload too large for .ozzx bundle: " << user_data.size() << std::endl;
        return false;
    }

    if (!WriteFully(stream, kUserDataMagic, sizeof(kUserDataMagic)))
        return false;

    const std::uint32_t data_size = static_cast<std::uint32_t>(user_data.size());
    if (!WriteFully(stream, &data_size, sizeof(data_size)))
        return false;

    if (data_size > 0 && !WriteFully(stream, user_data.data(), user_data.size()))
        return false;

    return true;
}

bool ReadOzzxBundle(const std::filesystem::path& path, OzzxBundle& out_bundle)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        const std::error_code ec(errno, std::generic_category());
        std::cerr << "[OzzBundle] open bundle failed (errno " << ec.value() << ": " << ec.message() << ") — " << path << std::endl;
        return false;
    }

    char magic[sizeof(kOzzxMagic)] = {};
    const auto magic_offset = stream.tellg();
    if (!ReadFully(stream, magic, sizeof(magic)))
    {
        LogReadFailure(path, "read bundle magic", magic_offset);
        return false;
    }

    if (std::memcmp(magic, kOzzxMagic, sizeof(kOzzxMagic)) != 0)
    {
        const std::string actual(magic, magic + sizeof(kOzzxMagic));
        LogReadFailure(path, "unexpected bundle magic", magic_offset, actual);
        return false;
    }

    std::uint32_t version = 0;
    std::uint8_t model_type = 0;
    std::uint32_t skeleton_size = 0;
    std::uint32_t mesh_size = 0;

    const auto header_offset = stream.tellg();
    if (!ReadFully(stream, &version, sizeof(version)))
    {
        LogReadFailure(path, "read bundle version", header_offset);
        return false;
    }

    if (version >= 3u)
    {
        if (!ReadFully(stream, &model_type, sizeof(model_type)))
        {
            LogReadFailure(path, "read model_type", stream.tellg());
            return false;
        }
    }

    if (!ReadFully(stream, &skeleton_size, sizeof(skeleton_size)) || !ReadFully(stream, &mesh_size, sizeof(mesh_size)))
    {
        LogReadFailure(path, "read bundle sizes", stream.tellg());
        return false;
    }

    std::cout << "[OzzBundle] header version=" << version << ", model_type=" << static_cast<int>(model_type) << ", skeleton_size=" << skeleton_size
              << ", mesh_size=" << mesh_size << std::endl;

    out_bundle.version = version;
    out_bundle.model_type = model_type;
    out_bundle.skeleton.resize(skeleton_size);
    out_bundle.mesh.resize(mesh_size);

    const auto skeleton_offset = stream.tellg();
    if (!ReadFully(stream, out_bundle.skeleton.data(), out_bundle.skeleton.size()))
    {
        LogReadFailure(path, "read skeleton payload", skeleton_offset);
        return false;
    }
    std::cout << "[OzzBundle] read skeleton payload (" << out_bundle.skeleton.size() << " bytes)" << std::endl;

    const auto mesh_offset = stream.tellg();
    if (!ReadFully(stream, out_bundle.mesh.data(), out_bundle.mesh.size()))
    {
        LogReadFailure(path, "read mesh payload", mesh_offset);
        return false;
    }
    std::cout << "[OzzBundle] read mesh payload (" << out_bundle.mesh.size() << " bytes)" << std::endl;

    if (!ReadBoneMetadataBlock(stream, path, out_bundle.bone_metadata))
    {
        LogReadFailure(path, "read bone metadata block", stream.tellg());
        return false;
    }
    std::cout << "[OzzBundle] bone metadata entries=" << out_bundle.bone_metadata.size() << std::endl;

    if (!ReadUserDataBlock(stream, path, out_bundle.user_data))
    {
        LogReadFailure(path, "read user data block", stream.tellg());
        return false;
    }
    std::cout << "[OzzBundle] user data bytes=" << out_bundle.user_data.size() << std::endl;

    const std::streampos anim_block_offset = stream.tellg();
    char anim_magic[sizeof(kEmbeddedAnimationsMagic)] = {};
    if (!ReadFully(stream, anim_magic, sizeof(anim_magic)))
    {
        stream.clear();
        stream.seekg(anim_block_offset, std::ios::beg);
    }

    if (std::memcmp(anim_magic, kEmbeddedAnimationsMagic, sizeof(kEmbeddedAnimationsMagic)) == 0)
    {
        std::uint32_t animation_data_size = 0;
        const auto anim_size_offset = stream.tellg();
        if (!ReadFully(stream, &animation_data_size, sizeof(animation_data_size)))
        {
            LogReadFailure(path, "read embedded animation size", anim_size_offset);
            return false;
        }

        out_bundle.embedded_animation_data.clear();
        out_bundle.embedded_animation_data.resize(animation_data_size);
        if (animation_data_size > 0)
        {
            const auto anim_payload_offset = stream.tellg();
            if (!ReadFully(stream, out_bundle.embedded_animation_data.data(), animation_data_size))
            {
                LogReadFailure(path, "read embedded animation payload", anim_payload_offset);
                return false;
            }
        }
    }
    else
    {
        stream.seekg(anim_block_offset, std::ios::beg);
        out_bundle.embedded_animation_data.clear();
    }
    std::cout << "[OzzBundle] embedded animation bytes=" << out_bundle.embedded_animation_data.size() << std::endl;

    out_bundle.motion_refs.clear();

    std::uint32_t motion_ref_count = 0;
    const auto motion_count_offset = stream.tellg();
    if (!ReadFully(stream, &motion_ref_count, sizeof(motion_ref_count)))
    {
        LogReadFailure(path, "read motion reference count", motion_count_offset);
        return false;
    }

    std::cout << "[OzzBundle] motion_refs count=" << motion_ref_count << std::endl;

    if (motion_ref_count > 0)
    {
        out_bundle.motion_refs.reserve(motion_ref_count);
        for (std::uint32_t index = 0; index < motion_ref_count; ++index)
        {
            std::uint32_t length = 0;
            const auto ref_length_offset = stream.tellg();
            if (!ReadFully(stream, &length, sizeof(length)))
            {
                LogReadFailure(path, "read motion reference length", ref_length_offset);
                return false;
            }

            xr_string entry;
            entry.resize(length, '\0');

            if (length > 0)
            {
                const auto ref_data_offset = stream.tellg();
                if (!ReadFully(stream, entry.data(), length))
                {
                    LogReadFailure(path, "read motion reference data", ref_data_offset);
                    return false;
                }
            }

            out_bundle.motion_refs.emplace_back(std::move(entry));
        }
    }

    const std::streampos end_pos = stream.tellg();
    stream.seekg(0, std::ios::end);
    const std::streampos file_size = stream.tellg();
    stream.seekg(end_pos, std::ios::beg);
    std::cout << "[OzzBundle] consumed " << static_cast<std::streamoff>(end_pos) << " / " << static_cast<std::streamoff>(file_size) << " bytes" << std::endl;

    return true;
}

bool WriteOzzxBundle(const std::filesystem::path& path, const OzzxBundle& bundle)
{
    if (bundle.skeleton.size() > std::numeric_limits<std::uint32_t>::max())
    {
        std::cerr << "Skeleton payload too large for .ozzx bundle: " << bundle.skeleton.size() << std::endl;
        return false;
    }

    if (bundle.mesh.size() > std::numeric_limits<std::uint32_t>::max())
    {
        std::cerr << "Mesh payload too large for .ozzx bundle: " << bundle.mesh.size() << std::endl;
        return false;
    }

    std::error_code ec;
    if (const auto parent = path.parent_path(); !parent.empty())
        std::filesystem::create_directories(parent, ec);

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        std::cerr << "Failed to open bundle for writing: " << path << std::endl;
        return false;
    }

    if (!WriteFully(stream, kOzzxMagic, sizeof(kOzzxMagic)))
    {
        std::cerr << "Failed to write bundle magic: " << path << std::endl;
        return false;
    }

    const std::uint32_t version = bundle.version;
    const std::uint8_t model_type = bundle.model_type;

    const std::uint32_t skeleton_size = static_cast<std::uint32_t>(bundle.skeleton.size());
    const std::uint32_t mesh_size = static_cast<std::uint32_t>(bundle.mesh.size());
    const std::uint32_t motion_ref_count = static_cast<std::uint32_t>(bundle.motion_refs.size());

    if (!WriteFully(stream, &version, sizeof(version)))
    {
        std::cerr << "Failed to write bundle version: " << path << std::endl;
        return false;
    }

    if (version >= 3u)
    {
        if (!WriteFully(stream, &model_type, sizeof(model_type)))
        {
            std::cerr << "Failed to write model_type: " << path << std::endl;
            return false;
        }
    }

    if (!WriteFully(stream, &skeleton_size, sizeof(skeleton_size)) || !WriteFully(stream, &mesh_size, sizeof(mesh_size)))
    {
        std::cerr << "Failed to write bundle sizes: " << path << std::endl;
        return false;
    }

    if (!bundle.skeleton.empty() && !WriteFully(stream, bundle.skeleton.data(), bundle.skeleton.size()))
    {
        std::cerr << "Failed to write skeleton payload: " << path << std::endl;
        return false;
    }

    if (!bundle.mesh.empty() && !WriteFully(stream, bundle.mesh.data(), bundle.mesh.size()))
    {
        std::cerr << "Failed to write mesh payload: " << path << std::endl;
        return false;
    }

    if (!WriteBoneMetadataBlock(stream, bundle.bone_metadata))
    {
        std::cerr << "Failed to write bone metadata block: " << path << std::endl;
        return false;
    }

    if (!bundle.user_data.empty())
    {
        if (!WriteUserDataBlock(stream, bundle.user_data))
        {
            std::cerr << "Failed to write user data block: " << path << std::endl;
            return false;
        }
    }

    if (!bundle.embedded_animation_data.empty())
    {
        if (!WriteFully(stream, kEmbeddedAnimationsMagic, sizeof(kEmbeddedAnimationsMagic)))
        {
            std::cerr << "Failed to write embedded animation magic: " << path << std::endl;
            return false;
        }

        if (bundle.embedded_animation_data.size() > static_cast<size_t>(std::numeric_limits<std::uint32_t>::max()))
        {
            std::cerr << "Embedded animation payload too large for .ozzx bundle: " << bundle.embedded_animation_data.size() << std::endl;
            return false;
        }

        const std::uint32_t animation_size = static_cast<std::uint32_t>(bundle.embedded_animation_data.size());
        if (!WriteFully(stream, &animation_size, sizeof(animation_size)))
        {
            std::cerr << "Failed to write embedded animation size: " << path << std::endl;
            return false;
        }

        if (animation_size > 0 && !WriteFully(stream, bundle.embedded_animation_data.data(), bundle.embedded_animation_data.size()))
        {
            std::cerr << "Failed to write embedded animation payload: " << path << std::endl;
            return false;
        }
    }

    if (!WriteFully(stream, &motion_ref_count, sizeof(motion_ref_count)))
    {
        std::cerr << "Failed to write motion reference count: " << path << std::endl;
        return false;
    }

    for (const auto& reference : bundle.motion_refs)
    {
        const std::uint32_t length = static_cast<std::uint32_t>(reference.size());
        if (!WriteFully(stream, &length, sizeof(length)))
        {
            std::cerr << "Failed to write motion reference length: " << path << std::endl;
            return false;
        }

        if (length > 0)
        {
            if (!WriteFully(stream, reference.data(), length))
            {
                std::cerr << "Failed to write motion reference data: " << path << std::endl;
                return false;
            }
        }
    }

    return true;
}
} // namespace Animation
} // namespace XRay
