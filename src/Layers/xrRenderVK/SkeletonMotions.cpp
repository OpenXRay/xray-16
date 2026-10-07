#include "SkeletonMotions.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <limits>

namespace xray::render::vulkan
{
namespace
{
constexpr uint32_t params_id = 15, motions_id = 14;
constexpr size_t max_slots = 48, max_motions = 0x3ffe, max_frames = 100000;

struct Cursor
{
    LevelBytes bytes;
    size_t at{};

    bool take(size_t size, const uint8_t*& out)
    {
        if (size > bytes.size - at)
            return false;
        out = bytes.data + at;
        at += size;
        return true;
    }

    bool skip(size_t size)
    {
        const uint8_t* ignored;
        return take(size, ignored);
    }

    bool u8(uint8_t& value)
    {
        const uint8_t* p;
        if (!take(1, p))
            return false;
        value = *p;
        return true;
    }

    bool u16(uint16_t& value)
    {
        const uint8_t* p;
        if (!take(2, p))
            return false;
        value = uint16_t(p[0]) | (uint16_t(p[1]) << 8);
        return true;
    }

    bool u32(uint32_t& value)
    {
        const uint8_t* p;
        if (!take(4, p))
            return false;
        value = uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
        return true;
    }

    bool f32(float& value)
    {
        uint32_t bits;
        if (!u32(bits))
            return false;
        std::memcpy(&value, &bits, sizeof(value));
        return std::isfinite(value);
    }

    bool vec3(std::array<float, 3>& value)
    {
        return f32(value[0]) && f32(value[1]) && f32(value[2]);
    }

    bool string_z(std::string& value)
    {
        const size_t start = at;
        while (at < bytes.size && bytes.data[at])
            ++at;
        if (at == bytes.size || at - start >= 256)
            return false;
        value.assign(reinterpret_cast<const char*>(bytes.data + start), at - start);
        ++at;
        return true;
    }

    bool string_line(std::string& value)
    {
        const size_t start = at;
        while (at < bytes.size && bytes.data[at] != '\r' && bytes.data[at] != '\n')
            ++at;
        if (at == bytes.size || at - start >= 256)
            return false;
        value.assign(reinterpret_cast<const char*>(bytes.data + start), at - start);
        while (at < bytes.size && (bytes.data[at] == '\r' || bytes.data[at] == '\n'))
            ++at;
        return true;
    }

    bool done() const
    {
        return at == bytes.size;
    }
};

bool find_chunk(LevelBytes bytes, uint32_t wanted, LevelBytes& found)
{
    if (!bytes.data || bytes.size > 64 * 1024 * 1024)
        return false;
    Cursor cursor{ bytes };
    while (!cursor.done())
    {
        uint32_t id, size;
        if (!cursor.u32(id) || !cursor.u32(size) || (id & 0x80000000u) || size > bytes.size - cursor.at)
            return false;
        if (id == wanted)
        {
            found = { bytes.data + cursor.at, size };
            return true;
        }
        cursor.skip(size);
    }
    return true;
}

void lowercase(std::string& value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
}

bool read_params(LevelBytes bytes, const std::vector<std::string>& bones, MotionSlot& slot, std::vector<uint16_t>& remap)
{
    Cursor r{ bytes };
    uint16_t version, part_count;
    if (!r.u16(version) || version > 4 || !r.u16(part_count) || part_count > 4)
        return false;
    remap.assign(bones.size(), UINT16_MAX);
    for (uint16_t p = 0; p < part_count; ++p)
    {
        std::string name;
        uint16_t count;
        if (!r.string_z(name) || !r.u16(count) || count > bones.size())
            return false;
        lowercase(name);
        slot.partition_names.push_back(std::move(name));
        auto& part = slot.partitions.emplace_back();
        for (uint16_t i = 0; i < count; ++i)
        {
            std::string bone;
            uint32_t index;
            if (!r.string_z(bone) || !r.u32(index) || index >= bones.size() || remap[index] != UINT16_MAX)
                return false;
            lowercase(bone);
            auto it = std::find(bones.begin(), bones.end(), bone);
            if (it == bones.end())
                return false;
            auto id = static_cast<uint16_t>(it - bones.begin());
            if (std::find(remap.begin(), remap.end(), id) != remap.end())
                return false;
            remap[index] = id;
            part.push_back(id);
        }
    }
    if (std::find(remap.begin(), remap.end(), UINT16_MAX) != remap.end())
        return false;
    uint16_t count;
    if (!r.u16(count) || count > max_motions)
        return false;
    for (uint16_t i = 0; i < count; ++i)
    {
        MotionDefinition def;
        if (!r.string_z(def.name) || def.name.empty() || !r.u32(def.flags) || !r.u16(def.bone_or_part) || !r.u16(def.motion))
            return false;
        lowercase(def.name);
        if (def.bone_or_part != UINT16_MAX &&
            ((def.flags & 1u) ? def.bone_or_part >= bones.size() : def.bone_or_part >= slot.partitions.size()))
            return false;
        for (auto& param : def.parameters)
            if (!r.f32(param))
                return false;
        if (version >= 4)
        {
            uint32_t marks;
            if (!r.u32(marks) || marks > r.bytes.size - r.at)
                return false;
            for (uint32_t j = 0; j < marks; ++j)
            {
                MotionMark mark;
                uint32_t intervals;
                if (!r.string_line(mark.name) || !r.u32(intervals) || intervals > (r.bytes.size - r.at) / 8)
                    return false;
                for (uint32_t k = 0; k < intervals; ++k)
                {
                    std::array<float, 2> interval;
                    if (!r.f32(interval[0]) || !r.f32(interval[1]) || interval[0] > interval[1])
                        return false;
                    mark.intervals.push_back(interval);
                }
                def.marks.push_back(std::move(mark));
            }
        }
        if (std::any_of(slot.definitions.begin(), slot.definitions.end(),
                [&](const auto& prior)
                {
                    return prior.name == def.name;
                }))
            return false;
        slot.definitions.push_back(std::move(def));
    }
    return r.done();
}

bool read_motions(LevelBytes bytes, const std::vector<uint16_t>& remap, MotionSlot& slot)
{
    LevelBytes count_bytes;
    if (!find_chunk(bytes, 0, count_bytes) || count_bytes.size != 4)
        return false;
    Cursor header{ count_bytes };
    uint32_t count;
    if (!header.u32(count) || !count || count > max_motions || count != slot.definitions.size())
        return false;
    for (const auto& definition : slot.definitions)
        if (definition.motion >= count)
            return false;
    for (uint32_t m = 0; m < count; ++m)
    {
        LevelBytes data;
        if (!find_chunk(bytes, m + 1, data) || !data.data)
            return false;
        Cursor r{ data };
        MotionClip clip;
        if (!r.string_z(clip.name) || !r.u32(clip.frames) || !clip.frames || clip.frames > max_frames)
            return false;
        lowercase(clip.name);
        // Definition lookup indices follow the named motion chunks. The
        // definition's separate motion field selects its playback track.
        if (slot.definitions[m].name != clip.name) return false;
        clip.bones.resize(remap.size());
        for (uint16_t id : remap)
        {
            auto& motion = clip.bones[id];
            if (!r.u8(motion.flags) || (motion.flags & ~uint8_t(7)))
                return false;
            const bool constant_rotation = (motion.flags & 2) != 0;
            const size_t rotations = constant_rotation ? 1 : clip.frames;
            if (!constant_rotation)
            {
                uint32_t crc;
                if (!r.u32(crc))
                    return false;
            }
            if (rotations > (data.size - r.at) / 8)
                return false;
            for (size_t k = 0; k < rotations; ++k)
            {
                std::array<int16_t, 4> key;
                for (auto& component : key)
                {
                    uint16_t value;
                    if (!r.u16(value))
                        return false;
                    component = static_cast<int16_t>(value);
                }
                motion.rotations.push_back(key);
            }
            if (motion.flags & 1)
            {
                uint32_t crc;
                if (!r.u32(crc))
                    return false;
                const bool wide = (motion.flags & 4) != 0;
                const size_t stride = wide ? 6 : 3;
                if (clip.frames > (data.size - r.at) / stride)
                    return false;
                for (size_t k = 0; k < clip.frames; ++k)
                {
                    if (wide)
                    {
                        std::array<int16_t, 3> key;
                        for (auto& component : key)
                        {
                            uint16_t value;
                            if (!r.u16(value))
                                return false;
                            component = static_cast<int16_t>(value);
                        }
                        motion.translations16.push_back(key);
                    }
                    else
                    {
                        std::array<int8_t, 3> key;
                        for (auto& component : key)
                        {
                            uint8_t value;
                            if (!r.u8(value))
                                return false;
                            component = static_cast<int8_t>(value);
                        }
                        motion.translations8.push_back(key);
                    }
                }
                if (!r.vec3(motion.translation_size))
                    return false;
            }
            if (!r.vec3(motion.translation_init))
                return false;
        }
        if (!r.done())
            return false;
        slot.clips.push_back(std::move(clip));
    }
    return true;
}

bool read_slot(LevelBytes bytes, const std::vector<std::string>& bones, MotionSlot& slot, std::string& error)
{
    LevelBytes params, motions;
    if (!find_chunk(bytes, params_id, params) || !find_chunk(bytes, motions_id, motions) || !params.data || !motions.data)
    {
        error = "motion source has no valid parameters or clip chunks";
        return false;
    }
    std::vector<uint16_t> remap;
    if (!read_params(params, bones, slot, remap) || !read_motions(motions, remap, slot))
    {
        error = "motion source has invalid partitions, definitions or tracks: " + slot.source;
        return false;
    }
    return true;
}
} // namespace

bool parse_skeleton_motions(LevelBytes ogf, const std::vector<std::string>& bone_names, const std::string& model_name, const MotionResolver& resolve,
    std::vector<MotionSlot>& result, std::string& error)
{
    if (!ogf.data || bone_names.empty() || bone_names.size() > 64)
    {
        error = "motion source has no skeleton";
        return false;
    }
    LevelBytes refs, refs2;
    if (!find_chunk(ogf, 19, refs) || !find_chunk(ogf, 24, refs2) || (refs.data && refs2.data))
    {
        error = "motion references are invalid";
        return false;
    }
    std::vector<MotionSlot> parsed;
    if (refs.data || refs2.data)
    {
        std::vector<std::string> names;
        Cursor r{ refs.data ? refs : refs2 };
        if (refs.data)
        {
            std::string list;
            if (!r.string_z(list) || !r.done())
            {
                error = "legacy motion reference list is truncated";
                return false;
            }
            size_t start = 0;
            while (start < list.size())
            {
                size_t end = list.find(',', start);
                if (end == std::string::npos)
                    end = list.size();
                const auto first = list.find_first_not_of(" \t", start);
                const auto last = list.find_last_not_of(" \t", end - 1);
                if (first == std::string::npos || first >= end)
                {
                    error = "empty motion reference";
                    return false;
                }
                names.push_back(list.substr(first, last - first + 1));
                start = end + 1;
            }
        }
        else
        {
            uint32_t count;
            if (!r.u32(count) || !count || count >= max_slots)
            {
                error = "motion reference count is invalid";
                return false;
            }
            for (uint32_t i = 0; i < count; ++i)
            {
                std::string name;
                if (!r.string_z(name) || name.empty())
                {
                    error = "motion reference is truncated";
                    return false;
                }
                names.push_back(std::move(name));
            }
            if (!r.done())
            {
                error = "motion references have trailing data";
                return false;
            }
        }
        if (names.empty() || !resolve)
        {
            error = "motion references have no resolver";
            return false;
        }
        for (auto& name : names)
        {
            lowercase(name);
            if (name.size() < 4 || name.compare(name.size() - 4, 4, ".omf") != 0)
                name += ".omf";
            std::vector<MotionFile> files;
            if (!resolve(name, files, error) || files.empty())
            {
                if (error.empty())
                    error = "motion reference not found: " + name;
                return false;
            }
            for (auto& file : files)
            {
                if (parsed.size() + 1 >= max_slots)
                {
                    error = "too many motion slots";
                    return false;
                }
                MotionSlot slot;
                slot.source = file.first;
                if (!read_slot({ file.second.data(), file.second.size() }, bone_names, slot, error))
                    return false;
                // The resolver owns this buffer. Transfer it for the legacy
                // decoder instead of allocating and copying the entire OMF.
                slot.raw = std::move(file.second);
                parsed.push_back(std::move(slot));
            }
        }
    }
    else
    {
        MotionSlot slot;
        slot.source = model_name;
        if (!read_slot(ogf, bone_names, slot, error))
            return false;
        // Embedded OGF bytes belong to the caller and cannot be moved.
        slot.raw.assign(ogf.data, ogf.data + ogf.size);
        parsed.push_back(std::move(slot));
    }
    result = std::move(parsed);
    error.clear();
    return true;
}
} // namespace xray::render::vulkan
