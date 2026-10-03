#include "xrEngine/stdafx.h"
#include "src/Layers/xrRenderVK/ModelGeometry.h"
#include "src/Layers/xrRenderVK/VulkanModelVisual.h"
#include "src/Layers/xrRenderVK/VulkanVisual.h"

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
    Core.Initialize("vulkan_model_geometry_test", nullptr, false);
    VisualRecord static_record;
    static_record.type = 0;
    static_record.bounds = { -1, -2, -3, 1, 2, 3 };
    auto shared_geometry = std::make_shared<GpuModel>();
    const std::weak_ptr<GpuModel> lifetime = shared_geometry;
    auto first = std::make_unique<VulkanModelVisual>(static_record, "test.ogf", shared_geometry);
    auto second = std::make_unique<VulkanModelVisual>(*first);
    assert(&first->gpu() == &second->gpu());
    assert(&first->getVisData() != &second->getVisData());
    first->getVisData().box.vMin.x = -9;
    assert(second->getVisData().box.vMin.x == -1);
    shared_geometry.reset();
    first.reset();
    assert(!lifetime.expired());
    second.reset();
    assert(lifetime.expired());

    // Embedded hierarchy instances own separate child identities and bounds,
    // while every duplicate keeps the same leaf buffers alive.
    VisualRecord hierarchy;
    hierarchy.type = 1;
    hierarchy.bounds = {-2, -2, -2, 2, 2, 2, 0, 0, 0, 3};
    auto leaf_gpu = std::make_shared<GpuModel>();
    const std::weak_ptr<GpuModel> leaf_lifetime = leaf_gpu;
    auto parent = std::make_unique<VulkanModelVisual>(hierarchy, "tree.ogf", nullptr);
    parent->add_child(std::make_unique<VulkanModelVisual>(static_record, "", leaf_gpu));
    auto parent_copy = std::make_unique<VulkanModelVisual>(*parent);
    auto* child = parent->getSubModel(0);
    auto* child_copy = parent_copy->getSubModel(0);
    assert(parent->getType() == 1 && parent->getSubModel(1) == nullptr);
    assert(child && child_copy && child != child_copy);
    assert(parent_copy->find(child_copy) && !parent->find(child_copy));
    assert(&static_cast<VulkanModelVisual*>(child)->gpu() ==
        &static_cast<VulkanModelVisual*>(child_copy)->gpu());
    child->getVisData().sphere.R = 12;
    assert(child_copy->getVisData().sphere.R != 12);
    assert(parent_copy->getVisData().sphere.R == 3);
    parent->getVisData().box.vMin.x = -99;
    parent->getVisData().marker[0] = 123;
    vis_data external;
    parent->getVisData().obj_data = external.obj_data;
    parent->reset_instance_state();
    assert(parent->getVisData().box.vMin.x == -2);
    assert(parent->getVisData().marker[0] == 0 && parent->getVisData().obj_data != external.obj_data);
    assert(child->getVisData().sphere.R == 0);
    leaf_gpu.reset(); parent.reset();
    assert(!leaf_lifetime.expired());
    parent_copy.reset();
    assert(leaf_lifetime.expired());

    GpuLevel level;
    VisualRecord linked;
    linked.type = 1;
    linked.linked_children = {0};
    VulkanModelVisual borrowed(linked, "", nullptr, &level);
    assert(borrowed.linked_valid() && borrowed.getSubModel(0) == nullptr);
    level.destroy();
    assert(!borrowed.linked_valid() && borrowed.getSubModel(0) == nullptr);

    for (unsigned links = 1; links <= 4; ++links)
    {
        auto visual = record(links);
        ModelGeometry geometry;
        std::string error;
        assert(decode_model_geometry(visual, geometry, error));
        assert(geometry.type == 3 && geometry.children.size() == 1);
        const auto& child = geometry.children[0];
        assert(child.skin_weights == links);
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
    // Skeletal progressive children use the same active index windows as
    // static progressive meshes, with vertices evaluated per instance.
    auto animated = record(1);
    auto& progressive_child = animated.embedded_children[0];
    progressive_child.type = 4;
    auto& source = progressive_child.source;
    const size_t vertex_chunk = 8 + 8 + 3 * 60;
    source.resize(vertex_chunk);
    std::vector<uint8_t> skinned_indices, skinned_windows;
    put32(skinned_indices, 6);
    for (uint16_t index : {0, 1, 2, 0, 1, 2}) put16(skinned_indices, index);
    chunk(source, 4, skinned_indices);
    for (unsigned i = 0; i < 4; ++i) put32(skinned_windows, 0);
    put32(skinned_windows, 2);
    put32(skinned_windows, 0); put16(skinned_windows, 2); put16(skinned_windows, 3);
    put32(skinned_windows, 3); put16(skinned_windows, 1); put16(skinned_windows, 3);
    chunk(source, 6, skinned_windows);
    ModelGeometry skinned_geometry;
    std::string skinned_error;
    assert(decode_model_geometry(animated, skinned_geometry, skinned_error));
    assert(skinned_geometry.children[0].windows.size() == 2);
    assert(select_slide_window(skinned_geometry.children[0].windows, 0.f, 6).offset == 3);
    float first_pose[4][16]{}, second_pose[4][16]{};
    for (unsigned bone = 0; bone < 4; ++bone)
    {
        first_pose[bone][0] = first_pose[bone][5] = first_pose[bone][10] = first_pose[bone][15] = 1.f;
        second_pose[bone][0] = second_pose[bone][5] = second_pose[bone][10] = second_pose[bone][15] = 1.f;
        second_pose[bone][12] = 4.f;
    }
    std::vector<LevelVertex> first_vertices, second_vertices;
    assert(skin_model_mesh(skinned_geometry.children[0], &first_pose[0][0], 4, first_vertices, skinned_error));
    assert(skin_model_mesh(skinned_geometry.children[0], &second_pose[0][0], 4, second_vertices, skinned_error));
    assert(second_vertices[0].position[0] == first_vertices[0].position[0] + 4.f);
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
    VisualRecord progressive = plain;
    progressive.type = 2;
    progressive.source.clear();
    indices.clear();
    put32(indices, 6);
    for (uint16_t index : {0, 1, 2, 0, 1, 2}) put16(indices, index);
    chunk(progressive.source, 3, vertices); chunk(progressive.source, 4, indices);
    std::vector<uint8_t> windows;
    for (unsigned i = 0; i < 4; ++i) put32(windows, 0);
    put32(windows, 2);
    put32(windows, 0); put16(windows, 2); put16(windows, 3);
    put32(windows, 3); put16(windows, 1); put16(windows, 3);
    chunk(progressive.source, 6, windows);
    assert(decode_model_geometry(progressive, geometry, error));
    assert(geometry.indices.size() == 6 && geometry.windows.size() == 2);
    assert(select_slide_window(geometry.windows, 1.f, geometry.indices.size()).index_count == 6);
    assert(select_slide_window(geometry.windows, 0.f, geometry.indices.size()).offset == 3);
}
