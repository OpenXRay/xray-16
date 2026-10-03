#include "DetailAssets.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace xray::render::vulkan
{
namespace
{
uint32_t word(const uint8_t* data)
{
    return uint32_t(data[0]) | uint32_t(data[1]) << 8 |
        uint32_t(data[2]) << 16 | uint32_t(data[3]) << 24;
}
float scalar(const uint8_t* data)
{
    const uint32_t bits = word(data);
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
bool name(LevelBytes bytes, size_t& offset, std::string& value)
{
    const size_t begin = offset;
    while (offset < bytes.size && bytes.data[offset]) ++offset;
    if (offset == bytes.size || offset - begin > 255) return false;
    value.assign(reinterpret_cast<const char*>(bytes.data + begin), offset - begin);
    ++offset;
    return !value.empty();
}
bool prototype(LevelBytes source, DetailPrototype& output)
{
    if (!source.data || source.size < 2) return false;
    size_t offset = 0;
    DetailPrototype item;
    if (!name(source, offset, item.shader) || !name(source, offset, item.texture) ||
        offset > source.size || source.size - offset < 20) return false;
    item.min_scale = scalar(source.data + offset + 4);
    item.max_scale = scalar(source.data + offset + 8);
    const uint32_t vertex_count = word(source.data + offset + 12);
    const uint32_t index_count = word(source.data + offset + 16);
    offset += 20;
    if (!std::isfinite(item.min_scale) || !std::isfinite(item.max_scale) ||
        item.min_scale < 0 || item.max_scale <= 0 || item.max_scale < item.min_scale ||
        !vertex_count || vertex_count > 65535 || !index_count || index_count % 3 ||
        index_count > 3'000'000 || source.size - offset != size_t(vertex_count) * 20 + size_t(index_count) * 2)
        return false;
    item.vertices.resize(vertex_count);
    for (size_t i = 0; i < vertex_count; ++i)
    {
        auto& vertex = item.vertices[i];
        const uint8_t* p = source.data + offset + 20 * i;
        for (int axis = 0; axis < 3; ++axis)
            vertex.position[axis] = scalar(p + 4 * axis);
        vertex.uv[0] = scalar(p + 12);
        vertex.uv[1] = scalar(p + 16);
        vertex.normal[1] = 1.f;
        for (float coord : vertex.position) if (!std::isfinite(coord)) return false;
        for (float coord : vertex.uv) if (!std::isfinite(coord)) return false;
    }
    offset += size_t(vertex_count) * 20;
    item.indices.reserve(index_count);
    for (size_t i = 0; i < index_count; ++i)
    {
        const uint32_t index = uint32_t(source.data[offset + 2 * i]) |
            uint32_t(source.data[offset + 2 * i + 1]) << 8;
        if (index >= vertex_count) return false;
        item.indices.push_back(index);
    }
    output = std::move(item);
    return true;
}
}

bool decode_detail_assets(LevelBytes header, const std::vector<LevelBytes>& objects,
    LevelBytes slots, DetailAssets& result, std::string& error)
{
    DetailAssets loaded;
    if (!header.data || header.size != 24 || !slots.data || word(header.data) != 3 ||
        word(header.data + 4) != objects.size() || objects.size() > 64)
        goto invalid;
    loaded.offset_x = static_cast<int32_t>(word(header.data + 8));
    loaded.offset_z = static_cast<int32_t>(word(header.data + 12));
    loaded.width = word(header.data + 16);
    loaded.height = word(header.data + 20);
    if (loaded.offset_x < -1'000'000 || loaded.offset_x > 1'000'000 ||
        loaded.offset_z < -1'000'000 || loaded.offset_z > 1'000'000 ||
        !loaded.width || !loaded.height || uint64_t(loaded.width) * loaded.height > 2'000'000 ||
        slots.size != uint64_t(loaded.width) * loaded.height * 16) goto invalid;
    loaded.prototypes.resize(objects.size());
    for (size_t i = 0; i < objects.size(); ++i)
        if (!prototype(objects[i], loaded.prototypes[i])) goto invalid;
    loaded.cells.reserve(size_t(loaded.width) * loaded.height);
    for (size_t i = 0; i < size_t(loaded.width) * loaded.height; ++i)
    {
        const uint8_t* p = slots.data + 16 * i;
        const uint64_t bits = uint64_t(word(p)) | uint64_t(word(p + 4)) << 32;
        DetailCell cell;
        cell.ground = float(bits & 0xfffu) * .2f - 200.f;
        cell.height = float((bits >> 12) & 0xffu) * .1f;
        for (unsigned n = 0; n < 4; ++n)
        {
            cell.ids[n] = static_cast<uint8_t>((bits >> (20 + n * 6)) & 63);
            const uint16_t palette = uint16_t(p[8 + n * 2]) | uint16_t(p[9 + n * 2]) << 8;
            for (unsigned corner = 0; corner < 4; ++corner)
                cell.palette[n][corner] = uint8_t((palette >> (corner * 4)) & 15);
            if (cell.ids[n] != 63 && cell.ids[n] >= objects.size()) goto invalid;
        }
        loaded.cells.push_back(cell);
    }
    result = std::move(loaded);
    error.clear();
    return true;
invalid:
    error = "invalid level.details header, prototype or slot database";
    return false;
}
}
