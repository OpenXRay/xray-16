#include "SpecialVisuals.h"
#include "SlidingWindows.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace xray::render::vulkan
{
namespace
{
uint32_t integer(const uint8_t* p)
{
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
float real(const uint8_t* p)
{
    const uint32_t bits = integer(p);
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
}

bool decode_lod_facets(LevelBytes bytes, uint16_t material, LodFacets& result,
    std::string& error)
{
    constexpr size_t vertex_size = 28;
    if (!bytes.data || bytes.size != 8 * 4 * vertex_size)
    { error = "OGF LOD needs eight four-vertex facets"; return false; }
    LodFacets parsed;
    for (size_t facet = 0; facet < 8; ++facet)
    {
        auto& mesh = parsed.meshes[facet];
        mesh.material = material;
        mesh.vertices.resize(4);
        mesh.indices = {0, 1, 2, 0, 2, 3};
        for (size_t vertex = 0; vertex < 4; ++vertex)
        {
            const uint8_t* raw = bytes.data + (facet * 4 + vertex) * vertex_size;
            auto& out = mesh.vertices[vertex];
            for (size_t axis = 0; axis < 3; ++axis) out.position[axis] = real(raw + axis * 4);
            for (size_t axis = 0; axis < 2; ++axis) out.uv[axis] = real(raw + 12 + axis * 4);
            for (float value : out.position) if (!std::isfinite(value))
            { error = "OGF LOD position is not finite"; return false; }
            for (float value : out.uv) if (!std::isfinite(value))
            { error = "OGF LOD UV is not finite"; return false; }
        }
        auto& normal = parsed.normals[facet];
        for (size_t i = 0; i < 4; ++i)
        {
            const auto& a = mesh.vertices[i].position;
            const auto& b = mesh.vertices[(i + 1) % 4].position;
            const auto& c = mesh.vertices[(i + 2) % 4].position;
            const float x1 = b[0] - a[0], y1 = b[1] - a[1], z1 = b[2] - a[2];
            const float x2 = c[0] - a[0], y2 = c[1] - a[1], z2 = c[2] - a[2];
            const float nx = y1 * z2 - z1 * y2, ny = z1 * x2 - x1 * z2;
            const float nz = x1 * y2 - y1 * x2;
            const float nlen = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (!std::isfinite(nlen) || nlen <= .000001f)
            { error = "OGF LOD has a degenerate facet"; return false; }
            normal[0] -= nx / nlen;
            normal[1] -= ny / nlen;
            normal[2] -= nz / nlen;
        }
        const float length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        if (!std::isfinite(length) || length <= .000001f)
        { error = "OGF LOD has a degenerate facet"; return false; }
        for (float& component : normal) component /= length;
        for (auto& vertex : mesh.vertices)
            std::copy(normal.begin(), normal.end(), vertex.normal);
    }
    result = std::move(parsed);
    error.clear();
    return true;
}

uint8_t select_lod_facet(const std::array<std::array<float, 3>, 8>& normals,
    const std::array<float, 3>& direction)
{
    float best = -INFINITY;
    uint8_t selected = 0;
    for (uint8_t i = 0; i < 8; ++i)
    {
        const auto& n = normals[i];
        const float dot = n[0] * direction[0] + n[1] * direction[1] + n[2] * direction[2];
        if (dot > best) { best = dot; selected = i; }
    }
    return selected;
}

bool transform_tree_vertices(LevelBytes definition, LevelModel& model, std::string& error)
{
    if (!definition.data || definition.size != 104 || model.vertices.empty())
    { error = "invalid OGF tree transform or vertices"; return false; }
    float matrix[16];
    for (size_t i = 0; i < 16; ++i)
    {
        matrix[i] = real(definition.data + 4 * i);
        if (!std::isfinite(matrix[i]))
        { error = "nonfinite OGF tree transform"; return false; }
    }
    for (auto& vertex : model.vertices)
    {
        const std::array<float, 3> source{vertex.position[0], vertex.position[1], vertex.position[2]};
        const std::array<float, 3> normal{vertex.normal[0], vertex.normal[1], vertex.normal[2]};
        for (size_t axis = 0; axis < 3; ++axis)
        {
            // OGF_TREEDEF2 is the same transform passed directly to the GLES
            // tree shader. Only tree texture coordinates use the 1/2048
            // quantization factor; dividing positions collapses whole trees.
            vertex.position[axis] = matrix[axis] * source[0] + matrix[4 + axis] * source[1] +
                matrix[8 + axis] * source[2] + matrix[12 + axis];
            if (!std::isfinite(vertex.position[axis]))
            { error = "nonfinite transformed OGF tree"; return false; }
            vertex.normal[axis] = matrix[axis] * normal[0] + matrix[4 + axis] * normal[1] +
                matrix[8 + axis] * normal[2];
        }
    }
    error.clear();
    return true;
}

bool decode_tree_windows(LevelBytes table, uint32_t item,
    const LevelModel& model, std::vector<SlideWindow>& result, std::string& error)
{
    if (!table.data || table.size < 4)
    { error = "level.geom has no tree sliding windows"; return false; }
    const uint32_t count = integer(table.data);
    if (count > 65536 || item >= count)
    { error = "tree sliding window index is out of range"; return false; }
    size_t offset = 4;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (offset > table.size || table.size - offset < 20)
        { error = "truncated tree sliding window table"; return false; }
        const uint32_t windows = integer(table.data + offset + 16);
        if (!windows || windows > 65536 || size_t(windows) * 8 > table.size - offset - 20)
        { error = "invalid tree sliding window count"; return false; }
        const size_t size = 20 + size_t(windows) * 8;
        if (i == item)
        {
            if (!decode_slide_windows(table.data + offset, size, model.indices,
                    model.vertices.size(), result, error)) return false;
        }
        offset += size;
    }
    if (offset != table.size)
    { error = "trailing tree sliding window bytes"; return false; }
    error.clear();
    return true;
}
} // namespace xray::render::vulkan
