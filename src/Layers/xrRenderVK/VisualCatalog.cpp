#include "VisualCatalog.h"

#include <cstring>
#include <cmath>
#include <functional>

namespace xray::render::vulkan
{
namespace
{
constexpr uint32_t header_id = 1, texture_id = 2, children_id = 9, linked_id = 10;
constexpr unsigned max_depth = 32;
constexpr size_t max_visuals = 1'000'000;

uint32_t read_u32(const uint8_t* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

bool each_chunk(LevelBytes bytes, const std::function<bool(uint32_t, LevelBytes)>& callback)
{
    if (bytes.size && !bytes.data) return false;
    size_t offset = 0;
    while (offset < bytes.size)
    {
        if (bytes.size - offset < 8) return false;
        const uint32_t id = read_u32(bytes.data + offset);
        const uint32_t length = read_u32(bytes.data + offset + 4);
        offset += 8;
        // IReader::open_chunk handles engine compression. This parser only
        // accepts the uncompressed payload it receives from that reader.
        if ((id & 0x80000000u) || length > bytes.size - offset ||
            !callback(id, {bytes.data + offset, length})) return false;
        offset += length;
    }
    return true;
}

bool read_string(LevelBytes bytes, size_t& offset, std::string& output)
{
    const size_t begin = offset;
    while (offset < bytes.size && bytes.data[offset]) ++offset;
    if (offset == bytes.size) return false;
    output.assign(reinterpret_cast<const char*>(bytes.data + begin), offset - begin);
    ++offset;
    return true;
}

bool parse_visual(LevelBytes bytes, VisualRecord& result, unsigned depth)
{
    if (depth > max_depth || bytes.size > 64 * 1024 * 1024 || !bytes.data) return false;
    VisualRecord visual;
    bool header = false, children = false, links = false, texture = false;
    if (!each_chunk(bytes, [&](uint32_t id, LevelBytes body)
        {
            if (id == header_id)
            {
                if (header || body.size != 44 || body.data[0] != 4 || body.data[1] > 12)
                    return false;
                header = true;
                visual.type = body.data[1];
                visual.shader_id = uint16_t(body.data[2]) | (uint16_t(body.data[3]) << 8);
                std::memcpy(visual.bounds.data(), body.data + 4, sizeof(float) * 10);
                for (float value : visual.bounds)
                    if (!std::isfinite(value))
                        return false;
                if (visual.bounds[9] < 0)
                    return false;
            }
            else if (id == texture_id)
            {
                if (texture) return false;
                texture = true;
                size_t position = 0;
                if (!read_string(body, position, visual.texture) ||
                    !read_string(body, position, visual.shader) || position != body.size) return false;
            }
            else if (id == linked_id)
            {
                if (links || body.size < 4) return false;
                links = true;
                const uint32_t count = read_u32(body.data);
                if (count > max_visuals || size_t(count) * 4 != body.size - 4) return false;
                visual.linked_children.reserve(count);
                for (uint32_t i = 0; i < count; ++i)
                    visual.linked_children.push_back(read_u32(body.data + 4 + size_t(i) * 4));
            }
            else if (id == children_id)
            {
                if (children) return false;
                children = true;
                uint32_t expected = 0;
                return each_chunk(body, [&](uint32_t child_id, LevelBytes child)
                    {
                        if (child_id != expected++ || expected > max_visuals) return false;
                        VisualRecord record;
                        if (!parse_visual(child, record, depth + 1)) return false;
                        visual.embedded_children.push_back(std::move(record));
                        return true;
                    });
            }
            return true; // Keep all model-specific chunks verbatim in source.
        }) || !header || (links && children)) return false;
    visual.source.assign(bytes.data, bytes.data + bytes.size);
    result = std::move(visual);
    return true;
}
}

bool parse_ogf_visual(LevelBytes bytes, VisualRecord& result, std::string& error)
{
    if (!parse_visual(bytes, result, 0))
    {
        uint32_t type = UINT32_MAX, shader = UINT32_MAX;
        each_chunk(bytes, [&](uint32_t id, LevelBytes body) {
            if (id == header_id && body.size >= 4)
            {
                type = body.data[1];
                shader = uint16_t(body.data[2]) | (uint16_t(body.data[3]) << 8);
            }
            return true;
        });
        error = "invalid OGF visual record type=" + std::to_string(type) + " shader_id=" + std::to_string(shader);
        return false;
    }
    error.clear();
    return true;
}

bool parse_level_visuals(LevelBytes bytes, std::vector<VisualRecord>& result, std::string& error)
{
    std::vector<VisualRecord> records;
    uint32_t expected = 0;
    if (!each_chunk(bytes, [&](uint32_t id, LevelBytes body) {
            if (id != expected++ || expected > max_visuals)
                return false;
            VisualRecord record;
            if (!parse_visual(body, record, 0))
                return false;
            records.push_back(std::move(record));
            return true;
        }))
    {
        error = "invalid level visual table id=" + std::to_string(expected ? expected - 1 : 0);
        return false;
    }
    std::vector<uint8_t> visiting(records.size());
    std::function<bool(size_t)> validate = [&](size_t index)
    {
        if (visiting[index] == 1) return false;
        if (visiting[index] == 2) return true;
        visiting[index] = 1;
        for (uint32_t child : records[index].linked_children)
            if (child >= records.size() || !validate(child)) return false;
        visiting[index] = 2;
        return true;
    };
    for (size_t i = 0; i < records.size(); ++i)
        if (!validate(i))
        {
            error = "invalid or cyclic OGF child references";
            return false;
        }
    result = std::move(records);
    error.clear();
    return true;
}
}
