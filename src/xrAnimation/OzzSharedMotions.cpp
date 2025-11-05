#include "stdafx.h"
#include "OzzSharedMotions.hpp"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "xrCore/FS.h"
#include "xrCore/FS_impl.h"
#include "xrCore/_std_extensions.h" // for crc32

#include <algorithm>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace XRay::Animation
{

namespace
{
std::string ReadOzzString(ozz::io::IArchive& archive)
{
    uint32_t length = 0;
    archive >> length;

    // Security: Limit string length to prevent excessive allocations
    constexpr uint32_t kMaxStringLength = 256;
    if (length > kMaxStringLength)
    {
        Msg("[OzzMotions] Warning: String length %u exceeds max %u, truncating", length, kMaxStringLength);
        length = kMaxStringLength;
    }

    std::string result;
    if (length == 0)
        return result;

    result.resize(length);
    archive >> ozz::io::MakeArray(result.data(), length);
    return result;
}

struct MotionBoneData
{
    u16 bone_id = BI_NONE;
    u8 flags = 0;
    u8 translation_format = 0;
    u32 rotation_crc = 0;
    u32 translation_crc = 0;
    xr_vector<CKeyQR> rotation_keys;
    xr_vector<CKeyQT8> translation_keys8;
    xr_vector<CKeyQT16> translation_keys16;
    Fvector translation_size{};
    Fvector translation_init{};
};

struct MotionMetadata
{
    xr_string name;
    uint32_t flags = 0;
    uint16_t bone_or_part = 0;
    uint16_t motion_id = 0;
    float speed = 1.f;
    float power = 1.f;
    float accrue = 0.f;
    float falloff = 0.f;
    uint32_t frame_count = 0;
    xr_vector<MotionBoneData> bone_motions;
};

MotionMetadata ReadMotionMetadataFromArchive(ozz::io::IArchive& archive)
{
    MotionMetadata metadata;

    std::string name = ReadOzzString(archive);
    metadata.name = name.c_str();

    archive >> metadata.flags;
    archive >> metadata.bone_or_part;
    archive >> metadata.motion_id;
    archive >> metadata.speed;
    archive >> metadata.power;
    archive >> metadata.accrue;
    archive >> metadata.falloff;

    uint32_t mark_count = 0;
    archive >> mark_count;

    // Security: Limit mark count
    constexpr uint32_t kMaxMarks = 100;
    if (mark_count > kMaxMarks)
    {
        Msg("[OzzMotions] Warning: Mark count %u exceeds max %u, truncating", mark_count, kMaxMarks);
        mark_count = kMaxMarks;
    }

    for (uint32_t mark_index = 0; mark_index < mark_count; ++mark_index)
    {
        ReadOzzString(archive);

        uint32_t interval_count = 0;
        archive >> interval_count;
        for (uint32_t interval_index = 0; interval_index < interval_count; ++interval_index)
        {
            float start = 0.f;
            float end = 0.f;
            archive >> start;
            archive >> end;
        }
    }

    archive >> metadata.frame_count;

    uint32_t bone_motion_count = 0;
    archive >> bone_motion_count;

    // Security: Limit bone motion count to prevent excessive allocations
    constexpr uint32_t kMaxBoneMotions = 512;
    if (bone_motion_count > kMaxBoneMotions)
    {
        Msg("[OzzMotions] ERROR: Bone motion count %u exceeds max %u", bone_motion_count, kMaxBoneMotions);
        bone_motion_count = kMaxBoneMotions;
    }

    metadata.bone_motions.resize(bone_motion_count);

    for (uint32_t bone_index = 0; bone_index < bone_motion_count; ++bone_index)
    {
        MotionBoneData& bone = metadata.bone_motions[bone_index];

        archive >> bone.bone_id;

        uint8_t flags = 0;
        archive >> flags;
        bone.flags = flags;

        uint8_t translation_format = 0;
        archive >> translation_format;
        bone.translation_format = translation_format;

        archive >> bone.rotation_crc;
        archive >> bone.translation_crc;

        uint32_t rotation_key_count = 0;
        archive >> rotation_key_count;
        bone.rotation_keys.clear();
        bone.rotation_keys.resize(rotation_key_count);
        for (uint32_t key_index = 0; key_index < rotation_key_count; ++key_index)
        {
            s16 x = 0;
            s16 y = 0;
            s16 z = 0;
            s16 w = 0;
            archive >> x;
            archive >> y;
            archive >> z;
            archive >> w;
            bone.rotation_keys[key_index].x = x;
            bone.rotation_keys[key_index].y = y;
            bone.rotation_keys[key_index].z = z;
            bone.rotation_keys[key_index].w = w;
        }

        uint32_t translation_key_count = 0;
        archive >> translation_key_count;

        switch (translation_format)
        {
        case 1:
            bone.translation_keys8.clear();
            bone.translation_keys8.resize(translation_key_count);
            bone.translation_keys16.clear();
            for (uint32_t key_index = 0; key_index < translation_key_count; ++key_index)
            {
                s8 x = 0;
                s8 y = 0;
                s8 z = 0;
                archive >> x;
                archive >> y;
                archive >> z;
                bone.translation_keys8[key_index].x1 = x;
                bone.translation_keys8[key_index].y1 = y;
                bone.translation_keys8[key_index].z1 = z;
            }
            break;
        case 2:
            bone.translation_keys16.clear();
            bone.translation_keys16.resize(translation_key_count);
            bone.translation_keys8.clear();
            for (uint32_t key_index = 0; key_index < translation_key_count; ++key_index)
            {
                s16 x = 0;
                s16 y = 0;
                s16 z = 0;
                archive >> x;
                archive >> y;
                archive >> z;
                bone.translation_keys16[key_index].x1 = x;
                bone.translation_keys16[key_index].y1 = y;
                bone.translation_keys16[key_index].z1 = z;
            }
            break;
        default:
            bone.translation_keys8.clear();
            bone.translation_keys16.clear();
            break;
        }

        archive >> bone.translation_size.x;
        archive >> bone.translation_size.y;
        archive >> bone.translation_size.z;
        archive >> bone.translation_init.x;
        archive >> bone.translation_init.y;
        archive >> bone.translation_init.z;
    }

    return metadata;
}

u32 ComputeOrDefaultCrc(u32 provided_crc, const void* data, size_t byte_count)
{
    if (provided_crc != 0 || byte_count == 0 || data == nullptr)
        return provided_crc;
    return crc32(data, static_cast<u32>(byte_count));
}
} // anonymous namespace

SkeletonFingerprint SkeletonFingerprint::Compute(const ozz::animation::Skeleton& skeleton)
{
    SkeletonFingerprint fp;
    fp.jointCount = static_cast<u16>(skeleton.num_joints());
    fp.version = 1;

    auto names = skeleton.joint_names();
    u32 hash = 0;

    for (int i = 0; i < skeleton.num_joints(); ++i)
    {
        const char* name = names[i];
        hash = crc32(name, static_cast<u32>(xr_strlen(name)), hash);
    }

    fp.hash = hash;
    return fp;
}

bool SkeletonFingerprint::Matches(const SkeletonFingerprint& other) const
{
    return (hash == other.hash) && (jointCount == other.jointCount);
}

u32 OzzMotionsValue::MotionRecord::GetMemoryUsage() const
{
    u32 size = sizeof(MotionRecord);
    size += static_cast<u32>(name.size());
    if (animation)
        size += static_cast<u32>(animation->size());
    return size;
}

bool OzzMotionsValue::Load(pcstr file_path, const ozz::animation::Skeleton& skeleton)
{
    Msg("[OzzMotionsContainer] Starting Load for file: %s", file_path);

    MotionLoadState expected = MotionLoadState::Unloaded;
    if (!loadState.compare_exchange_strong(expected, MotionLoadState::Loading))
    {
        bool isLoaded = loadState.load() == MotionLoadState::Loaded;
        Msg("[OzzMotionsContainer] File '%s' already %s", file_path, isLoaded ? "loaded" : "loading");
        return isLoaded;
    }

    skelFingerprint = SkeletonFingerprint::Compute(skeleton);
    sourceFile = file_path;

    Msg("[OzzMotionsContainer] Skeleton fingerprint for '%s': hash=0x%08X, joints=%u", file_path, skelFingerprint.hash, skelFingerprint.jointCount);

    string_path resolved;
    FileStatus status(false, false);
    xr_string source_alias;

    constexpr pcstr kSearchAliases[] = { "$game_meshes$", "$levels$" };
    Msg("[OzzMotionsContainer] Searching for file '%s' in aliases...", file_path);
    for (pcstr alias : kSearchAliases)
    {
        status = FS.exist(resolved, alias, file_path);
        if (status)
        {
            source_alias = alias;
            Msg("[OzzMotionsContainer] Found '%s' in alias '%s' -> %s", file_path, alias, resolved);
            break;
        }
        else
        {
            Msg("[OzzMotionsContainer] Not found in '%s'", alias);
        }
    }

    if (!status)
    {
        Msg("[OzzMotionsContainer] ERROR: File not found in any search path: %s", file_path);
        loadState.store(MotionLoadState::Failed, std::memory_order_release);
        return false;
    }

    std::vector<std::byte> payload;
    if (status.External)
    {
        Msg("[OzzMotionsContainer] Loading external file: %s", resolved);
        std::error_code ec;
        fs::path absolute = fs::weakly_canonical(fs::path(resolved), ec);
        if (ec || !fs::exists(absolute, ec))
        {
            Msg("[OzzMotionsContainer] ERROR: Failed to resolve path: %s", resolved);
            loadState.store(MotionLoadState::Failed, std::memory_order_release);
            return false;
        }

        ozz::io::File file(absolute.string().c_str(), "rb");
        if (!file.opened())
        {
            Msg("[OzzMotionsContainer] ERROR: Failed to open file: %s", absolute.string().c_str());
            loadState.store(MotionLoadState::Failed, std::memory_order_release);
            return false;
        }

        const int64_t length = file.Size();
        if (length < 0)
        {
            Msg("[OzzMotionsContainer] ERROR: Invalid file size for: %s", absolute.string().c_str());
            loadState.store(MotionLoadState::Failed, std::memory_order_release);
            return false;
        }

        Msg("[OzzMotionsContainer] File size: %lld bytes", length);
        payload.resize(static_cast<size_t>(length));
        if (!payload.empty())
        {
            const size_t bytes_read = file.Read(payload.data(), payload.size());
            if (bytes_read != payload.size())
            {
                Msg("[OzzMotionsContainer] ERROR: Failed to read %zu bytes (only got %zu)", payload.size(), bytes_read);
                loadState.store(MotionLoadState::Failed, std::memory_order_release);
                return false;
            }
            Msg("[OzzMotionsContainer] Successfully read %zu bytes", bytes_read);
        }
    }
    else
    {
        Msg("[OzzMotionsContainer] Loading from FS: %s:%s", source_alias.c_str(), file_path);
        IReader* reader = FS.r_open(source_alias.c_str(), file_path);
        if (!reader)
        {
            Msg("[OzzMotionsContainer] ERROR: Failed to open reader for: %s:%s", source_alias.c_str(), file_path);
            loadState.store(MotionLoadState::Failed, std::memory_order_release);
            return false;
        }

        const size_t length = static_cast<size_t>(reader->length());
        Msg("[OzzMotionsContainer] File size: %zu bytes", length);
        payload.resize(length);
        if (length > 0)
        {
            if (!reader->pointer())
            {
                Msg("[OzzMotionsContainer] ERROR: Reader has no pointer");
                FS.r_close(reader);
                loadState.store(MotionLoadState::Failed, std::memory_order_release);
                return false;
            }
            std::memcpy(payload.data(), reader->pointer(), length);
            Msg("[OzzMotionsContainer] Successfully read %zu bytes", length);
        }
        FS.r_close(reader);
    }

    if (payload.empty())
    {
        Msg("[OzzMotionsContainer] ERROR: Payload is empty after loading");
        loadState.store(MotionLoadState::Failed, std::memory_order_release);
        return false;
    }

    Msg("[OzzMotionsContainer] Parsing ozz file...");
    ozz::io::MemoryStream stream;
    if (!stream.Write(payload.data(), payload.size()))
    {
        Msg("[OzzMotionsContainer] ERROR: Failed to write to memory stream");
        loadState.store(MotionLoadState::Failed, std::memory_order_release);
        return false;
    }
    stream.Seek(0, ozz::io::Stream::kSet);

    ozz::io::IArchive archive(&stream);
    uint32_t animation_count = 0;
    archive >> animation_count;

    // Security: Validate animation count to prevent excessive allocations
    constexpr uint32_t kMaxAnimations = 1000;
    if (animation_count > kMaxAnimations)
    {
        Msg("[OzzMotionsContainer] ERROR: Animation count %u exceeds max %u", animation_count, kMaxAnimations);
        loadState.store(MotionLoadState::Failed, std::memory_order_release);
        return false;
    }

    Msg("[OzzMotionsContainer] Animation count in file: %u", animation_count);

    const u32 joint_count = skeleton.num_joints();
    auto joint_names = skeleton.joint_names();

    Msg("[OzzMotionsContainer] Skeleton has %u joints", joint_count);

    xr_vector<MotionMetadata> all_metadata;

    if (animation_count == 0)
    {
        Msg("[OzzMotionsContainer] No aggregate animations found, trying as single animation");

        stream.Seek(0, ozz::io::Stream::kSet);
        ozz::io::IArchive single_archive(&stream);

        if (single_archive.TestTag<ozz::animation::Animation>())
        {
            Msg("[OzzMotionsContainer] Found single animation");
            std::shared_ptr<ozz::animation::Animation> animation = std::make_shared<ozz::animation::Animation>();
            single_archive >> *animation;

            if (animation->num_tracks() != skeleton.num_joints())
            {
                Msg("[OzzMotionsContainer] ERROR: Animation track mismatch: anim has %d tracks, skeleton has %d joints", animation->num_tracks(),
                    skeleton.num_joints());
                loadState.store(MotionLoadState::Failed, std::memory_order_release);
                return false;
            }

            MotionMetadata metadata = ReadMotionMetadataFromArchive(single_archive);

            MotionRecord record;
            record.name = metadata.name.empty() ? xr_string(file_path) : metadata.name;
            record.animation = animation;
            record.id.set(0, 0);
            record.frameCount = metadata.frame_count > 0 ? metadata.frame_count : 1;

            Msg("[OzzMotionsContainer] Added motion '%s' with %u frames, %u bone motions", record.name.c_str(), record.frameCount,
                static_cast<u32>(metadata.bone_motions.size()));

            record.definition.bone_or_part = metadata.bone_or_part;
            record.definition.motion = metadata.motion_id;
            record.definition.speed = record.definition.Quantize(std::max(metadata.speed, 0.f));
            record.definition.power = record.definition.Quantize(std::max(metadata.power, 0.f));
            record.definition.accrue = record.definition.Quantize(std::max(metadata.accrue, 0.f));
            record.definition.falloff = record.definition.Quantize(std::max(metadata.falloff, 0.f));
            record.definition.flags = static_cast<u16>(metadata.flags);
            if (!(record.definition.flags & esmFX) && record.definition.falloff >= record.definition.accrue && record.definition.accrue > 0)
                record.definition.falloff = static_cast<u16>(record.definition.accrue - 1);

            records.push_back(record);
            lookup[record.name] = 0;
            all_metadata.push_back(metadata);
        }
        else
        {
            Msg("[OzzMotionsContainer] ERROR: Failed to find valid animation in file");
            loadState.store(MotionLoadState::Failed, std::memory_order_release);
            return false;
        }
    }
    else
    {
        Msg("[OzzMotionsContainer] Loading %u aggregate animations", animation_count);

        for (uint32_t idx = 0; idx < animation_count; ++idx)
        {
            std::shared_ptr<ozz::animation::Animation> animation = std::make_shared<ozz::animation::Animation>();
            archive >> *animation;

            MotionMetadata metadata = ReadMotionMetadataFromArchive(archive);

            MotionRecord record;
            record.name = metadata.name.empty() ? (xr_string(file_path) + "_" + xr_string(std::to_string(idx).c_str())) : metadata.name;
            record.animation = animation;
            record.id.set(0, static_cast<u16>(idx));
            record.frameCount = metadata.frame_count > 0 ? metadata.frame_count : 1;

            Msg("[OzzMotionsContainer] Motion %u: '%s' with %u frames, %u bone motions", idx, record.name.c_str(), record.frameCount,
                static_cast<u32>(metadata.bone_motions.size()));

            record.definition.bone_or_part = metadata.bone_or_part;
            record.definition.motion = metadata.motion_id;
            record.definition.speed = record.definition.Quantize(std::max(metadata.speed, 0.f));
            record.definition.power = record.definition.Quantize(std::max(metadata.power, 0.f));
            record.definition.accrue = record.definition.Quantize(std::max(metadata.accrue, 0.f));
            record.definition.falloff = record.definition.Quantize(std::max(metadata.falloff, 0.f));
            record.definition.flags = static_cast<u16>(metadata.flags);
            if (!(record.definition.flags & esmFX) && record.definition.falloff >= record.definition.accrue && record.definition.accrue > 0)
                record.definition.falloff = static_cast<u16>(record.definition.accrue - 1);

            records.push_back(record);
            lookup[record.name] = static_cast<u16>(idx);
            all_metadata.push_back(metadata);
        }
    }

    Msg("[OzzMotionsContainer] Building bone-major layout for %u bones and %u motions", joint_count, static_cast<u32>(records.size()));

    for (u32 bone_idx = 0; bone_idx < joint_count; ++bone_idx)
    {
        shared_str bone_name(joint_names[bone_idx]);
        bone_motions[bone_name].resize(records.size());
    }

    for (u32 motion_idx = 0; motion_idx < all_metadata.size(); ++motion_idx)
    {
        const MotionMetadata& metadata = all_metadata[motion_idx];
        const u32 frame_count = records[motion_idx].frameCount;

        for (const MotionBoneData& bone_data : metadata.bone_motions)
        {
            if (bone_data.bone_id == BI_NONE || bone_data.bone_id >= joint_count)
                continue;

            const char* bone_name = joint_names[bone_data.bone_id];
            CMotion& motion = bone_motions[shared_str(bone_name)][motion_idx];

            motion.set_flags(bone_data.flags);
            motion.set_count(frame_count);

            if (!bone_data.rotation_keys.empty())
            {
                const u32 crc = ComputeOrDefaultCrc(bone_data.rotation_crc, bone_data.rotation_keys.data(), bone_data.rotation_keys.size() * sizeof(CKeyQR));
                motion._keysR.create(crc, static_cast<u32>(bone_data.rotation_keys.size()), const_cast<CKeyQR*>(bone_data.rotation_keys.data()));
            }

            switch (bone_data.translation_format)
            {
            case 1:
                if (!bone_data.translation_keys8.empty())
                {
                    const u32 crc =
                        ComputeOrDefaultCrc(bone_data.translation_crc, bone_data.translation_keys8.data(), bone_data.translation_keys8.size() * sizeof(CKeyQT8));
                    motion._keysT8.create(crc, static_cast<u32>(bone_data.translation_keys8.size()), const_cast<CKeyQT8*>(bone_data.translation_keys8.data()));
                }
                break;
            case 2:
                if (!bone_data.translation_keys16.empty())
                {
                    const u32 crc = ComputeOrDefaultCrc(bone_data.translation_crc, bone_data.translation_keys16.data(),
                        bone_data.translation_keys16.size() * sizeof(CKeyQT16));
                    motion._keysT16
                        .create(crc, static_cast<u32>(bone_data.translation_keys16.size()), const_cast<CKeyQT16*>(bone_data.translation_keys16.data()));
                }
                break;
            default: break;
            }

            motion._sizeT = bone_data.translation_size;
            motion._initT = bone_data.translation_init;
        }

        for (u32 bone_idx = 0; bone_idx < joint_count; ++bone_idx)
        {
            const char* bone_name = joint_names[bone_idx];
            CMotion& motion = bone_motions[shared_str(bone_name)][motion_idx];

            if (motion._keysR.size() == 0)
            {
                motion.set_flags(flRKeyAbsent);
                motion.set_count(frame_count);

                CKeyQR* identity = xr_new<CKeyQR>();
                identity->x = 0;
                identity->y = 0;
                identity->z = 0;
                identity->w = static_cast<s16>(KEY_Quant);

                const u32 crc = ComputeOrDefaultCrc(0, identity, sizeof(CKeyQR));
                motion._keysR.create(crc, 1, identity);
                xr_delete(identity);
                motion._sizeT.set(0.f, 0.f, 0.f);
                motion._initT.set(0.f, 0.f, 0.f);
            }
        }
    }

    UpdateMemoryUsage();
    MarkAccessed();
    loadState.store(MotionLoadState::Loaded, std::memory_order_release);

    Msg("[OzzMotionsContainer] Successfully loaded '%s':", file_path);
    Msg("[OzzMotionsContainer]   Total motions: %u", static_cast<u32>(records.size()));
    Msg("[OzzMotionsContainer]   Bones in skeleton: %u", joint_count);
    Msg("[OzzMotionsContainer]   Memory usage: %u KB", totalMemoryBytes / 1024);

    for (const auto& record : records)
    {
        Msg("[OzzMotionsContainer]   Motion: '%s' (frames=%u)", record.name.c_str(), record.frameCount);
    }

    return true;
}

bool OzzMotionsValue::LoadFromMemory(const std::vector<std::uint8_t>& data, const ozz::animation::Skeleton& skeleton, pcstr source_label)
{
    if (data.empty())
    {
        Msg("[OzzMotionsContainer] LoadFromMemory: Empty data buffer");
        return false;
    }

    Msg("[OzzMotionsContainer] Starting LoadFromMemory for '%s' (%zu bytes)", source_label, data.size());

    MotionLoadState expected = MotionLoadState::Unloaded;
    if (!loadState.compare_exchange_strong(expected, MotionLoadState::Loading))
    {
        bool isLoaded = loadState.load() == MotionLoadState::Loaded;
        Msg("[OzzMotionsContainer] '%s' already %s", source_label, isLoaded ? "loaded" : "loading");
        return isLoaded;
    }

    skelFingerprint = SkeletonFingerprint::Compute(skeleton);
    sourceFile = source_label;

    Msg("[OzzMotionsContainer] Skeleton fingerprint for '%s': hash=0x%08X, joints=%u", source_label, skelFingerprint.hash, skelFingerprint.jointCount);

    Msg("[OzzMotionsContainer] Parsing ozz data from memory...");
    ozz::io::MemoryStream stream;
    if (!stream.Write(data.data(), data.size()))
    {
        Msg("[OzzMotionsContainer] ERROR: Failed to write to memory stream");
        loadState.store(MotionLoadState::Failed, std::memory_order_release);
        return false;
    }
    stream.Seek(0, ozz::io::Stream::kSet);

    ozz::io::IArchive archive(&stream);
    uint32_t animation_count = 0;
    archive >> animation_count;

    // Security: Validate animation count to prevent excessive allocations
    constexpr uint32_t kMaxAnimations = 1000;
    if (animation_count > kMaxAnimations)
    {
        Msg("[OzzMotionsContainer] ERROR: Animation count %u exceeds max %u", animation_count, kMaxAnimations);
        loadState.store(MotionLoadState::Failed, std::memory_order_release);
        return false;
    }

    Msg("[OzzMotionsContainer] Animation count in memory: %u", animation_count);

    const u32 joint_count = skeleton.num_joints();
    auto joint_names = skeleton.joint_names();

    Msg("[OzzMotionsContainer] Skeleton has %u joints", joint_count);

    xr_vector<MotionMetadata> all_metadata;

    if (animation_count == 0)
    {
        Msg("[OzzMotionsContainer] No aggregate animations found, trying as single animation");

        stream.Seek(0, ozz::io::Stream::kSet);
        ozz::io::IArchive single_archive(&stream);

        if (single_archive.TestTag<ozz::animation::Animation>())
        {
            Msg("[OzzMotionsContainer] Found single animation");
            std::shared_ptr<ozz::animation::Animation> animation = std::make_shared<ozz::animation::Animation>();
            single_archive >> *animation;

            if (animation->num_tracks() != skeleton.num_joints())
            {
                Msg("[OzzMotionsContainer] ERROR: Animation track mismatch: anim has %d tracks, skeleton has %d joints", animation->num_tracks(),
                    skeleton.num_joints());
                loadState.store(MotionLoadState::Failed, std::memory_order_release);
                return false;
            }

            MotionMetadata metadata = ReadMotionMetadataFromArchive(single_archive);

            MotionRecord record;
            record.name = metadata.name.empty() ? xr_string(source_label) : metadata.name;
            record.animation = animation;
            record.id.set(0, 0);
            record.frameCount = metadata.frame_count > 0 ? metadata.frame_count : 1;

            Msg("[OzzMotionsContainer] Added motion '%s' with %u frames, %u bone motions", record.name.c_str(), record.frameCount,
                static_cast<u32>(metadata.bone_motions.size()));

            record.definition.bone_or_part = metadata.bone_or_part;
            record.definition.motion = metadata.motion_id;
            record.definition.speed = record.definition.Quantize(std::max(metadata.speed, 0.f));
            record.definition.power = record.definition.Quantize(std::max(metadata.power, 0.f));
            record.definition.accrue = record.definition.Quantize(std::max(metadata.accrue, 0.f));
            record.definition.falloff = record.definition.Quantize(std::max(metadata.falloff, 0.f));
            record.definition.flags = static_cast<u16>(metadata.flags);
            if (!(record.definition.flags & esmFX) && record.definition.falloff >= record.definition.accrue && record.definition.accrue > 0)
                record.definition.falloff = static_cast<u16>(record.definition.accrue - 1);

            records.push_back(record);
            lookup[record.name] = 0;
            all_metadata.push_back(metadata);
        }
        else
        {
            Msg("[OzzMotionsContainer] ERROR: Failed to find valid animation in memory");
            loadState.store(MotionLoadState::Failed, std::memory_order_release);
            return false;
        }
    }
    else
    {
        Msg("[OzzMotionsContainer] Loading %u aggregate animations from memory", animation_count);

        for (uint32_t idx = 0; idx < animation_count; ++idx)
        {
            std::shared_ptr<ozz::animation::Animation> animation = std::make_shared<ozz::animation::Animation>();
            archive >> *animation;

            MotionMetadata metadata = ReadMotionMetadataFromArchive(archive);

            MotionRecord record;
            record.name = metadata.name.empty() ? (xr_string(source_label) + "_" + xr_string(std::to_string(idx).c_str())) : metadata.name;
            record.animation = animation;
            record.id.set(0, static_cast<u16>(idx));
            record.frameCount = metadata.frame_count > 0 ? metadata.frame_count : 1;

            Msg("[OzzMotionsContainer] Motion %u: '%s' with %u frames, %u bone motions", idx, record.name.c_str(), record.frameCount,
                static_cast<u32>(metadata.bone_motions.size()));

            record.definition.bone_or_part = metadata.bone_or_part;
            record.definition.motion = metadata.motion_id;
            record.definition.speed = record.definition.Quantize(std::max(metadata.speed, 0.f));
            record.definition.power = record.definition.Quantize(std::max(metadata.power, 0.f));
            record.definition.accrue = record.definition.Quantize(std::max(metadata.accrue, 0.f));
            record.definition.falloff = record.definition.Quantize(std::max(metadata.falloff, 0.f));
            record.definition.flags = static_cast<u16>(metadata.flags);
            if (!(record.definition.flags & esmFX) && record.definition.falloff >= record.definition.accrue && record.definition.accrue > 0)
                record.definition.falloff = static_cast<u16>(record.definition.accrue - 1);

            records.push_back(record);
            lookup[record.name] = static_cast<u16>(idx);
            all_metadata.push_back(metadata);
        }
    }

    Msg("[OzzMotionsContainer] Building bone-major layout for %u bones and %u motions", joint_count, static_cast<u32>(records.size()));

    for (u32 bone_idx = 0; bone_idx < joint_count; ++bone_idx)
    {
        shared_str bone_name(joint_names[bone_idx]);
        bone_motions[bone_name].resize(records.size());
    }

    for (u32 motion_idx = 0; motion_idx < all_metadata.size(); ++motion_idx)
    {
        const MotionMetadata& metadata = all_metadata[motion_idx];
        const u32 frame_count = records[motion_idx].frameCount;

        for (const MotionBoneData& bone_data : metadata.bone_motions)
        {
            if (bone_data.bone_id == BI_NONE || bone_data.bone_id >= joint_count)
                continue;

            const char* bone_name = joint_names[bone_data.bone_id];
            CMotion& motion = bone_motions[shared_str(bone_name)][motion_idx];

            motion.set_flags(bone_data.flags);
            motion.set_count(frame_count);

            if (!bone_data.rotation_keys.empty())
            {
                const u32 crc = ComputeOrDefaultCrc(bone_data.rotation_crc, bone_data.rotation_keys.data(), bone_data.rotation_keys.size() * sizeof(CKeyQR));
                motion._keysR.create(crc, static_cast<u32>(bone_data.rotation_keys.size()), const_cast<CKeyQR*>(bone_data.rotation_keys.data()));
            }

            switch (bone_data.translation_format)
            {
            case 1:
                if (!bone_data.translation_keys8.empty())
                {
                    const u32 crc =
                        ComputeOrDefaultCrc(bone_data.translation_crc, bone_data.translation_keys8.data(), bone_data.translation_keys8.size() * sizeof(CKeyQT8));
                    motion._keysT8.create(crc, static_cast<u32>(bone_data.translation_keys8.size()), const_cast<CKeyQT8*>(bone_data.translation_keys8.data()));
                }
                break;
            case 2:
                if (!bone_data.translation_keys16.empty())
                {
                    const u32 crc = ComputeOrDefaultCrc(bone_data.translation_crc, bone_data.translation_keys16.data(),
                        bone_data.translation_keys16.size() * sizeof(CKeyQT16));
                    motion._keysT16
                        .create(crc, static_cast<u32>(bone_data.translation_keys16.size()), const_cast<CKeyQT16*>(bone_data.translation_keys16.data()));
                }
                break;
            }

            motion._sizeT = bone_data.translation_size;
            motion._initT = bone_data.translation_init;
        }

        for (u32 bone_idx = 0; bone_idx < joint_count; ++bone_idx)
        {
            const char* bone_name = joint_names[bone_idx];
            CMotion& motion = bone_motions[shared_str(bone_name)][motion_idx];

            if (motion._keysR.size() == 0)
            {
                motion.set_flags(flRKeyAbsent);
                motion.set_count(frame_count);

                CKeyQR* identity = xr_new<CKeyQR>();
                identity->x = 0;
                identity->y = 0;
                identity->z = 0;
                identity->w = static_cast<s16>(KEY_Quant);

                const u32 crc = ComputeOrDefaultCrc(0, identity, sizeof(CKeyQR));
                motion._keysR.create(crc, 1, identity);
                xr_delete(identity);
                motion._sizeT.set(0.f, 0.f, 0.f);
                motion._initT.set(0.f, 0.f, 0.f);
            }
        }
    }

    UpdateMemoryUsage();
    MarkAccessed();
    loadState.store(MotionLoadState::Loaded, std::memory_order_release);

    Msg("[OzzMotionsContainer] Successfully loaded from memory '%s':", source_label);
    Msg("[OzzMotionsContainer]   Total motions: %u", static_cast<u32>(records.size()));
    Msg("[OzzMotionsContainer]   Bones in skeleton: %u", joint_count);
    Msg("[OzzMotionsContainer]   Memory usage: %u KB", totalMemoryBytes / 1024);

    for (const auto& record : records)
    {
        Msg("[OzzMotionsContainer]   Motion: '%s' (frames=%u)", record.name.c_str(), record.frameCount);
    }

    return true;
}

OzzMotionsValue::MotionRecord* OzzMotionsValue::FindMotion(const xr_string& name)
{
    auto it = lookup.find(name);
    if (it == lookup.end())
        return nullptr;
    return &records[it->second];
}

const OzzMotionsValue::MotionRecord* OzzMotionsValue::FindMotion(const xr_string& name) const
{
    auto it = lookup.find(name);
    if (it == lookup.end())
        return nullptr;
    return &records[it->second];
}

OzzMotionsValue::MotionRecord* OzzMotionsValue::FindMotion(u16 index)
{
    if (index >= records.size())
        return nullptr;
    return &records[index];
}

const OzzMotionsValue::MotionRecord* OzzMotionsValue::FindMotion(u16 index) const
{
    if (index >= records.size())
        return nullptr;
    return &records[index];
}

MotionVec* OzzMotionsValue::GetBoneMotions(const shared_str& bone_name)
{
    auto it = bone_motions.find(bone_name);
    if (it == bone_motions.end())
        return nullptr;
    return &it->second;
}

const MotionVec* OzzMotionsValue::GetBoneMotions(const shared_str& bone_name) const
{
    auto it = bone_motions.find(bone_name);
    if (it == bone_motions.end())
        return nullptr;
    return &it->second;
}

void OzzMotionsValue::UpdateMemoryUsage()
{
    u32 total = sizeof(OzzMotionsValue);

    for (const auto& record : records)
        total += record.GetMemoryUsage();

    for (const auto& [bone_name, motions] : bone_motions)
        total += static_cast<u32>(sizeof(CMotion) * motions.size());

    totalMemoryBytes = total;
}

void OzzMotionsValue::MarkAccessed()
{
    lastAccessTime = 0;
}

OzzMotionsContainer::OzzMotionsContainer()
{
    Msg("[OzzMotionsContainer] Initialized");
}

OzzMotionsContainer::~OzzMotionsContainer()
{
    Clean(true);
}

MotionLibraryHandle OzzMotionsContainer::Dock(shared_str key, const ozz::animation::Skeleton& skeleton)
{
    Msg("[OzzMotionsContainer] Dock called for: %s", key.c_str());

    LoadRequest request;
    request.key = key;
    request.skelFingerprint = SkeletonFingerprint::Compute(skeleton);
    request.blocking = true;

    return DockInternal(request, skeleton);
}

MotionLibraryHandle OzzMotionsContainer::Dock(const LoadRequest& request, const ozz::animation::Skeleton& skeleton)
{
    Msg("[OzzMotionsContainer] Dock(LoadRequest) called for: %s", request.key.c_str());
    return DockInternal(request, skeleton);
}

MotionLibraryHandle OzzMotionsContainer::DockInternal(const LoadRequest& request, const ozz::animation::Skeleton& skeleton)
{
    Msg("[OzzMotionsContainer] DockInternal: Processing '%s'", request.key.c_str());
    ScopeLock lock(&containerLock);

    auto keyIt = keyToHandle.find(request.key);
    if (keyIt != keyToHandle.end())
    {
        u64 handleID = keyIt->second;
        OzzMotionsValue* value = values[handleID];

        Msg("[OzzMotionsContainer] Found cached entry for '%s' (handle=0x%llX)", request.key.c_str(), handleID);

        if (config.enforceSkeletonCompatibility)
        {
            if (!value->IsCompatibleWith(request.skelFingerprint))
            {
                Msg("[OzzMotionsContainer] ERROR: Skeleton mismatch for '%s'!", request.key.c_str());
                Msg("  Expected fingerprint: hash=0x%08X, joints=%u", request.skelFingerprint.hash, request.skelFingerprint.jointCount);
                Msg("  Actual fingerprint:   hash=0x%08X, joints=%u", value->skelFingerprint.hash, value->skelFingerprint.jointCount);
                stats.cacheMisses++;
                return MotionLibraryHandle{};
            }
        }

        value->refCount.fetch_add(1, std::memory_order_relaxed);
        value->MarkAccessed();
        stats.cacheHits++;

        Msg("[OzzMotionsContainer] CACHE HIT: Reusing '%s' (refs=%u, handle=0x%llX)", request.key.c_str(), value->refCount.load(), handleID);

        return MotionLibraryHandle{ handleID };
    }

    Msg("[OzzMotionsContainer] CACHE MISS: Loading '%s' from disk", request.key.c_str());
    stats.cacheMisses++;
    stats.totalLoads++;

    OzzMotionsValue* value = xr_new<OzzMotionsValue>();
    value->refCount.store(1, std::memory_order_relaxed);

    u64 handleID = GenerateHandleID();
    value->handle = MotionLibraryHandle{ handleID };

    Msg("[OzzMotionsContainer] Generated handle 0x%llX for '%s'", handleID, request.key.c_str());

    bool loadSuccess = false;
    if (!request.embeddedData.empty())
    {
        Msg("[OzzMotionsContainer] Loading from embedded data (%zu bytes)", request.embeddedData.size());
        loadSuccess = value->LoadFromMemory(request.embeddedData, skeleton, request.key.c_str());
    }
    else
    {
        Msg("[OzzMotionsContainer] Loading from file");
        loadSuccess = value->Load(request.key.c_str(), skeleton);
    }

    if (!loadSuccess)
    {
        Msg("[OzzMotionsContainer] ERROR: Failed to load '%s'", request.key.c_str());
        xr_delete(value);
        return MotionLibraryHandle{};
    }

    values[handleID] = value;
    keyToHandle[request.key] = handleID;

    UpdateMemoryTracking(value, true);
    CheckMemoryPressure();
    globalVersion++;

    Msg("[OzzMotionsContainer] SUCCESS: Added '%s' to container (handle=0x%llX, memory=%u KB, total entries=%u)", request.key.c_str(), handleID,
        value->totalMemoryBytes / 1024, static_cast<u32>(values.size()));

    return MotionLibraryHandle{ handleID };
}

OzzMotionsValue* OzzMotionsContainer::Resolve(MotionLibraryHandle handle)
{
    if (!handle.IsValid())
        return nullptr;

    ScopeLock lock(&containerLock);

    auto it = values.find(handle.id);
    return (it != values.end()) ? it->second : nullptr;
}

const OzzMotionsValue* OzzMotionsContainer::Resolve(MotionLibraryHandle handle) const
{
    if (!handle.IsValid())
        return nullptr;

    Lock* mutable_lock = const_cast<Lock*>(&containerLock);
    ScopeLock lock(mutable_lock);

    auto it = values.find(handle.id);
    return (it != values.end()) ? it->second : nullptr;
}

bool OzzMotionsContainer::Has(shared_str key) const
{
    Lock* mutable_lock = const_cast<Lock*>(&containerLock);
    ScopeLock lock(mutable_lock);

    return keyToHandle.find(key) != keyToHandle.end();
}

void OzzMotionsContainer::Release(MotionLibraryHandle handle)
{
    if (!handle.IsValid())
        return;

    ScopeLock lock(&containerLock);

    auto it = values.find(handle.id);
    if (it == values.end())
        return;

    OzzMotionsValue* value = it->second;
    const u32 prevRef = value->refCount.fetch_sub(1, std::memory_order_release);

    if (prevRef == 1)
    {
        Msg("[OzzMotions] Last reference released for '%s' (handle=0x%llX)", value->sourceFile.c_str(), handle.id);

        if (config.enableLRUEviction && GetMemoryUsage() > config.maxMemoryBytes)
        {
            EvictMotion(handle);
        }
    }
}

void OzzMotionsContainer::Clean(bool force_destroy)
{
    ScopeLock lock(&containerLock);

    xr_vector<u64> toDelete;

    for (auto& [handleID, value] : values)
    {
        if (force_destroy || value->refCount.load() == 0)
        {
            toDelete.push_back(handleID);
        }
    }

    for (u64 handleID : toDelete)
    {
        OzzMotionsValue* value = values[handleID];

        for (auto it = keyToHandle.begin(); it != keyToHandle.end(); ++it)
        {
            if (it->second == handleID)
            {
                keyToHandle.erase(it);
                break;
            }
        }

        values.erase(handleID);

        UpdateMemoryTracking(value, false);
        xr_delete(value);
    }

    if (!toDelete.empty())
    {
        Msg("[OzzMotions] Cleaned %u motion libraries", static_cast<u32>(toDelete.size()));
    }
}

void OzzMotionsContainer::PrefetchBatch(const xr_vector<LoadRequest>& requests, const ozz::animation::Skeleton& skeleton)
{
    for (const auto& request : requests)
    {
        Dock(request, skeleton);
    }
}

xr_vector<MotionLibraryHandle> OzzMotionsContainer::LoadBatch(const xr_vector<LoadRequest>& requests, const ozz::animation::Skeleton& skeleton)
{
    xr_vector<MotionLibraryHandle> handles;
    handles.reserve(requests.size());

    for (const auto& request : requests)
    {
        handles.push_back(Dock(request, skeleton));
    }

    return handles;
}

void OzzMotionsContainer::EvictLRU(u64 target_memory_bytes)
{
    // TODO: Implement LRU eviction based on lastAccessTime
    Msg("[OzzMotions] EvictLRU not fully implemented yet");
}

bool OzzMotionsContainer::EvictMotion(MotionLibraryHandle handle)
{
    if (!handle.IsValid())
        return false;

    ScopeLock lock(&containerLock);

    auto it = values.find(handle.id);
    if (it == values.end())
        return false;

    OzzMotionsValue* value = it->second;

    if (value->refCount.load() != 0)
        return false;

    for (auto kit = keyToHandle.begin(); kit != keyToHandle.end(); ++kit)
    {
        if (kit->second == handle.id)
        {
            keyToHandle.erase(kit);
            break;
        }
    }

    values.erase(handle.id);

    UpdateMemoryTracking(value, false);
    stats.totalEvictions++;

    Msg("[OzzMotions] Evicted '%s' (handle=0x%llX)", value->sourceFile.c_str(), handle.id);
    xr_delete(value);

    return true;
}

void OzzMotionsContainer::SetMemoryLimit(u64 max_bytes)
{
    config.maxMemoryBytes = max_bytes;
    CheckMemoryPressure();
}

void OzzMotionsContainer::CheckForUpdates()
{
    // TODO: Implement hot reload
    Msg("[OzzMotions] CheckForUpdates not implemented yet");
}

bool OzzMotionsContainer::ReloadMotion(MotionLibraryHandle handle, const ozz::animation::Skeleton& skeleton)
{
    // TODO: Implement reload
    Msg("[OzzMotions] ReloadMotion not implemented yet");
    return false;
}

void OzzMotionsContainer::ResetStatistics()
{
    stats.cacheHits.store(0);
    stats.cacheMisses.store(0);
    stats.totalLoads.store(0);
    stats.totalEvictions.store(0);
    stats.peakMemoryBytes.store(0);
}

void OzzMotionsContainer::Dump()
{
    ScopeLock lock(&containerLock);

    Msg("[OzzMotions] Container state:");
    Msg("  Total entries: %u", static_cast<u32>(values.size()));
    Msg("  Memory usage: %llu KB", stats.currentMemoryBytes.load() / 1024);
    Msg("  Cache hits: %llu", stats.cacheHits.load());
    Msg("  Cache misses: %llu", stats.cacheMisses.load());
    Msg("  Hit rate: %.1f%%", stats.GetHitRate() * 100.f);

    for (const auto& [handleID, value] : values)
    {
        Msg("    [0x%llX] '%s' refs=%u memory=%u KB", handleID, value->sourceFile.c_str(), value->refCount.load(), value->totalMemoryBytes / 1024);
    }
}

void OzzMotionsContainer::UpdateMemoryTracking(OzzMotionsValue* value, bool adding)
{
    if (!value)
        return;

    const u64 bytes = value->totalMemoryBytes;

    if (adding)
    {
        const u64 newTotal = stats.currentMemoryBytes.fetch_add(bytes, std::memory_order_relaxed) + bytes;

        u64 peak = stats.peakMemoryBytes.load();
        while (newTotal > peak && !stats.peakMemoryBytes.compare_exchange_weak(peak, newTotal))
        {}
    }
    else
    {
        stats.currentMemoryBytes.fetch_sub(bytes, std::memory_order_relaxed);
    }
}

void OzzMotionsContainer::CheckMemoryPressure()
{
    const u64 current = stats.currentMemoryBytes.load();

    if (config.enableLRUEviction && current > config.maxMemoryBytes)
    {
        Msg("[OzzMotions] Memory pressure detected (%llu KB / %llu KB)", current / 1024, config.maxMemoryBytes / 1024);

        // TODO: Implement actual eviction
        EvictLRU(config.maxMemoryBytes);
    }
}

xr_vector<OzzMotionsContainer::EvictionCandidate> OzzMotionsContainer::GatherEvictionCandidates()
{
    xr_vector<EvictionCandidate> candidates;

    for (const auto& [handleID, value] : values)
    {
        if (value->refCount.load() == 0)
        {
            EvictionCandidate candidate;
            candidate.handle = value->handle;
            candidate.lastAccessTime = value->lastAccessTime;
            candidate.memoryBytes = value->totalMemoryBytes;
            candidates.push_back(candidate);
        }
    }

    std::sort(candidates.begin(), candidates.end(),
        [](const EvictionCandidate& a, const EvictionCandidate& b)
        {
            return a.lastAccessTime < b.lastAccessTime;
        });

    return candidates;
}

SharedOzzMotions::SharedOzzMotions(const SharedOzzMotions& other) : handle_(other.handle_)
{
    if (handle_.IsValid() && g_pOzzMotionsContainer)
    {
        OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(handle_);
        if (value)
        {
            value->refCount.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

SharedOzzMotions& SharedOzzMotions::operator=(const SharedOzzMotions& other)
{
    if (this != &other)
    {
        Destroy();
        handle_ = other.handle_;

        if (handle_.IsValid() && g_pOzzMotionsContainer)
        {
            OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(handle_);
            if (value)
            {
                value->refCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
    return *this;
}
} // namespace XRay::Animation
