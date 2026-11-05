#include "SpecialVisuals.h"
#include "SlidingWindows.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace xray::render::vulkan
{
namespace
{
uint32_t integer(const uint8_t *p)
{
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
float real(const uint8_t *p)
{
    const uint32_t bits = integer(p);
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
} // namespace

bool decode_lod_facets(LevelBytes bytes, uint16_t material, LodFacets &result, std::string &error, bool paired)
{
    constexpr size_t vertex_size = 28;
    if (!bytes.data || bytes.size != 8 * 4 * vertex_size)
    {
        error = "OGF LOD needs eight four-vertex facets";
        return false;
    }
    LodFacets parsed;
    for (size_t facet = 0; facet < 8; ++facet)
    {
        auto &mesh = parsed.meshes[facet];
        mesh.material = material;
        mesh.vertices.resize(4);
        mesh.indices = {0, 1, 2, 0, 2, 3};
        for (size_t vertex = 0; vertex < 4; ++vertex)
        {
            const uint8_t *raw = bytes.data + (facet * 4 + vertex) * vertex_size;
            auto &out = mesh.vertices[vertex];
            for (size_t axis = 0; axis < 3; ++axis)
                out.position[axis] = real(raw + axis * 4);
            for (size_t axis = 0; axis < 2; ++axis)
                out.uv[axis] = real(raw + 12 + axis * 4);
            for (float value : out.position)
                if (!std::isfinite(value))
                {
                    error = "OGF LOD position is not finite";
                    return false;
                }
            for (float value : out.uv)
                if (!std::isfinite(value))
                {
                    error = "OGF LOD UV is not finite";
                    return false;
                }
        }
        auto &normal = parsed.normals[facet];
        for (size_t i = 0; i < 4; ++i)
        {
            const auto &a = mesh.vertices[i].position;
            const auto &b = mesh.vertices[(i + 1) % 4].position;
            const auto &c = mesh.vertices[(i + 2) % 4].position;
            const float x1 = b[0] - a[0], y1 = b[1] - a[1], z1 = b[2] - a[2];
            const float x2 = c[0] - a[0], y2 = c[1] - a[1], z2 = c[2] - a[2];
            const float nx = y1 * z2 - z1 * y2, ny = z1 * x2 - x1 * z2;
            const float nz = x1 * y2 - y1 * x2;
            const float nlen = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (!std::isfinite(nlen) || nlen <= .000001f)
            {
                error = "OGF LOD has a degenerate facet";
                return false;
            }
            normal[0] -= nx / nlen;
            normal[1] -= ny / nlen;
            normal[2] -= nz / nlen;
        }
        const float length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        if (!std::isfinite(length) || length <= .000001f)
        {
            error = "OGF LOD has a degenerate facet";
            return false;
        }
        for (float &component : normal)
            component /= length;
        for (auto &vertex : mesh.vertices)
            std::copy(normal.begin(), normal.end(), vertex.normal);
    }
    // Keep both baked views in one vertex stream. The vertex shader morphs
    // the four corners and the fragment shader blends the two atlas samples.
    if (paired)
    {
        const auto source = parsed.meshes;
        for (size_t a = 0; a < 8; ++a)
        {
            auto &mesh = parsed.meshes[a];
            mesh.lod_pairs = true;
            mesh.vertices.clear(); mesh.indices.clear();
            for (size_t b = a; b < 8; ++b)
            {
                for (unsigned v = 0; v < 4; ++v)
                {
                    auto vertex = source[a].vertices[v];
                    std::copy_n(source[b].vertices[v].position, 3, vertex.normal);
                    std::copy_n(source[b].vertices[v].uv, 2, vertex.lightmap_uv);
                    for (unsigned axis = 0; axis < 3; ++axis)
                    {
                        vertex.tangent[axis] = int8_t(std::round(parsed.normals[a][axis] * 127.f));
                        vertex.bitangent[axis] = int8_t(std::round(parsed.normals[b][axis] * 127.f));
                    }
                    const auto *ra = bytes.data + (a * 4 + v) * vertex_size;
                    const auto *rb = bytes.data + (b * 4 + v) * vertex_size;
                    vertex.baked[0] = ra[23] / 255.f;
                    vertex.baked[1] = rb[23] / 255.f;
                    mesh.vertices.push_back(vertex);
                }
                for (uint32_t v : {3u, 0u, 2u, 1u, 2u, 0u})
                    mesh.indices.push_back(uint32_t((b-a) * 4) + v);
            }
        }
    }
    result = std::move(parsed);
    error.clear();
    return true;
}

LodSelection select_lod_views(const std::array<std::array<float,3>,8>& normals,
    const std::array<float,3>& direction)
{
    std::array<std::pair<float,uint8_t>,8> ranked;
    for (uint8_t i=0;i<8;++i)
        ranked[i] = {normals[i][0]*direction[0]+normals[i][1]*direction[1]+normals[i][2]*direction[2],i};
    std::sort(ranked.begin(),ranked.end(),[](const auto& a,const auto& b){return a.first>b.first;});
    const float span=ranked[0].first-ranked[2].first;
    const float weight=span>1e-8f ? 1.f-.5f*(ranked[1].first-ranked[2].first)/span : .5f;
    return {ranked[0].second,ranked[1].second,std::clamp(weight,.5f,1.f)};
}

uint8_t select_lod_facet(const std::array<std::array<float, 3>, 8> &normals, const std::array<float, 3> &direction)
{
    float best = -INFINITY;
    uint8_t selected = 0;
    for (uint8_t i = 0; i < 8; ++i)
    {
        const auto &n = normals[i];
        const float dot = n[0] * direction[0] + n[1] * direction[1] + n[2] * direction[2];
        if (dot > best)
        {
            best = dot;
            selected = i;
        }
    }
    return selected;
}

bool transform_tree_vertices(LevelBytes definition, LevelModel &model, std::string &error)
{
    if (!definition.data || definition.size != 104 || model.vertices.empty())
    {
        error = "invalid OGF tree transform or vertices";
        return false;
    }
    float matrix[16];
    for (size_t i = 0; i < 16; ++i)
    {
        matrix[i] = real(definition.data + 4 * i);
        if (!std::isfinite(matrix[i]))
        {
            error = "nonfinite OGF tree transform";
            return false;
        }
    }
    // Desktop halves the serialized scale/bias, then multiplies by
    // Tree_SBC (default 1.5) and the deferred factor 1.3333.
    const float hemi_scale = real(definition.data + 76) * .999975f;
    const float hemi_bias = real(definition.data + 96) * .999975f;
    if (!std::isfinite(hemi_scale) || !std::isfinite(hemi_bias))
    {
        error = "nonfinite OGF tree lighting";
        return false;
    }
    for (auto &vertex : model.vertices)
    {
        const float authored = vertex.baked[3];
        vertex.baked[3] = std::clamp(authored * hemi_scale + hemi_bias, 0.f, 1.f);
        const std::array<float, 3> source{vertex.position[0], vertex.position[1], vertex.position[2]};
        const std::array<float, 3> normal{vertex.normal[0], vertex.normal[1], vertex.normal[2]};
        const std::array<float, 3> tangent{float(vertex.tangent[0]) / 127.f, float(vertex.tangent[1]) / 127.f,
                                           float(vertex.tangent[2]) / 127.f};
        const std::array<float, 3> bitangent{float(vertex.bitangent[0]) / 127.f, float(vertex.bitangent[1]) / 127.f,
                                             float(vertex.bitangent[2]) / 127.f};
        for (size_t axis = 0; axis < 3; ++axis)
        {
            // OGF_TREEDEF2 is the same transform passed directly to the GLES
            // tree shader. Only tree texture coordinates use the 1/2048
            // quantization factor; dividing positions collapses whole trees.
            vertex.position[axis] = matrix[axis] * source[0] + matrix[4 + axis] * source[1] +
                                    matrix[8 + axis] * source[2] + matrix[12 + axis];
            if (!std::isfinite(vertex.position[axis]))
            {
                error = "nonfinite transformed OGF tree";
                return false;
            }
            vertex.normal[axis] =
                matrix[axis] * normal[0] + matrix[4 + axis] * normal[1] + matrix[8 + axis] * normal[2];
            vertex.tangent[axis] = int8_t(std::round(
                std::clamp(matrix[axis] * tangent[0] + matrix[4 + axis] * tangent[1] + matrix[8 + axis] * tangent[2],
                           -1.f, 1.f) *
                127.f));
            vertex.bitangent[axis] =
                int8_t(std::round(std::clamp(matrix[axis] * bitangent[0] + matrix[4 + axis] * bitangent[1] +
                                                 matrix[8 + axis] * bitangent[2],
                                             -1.f, 1.f) *
                                  127.f));
        }
    }
    error.clear();
    return true;
}

bool decode_tree_windows(LevelBytes table, uint32_t item, const LevelModel &model, std::vector<SlideWindow> &result,
                         std::string &error)
{
    if (!table.data || table.size < 4)
    {
        error = "level.geom has no tree sliding windows";
        return false;
    }
    const uint32_t count = integer(table.data);
    if (count > 65536 || item >= count)
    {
        error = "tree sliding window index is out of range";
        return false;
    }
    size_t offset = 4;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (offset > table.size || table.size - offset < 20)
        {
            error = "truncated tree sliding window table";
            return false;
        }
        const uint32_t windows = integer(table.data + offset + 16);
        if (!windows || windows > 65536 || size_t(windows) * 8 > table.size - offset - 20)
        {
            error = "invalid tree sliding window count";
            return false;
        }
        const size_t size = 20 + size_t(windows) * 8;
        if (i == item)
        {
            if (!decode_slide_windows(table.data + offset, size, model.indices, model.vertices.size(), result, error))
                return false;
        }
        offset += size;
    }
    if (offset != table.size)
    {
        error = "trailing tree sliding window bytes";
        return false;
    }
    error.clear();
    return true;
}
} // namespace xray::render::vulkan
