#pragma once

#include "LegacyOmfConverterFull.h"

#include <ozz/base/io/archive.h>

#include <cstdint>
#include <string>

namespace XRay::Animation::Tools {

inline void SerializeString(ozz::io::OArchive& archive, const std::string& value)
{
    const std::uint32_t length = static_cast<std::uint32_t>(value.size());
    archive << length;
    if (length > 0)
        archive << ozz::io::MakeArray(value.c_str(), length);
}

inline void SerializeMotionMarks(ozz::io::OArchive& archive, const LegacyMotionMetadata& metadata)
{
    const std::uint32_t mark_count = static_cast<std::uint32_t>(metadata.marks.size());
    archive << mark_count;
    for (const auto& mark : metadata.marks)
    {
        SerializeString(archive, std::string(mark.name.c_str()));
        const std::uint32_t interval_count = static_cast<std::uint32_t>(mark.intervals.size());
        archive << interval_count;
        for (const auto& interval : mark.intervals)
        {
            archive << interval.first;
            archive << interval.second;
        }
    }
}

inline void SerializeMotionMetadata(ozz::io::OArchive& archive, const LegacyMotionMetadata& metadata)
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

}
