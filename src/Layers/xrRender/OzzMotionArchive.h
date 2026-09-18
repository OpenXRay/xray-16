#pragma once

#include "xrCore/xrCore.h"

#include <ozz/base/io/archive.h>

#include <cstdint>
#include <string>

namespace XRay::Animation {

inline std::string ReadOzzString(ozz::io::IArchive& archive)
{
    std::uint32_t length = 0;
    archive >> length;

    constexpr std::uint32_t kMaxStringLength = 256;
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

inline void SkipOzzAnimationMetadata(ozz::io::IArchive& archive)
{
    ReadOzzString(archive);

    std::uint32_t flags        = 0;
    std::uint16_t bone_or_part = 0;
    std::uint16_t motion_id    = 0;
    float speed   = 0.f;
    float power   = 0.f;
    float accrue  = 0.f;
    float falloff = 0.f;

    archive >> flags;
    archive >> bone_or_part;
    archive >> motion_id;
    archive >> speed;
    archive >> power;
    archive >> accrue;
    archive >> falloff;

    std::uint32_t mark_count = 0;
    archive >> mark_count;
    for (std::uint32_t mark_index = 0; mark_index < mark_count; ++mark_index)
    {
        ReadOzzString(archive);

        std::uint32_t interval_count = 0;
        archive >> interval_count;
        for (std::uint32_t interval_index = 0; interval_index < interval_count; ++interval_index)
        {
            float first  = 0.f;
            float second = 0.f;
            archive >> first;
            archive >> second;
        }
    }
}

inline u32 ComputeOrDefaultCrc(u32 provided_crc, const void* data, std::size_t byte_count)
{
    if (provided_crc != 0 || byte_count == 0 || data == nullptr)
        return provided_crc;
    return crc32(data, static_cast<u32>(byte_count));
}

}
