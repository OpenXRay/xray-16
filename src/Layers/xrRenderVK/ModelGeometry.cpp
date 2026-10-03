#include "ModelGeometry.h"

#include <cmath>
#include <cstring>

namespace xray::render::vulkan
{
namespace
{
constexpr uint32_t vertices_id = 3, indices_id = 4, sliding_id = 6;
constexpr size_t max_vertices = 4'000'000, max_indices = 12'000'000;

uint16_t u16(const uint8_t* p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
uint32_t u32(const uint8_t* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
        (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
float f32(const uint8_t* p)
{
    const uint32_t bits = u32(p);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

bool find_chunk(const std::vector<uint8_t>& source, uint32_t wanted, LevelBytes& chunk)
{
    size_t offset = 0;
    while (offset < source.size())
    {
        if (source.size() - offset < 8) return false;
        const uint32_t id = u32(source.data() + offset);
        const uint32_t size = u32(source.data() + offset + 4);
        offset += 8;
        if (size > source.size() - offset || id & 0x80000000u) return false;
        if (id == wanted)
        {
            chunk = {source.data() + offset, size};
            return true;
        }
        offset += size;
    }
    return false;
}

bool vertex(const uint8_t* p, unsigned links, ModelVertex& out)
{
    // Serialized layouts from xrCore/Animation/Bone.hpp, packed to 2 bytes.
    const size_t base = links == 1 ? 0 : links * 2;
    for (unsigned i = 0; i < 3; ++i)
    {
        out.position[i] = f32(p + base + i * 4);
        out.normal[i] = f32(p + base + 12 + i * 4);
        if (!std::isfinite(out.position[i]) || !std::isfinite(out.normal[i])) return false;
    }
    if (links == 1)
    {
        out.uv[0] = f32(p + 48); out.uv[1] = f32(p + 52);
        const uint32_t bone = u32(p + 56);
        if (bone > UINT16_MAX) return false;
        out.bones[0] = static_cast<uint16_t>(bone);
    }
    else
    {
        for (unsigned i = 0; i < links; ++i) out.bones[i] = u16(p + i * 2);
        const size_t weight_offset = base + 48;
        if (links == 2)
        {
            const float second = f32(p + weight_offset);
            out.weights[0] = 1 - second; out.weights[1] = second;
        }
        else
        {
            float remainder = 1;
            for (unsigned i = 0; i + 1 < links; ++i)
            {
                out.weights[i] = f32(p + weight_offset + i * 4);
                remainder -= out.weights[i];
            }
            out.weights[links - 1] = remainder;
        }
        const size_t uv_offset = weight_offset + (links - 1) * 4;
        out.uv[0] = f32(p + uv_offset); out.uv[1] = f32(p + uv_offset + 4);
    }
    if (!std::isfinite(out.uv[0]) || !std::isfinite(out.uv[1])) return false;
    for (unsigned i = 0; i < links; ++i)
        if (!std::isfinite(out.weights[i]) || out.weights[i] < -0.0001f ||
            out.weights[i] > 1.0001f) return false;
    return true;
}

bool mesh(const VisualRecord& source, ModelGeometry& out, std::string& error)
{
    LevelBytes vertices, indices;
    if (!find_chunk(source.source, vertices_id, vertices) ||
        !find_chunk(source.source, indices_id, indices) ||
        vertices.size < 8 || indices.size < 4)
    {
        error = "OGF model mesh has no embedded vertices or indices";
        return false;
    }
    const uint32_t format = u32(vertices.data), count = u32(vertices.data + 4);
    const uint32_t index_count = u32(indices.data);
    if (!count || count > max_vertices || !index_count || index_count > max_indices ||
        index_count % 3 || size_t(index_count) * 2 != indices.size - 4)
    {
        error = "OGF mesh dimensions or triangle indices are invalid";
        return false;
    }
    unsigned links = 0;
    if (format == 1 || format == 0x12071980u) links = 1;
    else if (format == 2 || format == 0x240e3300u) links = 2;
    else if (format == 3 || format == 0x481c6600u) links = 3;
    else if (format == 4 || format == 0x5a238f80u) links = 4;
    const bool plain = source.type == 0 || source.type == 2;
    if (plain ? format != 0x112u : !links)
    {
        error = "unsupported OGF mesh vertex encoding";
        return false;
    }
    out.skin_weights = static_cast<uint8_t>(links);
    const size_t stride = plain ? 32 : links == 1 ? 60 : links == 2 ? 64 : links == 3 ? 70 : 76;
    if (size_t(count) * stride != vertices.size - 8)
    {
        error = "OGF skinned vertex payload has an invalid stride";
        return false;
    }
    out.vertices.resize(count);
    for (size_t i = 0; i < count; ++i)
        if (plain)
        {
            const uint8_t* p = vertices.data + 8 + i * stride;
            auto& v = out.vertices[i];
            for (unsigned j = 0; j < 3; ++j)
            {
                v.position[j] = f32(p + j * 4);
                v.normal[j] = f32(p + 12 + j * 4);
            }
            v.uv[0] = f32(p + 24);
            v.uv[1] = f32(p + 28);
            for (float component : v.position)
                if (!std::isfinite(component))
                {
                    error = "OGF static vertex is not finite";
                    return false;
                }
            for (float component : v.normal)
                if (!std::isfinite(component))
                {
                    error = "OGF static normal is not finite";
                    return false;
                }
            for (float component : v.uv)
                if (!std::isfinite(component))
                {
                    error = "OGF static UV is not finite";
                    return false;
                }
        }
        else if (!vertex(vertices.data + 8 + i * stride, links, out.vertices[i]))
        {
            error = "OGF skinned vertex contains invalid weights or coordinates";
            return false;
        }
    out.indices.reserve(index_count);
    for (size_t i = 0; i < index_count; ++i)
    {
        const uint16_t index = u16(indices.data + 4 + i * 2);
        if (index >= count)
        {
            error = "OGF skinned index exceeds vertex count";
            return false;
        }
        out.indices.push_back(index);
    }
    if (source.type == 2 || source.type == 4)
    {
        LevelBytes windows;
        if (!find_chunk(source.source, sliding_id, windows))
        { error = "progressive OGF mesh has no sliding window"; return false; }
        if (!decode_slide_windows(windows.data, windows.size, out.indices,
                out.vertices.size(), out.windows, error)) return false;
    }
    return true;
}

bool decode(const VisualRecord& source, ModelGeometry& out, std::string& error, unsigned depth)
{
    if (depth > 32) { error = "OGF model hierarchy is too deep"; return false; }
    out.type = source.type;
    out.shader = source.shader;
    out.texture = source.texture;
    out.mode = classify_surface_material(out.shader, out.texture);
    if (source.type == 0 || source.type == 2 || source.type == 4 || source.type == 5)
    {
        if (!mesh(source, out, error)) return false;
    }
    else if (source.type != 1 && source.type != 3 && source.type != 10)
    {
        error = "OGF model type has no Vulkan geometry decoder";
        return false;
    }
    if (!source.linked_children.empty())
    {
        error = "standalone OGF model refers to a level visual table";
        return false;
    }
    out.children.reserve(source.embedded_children.size());
    for (const auto &child : source.embedded_children)
    {
        out.children.emplace_back();
        if (!decode(child, out.children.back(), error, depth + 1))
        {
            error = "child id=" + std::to_string(out.children.size() - 1) + " type=" + std::to_string(child.type) + " / " + error;
            return false;
        }
    }
    if ((source.type == 3 || source.type == 10) && out.children.empty())
    {
        error = "OGF skeleton has no child geometry";
        return false;
    }
    return true;
}
}

bool decode_model_geometry(const VisualRecord &source, ModelGeometry &result, std::string &error)
{
    ModelGeometry prepared;
    if (!decode(source, prepared, error, 0))
    {
        error = "OGF type=" + std::to_string(source.type) + " shader_id=" + std::to_string(source.shader_id) + " decode: " + error;
        return false;
    }
    result = std::move(prepared);
    error.clear();
    return true;
}

bool skin_model_mesh(const ModelGeometry& source, const float* matrices,
    size_t bone_count, std::vector<LevelVertex>& result, std::string& error)
{
    if ((source.type != 4 && source.type != 5) || source.vertices.empty() ||
        !matrices || !bone_count || bone_count > 256)
    {
        error = "skinned model needs vertices and a valid bone pose";
        return false;
    }
    std::vector<LevelVertex> prepared(source.vertices.size());
    for (size_t i = 0; i < source.vertices.size(); ++i)
    {
        const ModelVertex& from = source.vertices[i];
        LevelVertex& to = prepared[i];
        to.uv[0] = from.uv[0]; to.uv[1] = from.uv[1];
        for (unsigned influence = 0; influence < 4; ++influence)
        {
            const float weight = from.weights[influence];
            if (weight == 0) continue;
            if (!std::isfinite(weight) || weight < 0 || from.bones[influence] >= bone_count)
            {
                error = "skinned model references an invalid bone or weight";
                return false;
            }
            const float* m = matrices + size_t(from.bones[influence]) * 16;
            for (unsigned axis = 0; axis < 3; ++axis)
            {
                const float p = m[axis] * from.position[0] +
                    m[4 + axis] * from.position[1] + m[8 + axis] * from.position[2] + m[12 + axis];
                const float n = m[axis] * from.normal[0] +
                    m[4 + axis] * from.normal[1] + m[8 + axis] * from.normal[2];
                if (!std::isfinite(p) || !std::isfinite(n))
                {
                    error = "skinned model pose contains nonfinite coordinates";
                    return false;
                }
                to.position[axis] += p * weight;
                to.normal[axis] += n * weight;
            }
        }
    }
    result = std::move(prepared);
    error.clear();
    return true;
}
}
