#include "SkeletonBones.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace xray::render::vulkan
{
namespace
{
uint32_t u32(const uint8_t* bytes)
{
    return uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8) |
        (uint32_t(bytes[2]) << 16) | (uint32_t(bytes[3]) << 24);
}

bool chunk(LevelBytes ogf, uint32_t wanted, LevelBytes& result)
{
    if (!ogf.data) return false;
    size_t offset = 0;
    while (offset < ogf.size)
    {
        if (ogf.size - offset < 8) return false;
        const uint32_t id = u32(ogf.data + offset);
        const uint32_t length = u32(ogf.data + offset + 4);
        offset += 8;
        if ((id & 0x80000000u) || length > ogf.size - offset) return false;
        if (id == wanted)
        {
            result = {ogf.data + offset, length};
            return true;
        }
        offset += length;
    }
    return false;
}

bool string(LevelBytes bytes, size_t& offset, std::string& value)
{
    const size_t start = offset;
    while (offset < bytes.size && bytes.data[offset]) ++offset;
    if (offset == bytes.size || offset - start >= 256) return false;
    value.assign(reinterpret_cast<const char*>(bytes.data + start), offset - start);
    ++offset;
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
    { return static_cast<char>(std::tolower(c)); });
    return true;
}
}

bool parse_skeleton_bones(LevelBytes ogf, SkeletonBones& result, std::string& error)
{
    if (!ogf.data || ogf.size > 64 * 1024 * 1024)
    {
        error = "OGF skeleton source is missing or too large";
        return false;
    }
    for (size_t cursor = 0; cursor < ogf.size;)
    {
        if (ogf.size - cursor < 8)
        {
            error = "OGF skeleton has a truncated chunk header";
            return false;
        }
        const uint32_t length = u32(ogf.data + cursor + 4);
        if ((u32(ogf.data + cursor) & 0x80000000u) || length > ogf.size - cursor - 8)
        {
            error = "OGF skeleton has a truncated or compressed chunk";
            return false;
        }
        cursor += 8 + length;
    }
    LevelBytes names, ik, user_data;
    if (!chunk(ogf, 13, names) || names.size < 4)
    {
        error = "OGF skeleton has no valid bone-name chunk";
        return false;
    }
    const uint32_t count = u32(names.data);
    if (!count || count > 64)
    {
        error = "OGF skeleton bone count is outside 1..64";
        return false;
    }
    SkeletonBones parsed;
    parsed.bones.resize(count);
    std::vector<std::string> parents(count);
    size_t offset = 4;
    for (uint32_t i = 0; i < count; ++i)
    {
        auto& bone = parsed.bones[i];
        if (!string(names, offset, bone.name) || bone.name.empty() ||
            !string(names, offset, parents[i]) || names.size - offset < bone.obb.size())
        {
            error = "OGF skeleton has a truncated bone name or OBB";
            return false;
        }
        std::memcpy(bone.obb.data(), names.data + offset, bone.obb.size());
        offset += bone.obb.size();
        for (uint32_t previous = 0; previous < i; ++previous)
            if (parsed.bones[previous].name == bone.name)
            {
                error = "OGF skeleton contains duplicate bone names";
                return false;
            }
    }
    if (offset != names.size)
    {
        error = "OGF skeleton bone-name chunk has trailing data";
        return false;
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        if (parents[i].empty())
        {
            if (parsed.root != UINT16_MAX)
            {
                error = "OGF skeleton has more than one root";
                return false;
            }
            parsed.root = static_cast<uint16_t>(i);
            continue;
        }
        const auto parent = std::find_if(parsed.bones.begin(), parsed.bones.end(), [&](const auto& bone)
        { return bone.name == parents[i]; });
        if (parent == parsed.bones.end() || parent == parsed.bones.begin() + i)
        {
            error = "OGF skeleton has an invalid parent";
            return false;
        }
        parsed.bones[i].parent = static_cast<uint16_t>(parent - parsed.bones.begin());
    }
    if (parsed.root == UINT16_MAX)
    {
        error = "OGF skeleton has no root";
        return false;
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        uint16_t next = static_cast<uint16_t>(i);
        unsigned steps = 0;
        while (next != UINT16_MAX && ++steps <= count)
            next = parsed.bones[next].parent;
        if (steps > count)
        {
            error = "OGF skeleton has a bone-parent cycle";
            return false;
        }
    }

    if (chunk(ogf, 16, ik))
    {
        offset = 0;
        for (auto& bone : parsed.bones)
        {
            const size_t start = offset;
            if (ik.size - offset < 4)
            {
                error = "OGF skeleton IK data is truncated";
                return false;
            }
            const uint32_t version = u32(ik.data + offset);
            offset += 4;
            std::string material;
            if (version > 1 || !string(ik, offset, material))
            {
                error = "OGF skeleton IK version or material is invalid";
                return false;
            }
            // SBoneShape (0x70), SJointIKData (0x48 or 0x4c),
            // bind rotation/translation, mass and center of mass (0x28).
            const size_t remaining = 0x70 + (version ? 0x4c : 0x48) + 0x28;
            if (ik.size - offset < remaining)
            {
                error = "OGF skeleton IK record is truncated";
                return false;
            }
            offset += remaining;
            bone.ik_data.assign(ik.data + start, ik.data + offset);
        }
        if (offset != ik.size)
        {
            error = "OGF skeleton IK chunk has trailing data";
            return false;
        }
    }
    if (chunk(ogf, 17, user_data))
        parsed.user_data.assign(user_data.data, user_data.data + user_data.size);
    result = std::move(parsed);
    error.clear();
    return true;
}
}
