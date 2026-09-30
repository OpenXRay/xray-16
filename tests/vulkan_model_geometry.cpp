#include "src/Layers/xrRenderVK/ModelGeometry.h"

#include <cassert>
#include <cmath>
#include <cstring>

using namespace xray::render::vulkan;
namespace
{
void put32(std::vector<uint8_t>& bytes, uint32_t n)
{
    for (unsigned i = 0; i < 4; ++i) bytes.push_back(static_cast<uint8_t>(n >> (i * 8)));
}
void put16(std::vector<uint8_t>& bytes, uint16_t n)
{
    bytes.push_back(static_cast<uint8_t>(n)); bytes.push_back(static_cast<uint8_t>(n >> 8));
}
void putfloat(std::vector<uint8_t>& bytes, float value)
{
    uint32_t bits; std::memcpy(&bits, &value, 4); put32(bytes, bits);
}
void chunk(std::vector<uint8_t>& dst, uint32_t id, const std::vector<uint8_t>& body)
{
    put32(dst, id); put32(dst, static_cast<uint32_t>(body.size()));
    dst.insert(dst.end(), body.begin(), body.end());
}
VisualRecord record(unsigned links)
{
    VisualRecord child;
    child.type = 5;
    std::vector<uint8_t> vertices, indices;
    constexpr uint32_t formats[]{0, 0x12071980, 0x240e3300, 0x481c6600, 0x5a238f80};
    put32(vertices, formats[links]); put32(vertices, 3);
    for (unsigned v = 0; v < 3; ++v)
    {
        if (links > 1)
            for (unsigned i = 0; i < links; ++i) put16(vertices, i);
        for (unsigned i = 0; i < 12; ++i) putfloat(vertices, i == 0 ? float(v) : 0);
        if (links > 1)
            for (unsigned i = 0; i + 1 < links; ++i)
                putfloat(vertices, links == 2 ? 0.25f : 1.0f / links);
        putfloat(vertices, 0.5f); putfloat(vertices, 0.75f);
        if (links == 1) put32(vertices, 3);
    }
    put32(indices, 3); put16(indices, 0); put16(indices, 1); put16(indices, 2);
    chunk(child.source, 3, vertices);
    chunk(child.source, 4, indices);
    VisualRecord root;
    root.type = 3;
    root.embedded_children.push_back(std::move(child));
    return root;
}
}
int main()
{
    for (unsigned links = 1; links <= 4; ++links)
    {
        auto visual = record(links);
        ModelGeometry geometry;
        std::string error;
        assert(decode_model_geometry(visual, geometry, error));
        assert(geometry.type == 3 && geometry.children.size() == 1);
        const auto& child = geometry.children[0];
        assert(child.vertices.size() == 3 && child.indices.size() == 3);
        assert(child.vertices[2].position[0] == 2 && child.vertices[2].uv[1] == 0.75f);
        float total = 0;
        for (unsigned i = 0; i < links; ++i) total += child.vertices[0].weights[i];
        assert(std::abs(total - 1) < 0.0001f);
        if (links == 2) assert(child.vertices[0].weights[0] == 0.75f);
        float pose[4][16]{};
        for (unsigned bone = 0; bone < 4; ++bone)
        {
            pose[bone][0] = pose[bone][5] = pose[bone][10] = pose[bone][15] = 1;
            pose[bone][12] = float(bone);
        }
        std::vector<LevelVertex> skinned;
        assert(skin_model_mesh(child, &pose[0][0], 4, skinned, error));
        assert(skinned.size() == 3);
        if (links == 2) assert(std::abs(skinned[0].position[0] - 0.25f) < 0.0001f);
        assert(!skin_model_mesh(child, &pose[0][0], 1, skinned, error));
        visual.embedded_children[0].source.pop_back();
        assert(!decode_model_geometry(visual, geometry, error));
    }
    VisualRecord plain;
    plain.type = 0;
    std::vector<uint8_t> vertices, indices;
    put32(vertices, 0x112); put32(vertices, 3);
    for (unsigned i = 0; i < 3; ++i)
    {
        putfloat(vertices, float(i));
        for (unsigned j = 1; j < 8; ++j) putfloat(vertices, 0);
    }
    put32(indices, 3); put16(indices, 0); put16(indices, 1); put16(indices, 2);
    chunk(plain.source, 3, vertices); chunk(plain.source, 4, indices);
    ModelGeometry geometry;
    std::string error;
    assert(decode_model_geometry(plain, geometry, error));
    assert(geometry.vertices[2].position[0] == 2 && geometry.indices.size() == 3);
    plain.type = 2;
    std::vector<uint8_t> windows;
    for (unsigned i = 0; i < 4; ++i) put32(windows, 0);
    put32(windows, 1); put32(windows, 0); put16(windows, 1); put16(windows, 3);
    chunk(plain.source, 6, windows);
    assert(decode_model_geometry(plain, geometry, error));
    assert(geometry.indices.size() == 3);
}
