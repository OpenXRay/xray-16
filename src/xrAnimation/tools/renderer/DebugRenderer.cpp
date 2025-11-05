#include "stdafx.h"
#include "DebugRenderer.h"

#include "VulkanDevice.h"
#include "VulkanPipeline.h"

// X-Ray geometry types
#include "xrCore/_vector3d.h"
#include "xrCore/_obb.h"
#include "xrCore/_sphere.h"
#include "xrCore/_cylinder.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace xray {
namespace animation {
namespace renderer {

namespace {

constexpr VkDeviceSize kDefaultVertexCapacity = 1024;
constexpr float kTwoPi = 6.28318530718f;
constexpr float kEpsilon = 1e-5f;

inline ozz::math::Float3 ToFloat3(const Fvector& v) {
    return ozz::math::Float3{ v.x, v.y, v.z };
}

inline ozz::math::Float3 Add(const ozz::math::Float3& a, const ozz::math::Float3& b) {
    return ozz::math::Float3{ a.x + b.x, a.y + b.y, a.z + b.z };
}

inline ozz::math::Float3 Sub(const ozz::math::Float3& a, const ozz::math::Float3& b) {
    return ozz::math::Float3{ a.x - b.x, a.y - b.y, a.z - b.z };
}

inline ozz::math::Float3 Scale(const ozz::math::Float3& v, float s) {
    return ozz::math::Float3{ v.x * s, v.y * s, v.z * s };
}

inline ozz::math::Float3 TransformPoint(const ozz::math::Float4x4& m, const ozz::math::Float3& p) {
    const ozz::math::SimdFloat4 point = ozz::math::simd_float4::Load3PtrU(&p.x);
    const ozz::math::SimdFloat4 transformed = ozz::math::TransformPoint(m, point);
    ozz::math::Float3 result;
    ozz::math::Store3PtrU(transformed, &result.x);
    return result;
}

inline ozz::math::Float3 TransformVector(const ozz::math::Float4x4& m, const ozz::math::Float3& v) {
    const ozz::math::SimdFloat4 vec = ozz::math::simd_float4::Load3PtrU(&v.x);
    const ozz::math::SimdFloat4 transformed = ozz::math::TransformVector(m, vec);
    ozz::math::Float3 result;
    ozz::math::Store3PtrU(transformed, &result.x);
    return result;
}

inline float ColumnLength(const ozz::math::SimdFloat4& column) {
    float values[4];
    ozz::math::StorePtrU(column, values);
    return std::sqrt(values[0] * values[0] + values[1] * values[1] + values[2] * values[2]);
}

using ozz::math::Cross;
using ozz::math::Length;
using ozz::math::NormalizeSafe;

} // namespace

VkDeviceSize DebugRenderer::VertexBufferSize(size_t vertex_count) {
    return static_cast<VkDeviceSize>(vertex_count * sizeof(LineVertex));
}

bool DebugRenderer::Initialize(VulkanDevice* device) {
    device_ = device;

    if (!device_) {
        return false;
    }

    if (!CreateDescriptorSetLayout()) {
        return false;
    }

    if (!CreateDescriptorPool()) {
        return false;
    }

    if (!CreateUniformBuffer()) {
        return false;
    }

    if (!EnsureLineCapacity(kDefaultVertexCapacity)) {
        return false;
    }

    if (!EnsureSolidCapacity(kDefaultVertexCapacity)) {
        return false;
    }

    if (!CreateLinePipeline()) {
        return false;
    }

    if (!CreateSolidPipeline()) {
        return false;
    }

    if (!CreateBoneInstancedPipeline()) {
        printf("! ERROR: Failed to create bone instanced pipeline!\n");
        return false;
    }

    // Generate unit geometry for GPU instancing
    printf("* DebugRenderer: Generating unit geometry for GPU instancing...\n");
    GenerateUnitOctahedron();
    GenerateUnitSphere(8);
    printf("* DebugRenderer: GPU instancing initialized successfully\n");

    UpdateDescriptorSet();
    initialized_ = true;
    return true;
}

void DebugRenderer::Shutdown() {
    if (!device_ || device_->GetDevice() == VK_NULL_HANDLE) {
        return;
    }

    VkDevice vk_device = device_->GetDevice();

    line_pipeline_.Destroy();
    solid_pipeline_.Destroy();
    bone_instanced_pipeline_.Destroy();

    unit_octahedron_buffer_.Destroy();
    unit_sphere_buffer_.Destroy();
    instance_buffer_.Destroy();

    if (descriptor_pool_) {
        vkDestroyDescriptorPool(vk_device, descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }

    if (descriptor_set_layout_) {
        vkDestroyDescriptorSetLayout(vk_device, descriptor_set_layout_, nullptr);
        descriptor_set_layout_ = VK_NULL_HANDLE;
    }

    uniform_buffer_.Destroy();
    line_vertex_buffer_.Destroy();
    solid_vertex_buffer_.Destroy();

    line_vertices_.clear();
    solid_vertices_.clear();
    bone_instances_.clear();
    sphere_instances_.clear();
    line_vertex_capacity_ = 0;
    solid_vertex_capacity_ = 0;
    instance_buffer_capacity_ = 0;
    unit_octahedron_vertex_count_ = 0;
    unit_sphere_vertex_count_ = 0;
    line_buffer_dirty_ = false;
    solid_buffer_dirty_ = false;
    instance_buffer_dirty_ = false;
    initialized_ = false;
}

void DebugRenderer::BeginFrame() {
    line_vertices_.clear();
    solid_vertices_.clear();
    bone_instances_.clear();
    sphere_instances_.clear();
    line_buffer_dirty_ = false;
    solid_buffer_dirty_ = false;
    instance_buffer_dirty_ = false;
}

void DebugRenderer::DrawLine(const ozz::math::Float3& start, const ozz::math::Float3& end, const ozz::math::Float4& color) {
    LineVertex v0{};
    v0.position[0] = start.x;
    v0.position[1] = start.y;
    v0.position[2] = start.z;
    v0.color[0] = color.x;
    v0.color[1] = color.y;
    v0.color[2] = color.z;
    v0.color[3] = color.w;

    LineVertex v1{};
    v1.position[0] = end.x;
    v1.position[1] = end.y;
    v1.position[2] = end.z;
    v1.color[0] = color.x;
    v1.color[1] = color.y;
    v1.color[2] = color.z;
    v1.color[3] = color.w;

    line_vertices_.push_back(v0);
    line_vertices_.push_back(v1);
    line_buffer_dirty_ = true;
}

void DebugRenderer::DrawAxes(const ozz::math::Float4x4& transform, float scale, const ozz::math::Float4& color_x,
    const ozz::math::Float4& color_y, const ozz::math::Float4& color_z) {
    ozz::math::Float3 origin;
    ozz::math::Float3 axis_x;
    ozz::math::Float3 axis_y;
    ozz::math::Float3 axis_z;

    ozz::math::Store3PtrU(transform.cols[3], &origin.x);
    ozz::math::Store3PtrU(transform.cols[0], &axis_x.x);
    ozz::math::Store3PtrU(transform.cols[1], &axis_y.x);
    ozz::math::Store3PtrU(transform.cols[2], &axis_z.x);

    axis_x = Scale(axis_x, scale);
    axis_y = Scale(axis_y, scale);
    axis_z = Scale(axis_z, scale);

    DrawLine(origin, Add(origin, axis_x), color_x);
    DrawLine(origin, Add(origin, axis_y), color_y);
    DrawLine(origin, Add(origin, axis_z), color_z);
}

void DebugRenderer::DrawPoint(const ozz::math::Float3& position, float radius, const ozz::math::Float4& color, int segments) {
    segments = std::max(segments, 4);
    const float angle_step = kTwoPi / static_cast<float>(segments);

    float current_angle = 0.f;
    for (int i = 0; i < segments; ++i) {
        const float next_angle = current_angle + angle_step;
        const ozz::math::Float3 start{
            position.x + std::cos(current_angle) * radius,
            position.y,
            position.z + std::sin(current_angle) * radius };
        const ozz::math::Float3 end{
            position.x + std::cos(next_angle) * radius,
            position.y,
            position.z + std::sin(next_angle) * radius };
        DrawLine(start, end, color);
        current_angle = next_angle;
    }
}

void DebugRenderer::DrawSphere(const ozz::math::Float3& center, float radius, const ozz::math::Float4& color, int segments) {
    segments = std::max(segments, 8);
    const float angle_step = kTwoPi / static_cast<float>(segments);

    auto draw_ring = [&](auto sample_point) {
        float current = 0.f;
        for (int i = 0; i < segments; ++i) {
            const float next = current + angle_step;
            const ozz::math::Float3 start = sample_point(current);
            const ozz::math::Float3 end = sample_point(next);
            DrawLine(start, end, color);
            current = next;
        }
    };

    draw_ring([&](float angle) {
        return ozz::math::Float3{
            center.x + std::cos(angle) * radius,
            center.y + std::sin(angle) * radius,
            center.z };
    });

    draw_ring([&](float angle) {
        return ozz::math::Float3{
            center.x + std::cos(angle) * radius,
            center.y,
            center.z + std::sin(angle) * radius };
    });

    draw_ring([&](float angle) {
        return ozz::math::Float3{
            center.x,
            center.y + std::cos(angle) * radius,
            center.z + std::sin(angle) * radius };
    });
}

void DebugRenderer::DrawSolidSphere(const ozz::math::Float3& center, float radius, const ozz::math::Float4& color, int segments) {
    // Generate UV sphere with proper normals
    segments = std::max(segments, 8);
    const int rings = segments / 2;
    const float pi = 3.14159265359f;

    auto push_triangle_with_normal = [&](const ozz::math::Float3& p0, const ozz::math::Float3& p1, const ozz::math::Float3& p2) {
        // Compute normal from vertex positions (pointing outward from sphere center)
        ozz::math::Float3 n0 = NormalizeSafe(Sub(p0, center), ozz::math::Float3::y_axis());
        ozz::math::Float3 n1 = NormalizeSafe(Sub(p1, center), ozz::math::Float3::y_axis());
        ozz::math::Float3 n2 = NormalizeSafe(Sub(p2, center), ozz::math::Float3::y_axis());

        SolidVertex v0{};
        v0.position[0] = p0.x;
        v0.position[1] = p0.y;
        v0.position[2] = p0.z;
        v0.normal[0] = n0.x;
        v0.normal[1] = n0.y;
        v0.normal[2] = n0.z;
        v0.color[0] = color.x;
        v0.color[1] = color.y;
        v0.color[2] = color.z;
        v0.color[3] = color.w;

        SolidVertex v1 = v0;
        v1.position[0] = p1.x;
        v1.position[1] = p1.y;
        v1.position[2] = p1.z;
        v1.normal[0] = n1.x;
        v1.normal[1] = n1.y;
        v1.normal[2] = n1.z;

        SolidVertex v2 = v0;
        v2.position[0] = p2.x;
        v2.position[1] = p2.y;
        v2.position[2] = p2.z;
        v2.normal[0] = n2.x;
        v2.normal[1] = n2.y;
        v2.normal[2] = n2.z;

        solid_vertices_.push_back(v0);
        solid_vertices_.push_back(v1);
        solid_vertices_.push_back(v2);
        solid_buffer_dirty_ = true;
    };

    // Generate sphere vertices
    for (int ring = 0; ring < rings; ++ring) {
        const float phi0 = pi * static_cast<float>(ring) / static_cast<float>(rings);
        const float phi1 = pi * static_cast<float>(ring + 1) / static_cast<float>(rings);

        for (int seg = 0; seg < segments; ++seg) {
            const float theta0 = kTwoPi * static_cast<float>(seg) / static_cast<float>(segments);
            const float theta1 = kTwoPi * static_cast<float>(seg + 1) / static_cast<float>(segments);

            // Compute sphere points
            const float sin_phi0 = std::sin(phi0);
            const float cos_phi0 = std::cos(phi0);
            const float sin_phi1 = std::sin(phi1);
            const float cos_phi1 = std::cos(phi1);

            const ozz::math::Float3 p0{
                center.x + radius * sin_phi0 * std::cos(theta0),
                center.y + radius * cos_phi0,
                center.z + radius * sin_phi0 * std::sin(theta0)
            };

            const ozz::math::Float3 p1{
                center.x + radius * sin_phi0 * std::cos(theta1),
                center.y + radius * cos_phi0,
                center.z + radius * sin_phi0 * std::sin(theta1)
            };

            const ozz::math::Float3 p2{
                center.x + radius * sin_phi1 * std::cos(theta0),
                center.y + radius * cos_phi1,
                center.z + radius * sin_phi1 * std::sin(theta0)
            };

            const ozz::math::Float3 p3{
                center.x + radius * sin_phi1 * std::cos(theta1),
                center.y + radius * cos_phi1,
                center.z + radius * sin_phi1 * std::sin(theta1)
            };

            // Create two triangles for the quad - reversed winding for outward normals
            push_triangle_with_normal(p0, p2, p1);
            push_triangle_with_normal(p1, p2, p3);
        }
    }
}

void DebugRenderer::DrawOctahedronBone(const ozz::math::Float3& head, const ozz::math::Float3& tail,
    float radius, const ozz::math::Float4& color) {
    // Compute bone direction and length
    const ozz::math::Float3 bone_vec = Sub(tail, head);
    const float length = std::sqrt(bone_vec.x * bone_vec.x + bone_vec.y * bone_vec.y + bone_vec.z * bone_vec.z);

    if (length <= kEpsilon) {
        return; // Degenerate bone
    }

    const ozz::math::Float3 bone_dir = Scale(bone_vec, 1.0f / length);

    // Create perpendicular vectors for octahedron
    ozz::math::Float3 tangent = Cross(bone_dir, ozz::math::Float3::y_axis());
    if (std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z) <= kEpsilon) {
        tangent = Cross(bone_dir, ozz::math::Float3::x_axis());
    }
    tangent = NormalizeSafe(tangent, ozz::math::Float3::x_axis());
    const ozz::math::Float3 bitangent = NormalizeSafe(Cross(bone_dir, tangent), ozz::math::Float3::z_axis());

    // Octahedron vertices: middle ring with 4 points
    const float mid_offset = length * 0.5f;
    const ozz::math::Float3 mid_center = Add(head, Scale(bone_dir, mid_offset));

    const std::array<ozz::math::Float3, 6> vertices = {
        head,  // 0: head
        tail,  // 1: tail
        Add(mid_center, Scale(tangent, radius)),      // 2: +X
        Add(mid_center, Scale(tangent, -radius)),     // 3: -X
        Add(mid_center, Scale(bitangent, radius)),    // 4: +Y
        Add(mid_center, Scale(bitangent, -radius)),   // 5: -Y
    };

    auto push_triangle_with_normal = [&](int ia, int ib, int ic) {
        const ozz::math::Float3& p0 = vertices[ia];
        const ozz::math::Float3& p1 = vertices[ib];
        const ozz::math::Float3& p2 = vertices[ic];

        // Compute face normal
        const ozz::math::Float3 edge1 = Sub(p1, p0);
        const ozz::math::Float3 edge2 = Sub(p2, p0);
        const ozz::math::Float3 normal = NormalizeSafe(Cross(edge1, edge2), bone_dir);

        SolidVertex v0{};
        v0.position[0] = p0.x;
        v0.position[1] = p0.y;
        v0.position[2] = p0.z;
        v0.normal[0] = normal.x;
        v0.normal[1] = normal.y;
        v0.normal[2] = normal.z;
        v0.color[0] = color.x;
        v0.color[1] = color.y;
        v0.color[2] = color.z;
        v0.color[3] = color.w;

        SolidVertex v1 = v0;
        v1.position[0] = p1.x;
        v1.position[1] = p1.y;
        v1.position[2] = p1.z;

        SolidVertex v2 = v0;
        v2.position[0] = p2.x;
        v2.position[1] = p2.y;
        v2.position[2] = p2.z;

        solid_vertices_.push_back(v0);
        solid_vertices_.push_back(v1);
        solid_vertices_.push_back(v2);
        solid_buffer_dirty_ = true;
    };

    // Head pyramid (4 triangles) - reversed winding for outward normals
    push_triangle_with_normal(0, 4, 2);
    push_triangle_with_normal(0, 3, 4);
    push_triangle_with_normal(0, 5, 3);
    push_triangle_with_normal(0, 2, 5);

    // Tail pyramid (4 triangles) - reversed winding for outward normals
    push_triangle_with_normal(1, 2, 4);
    push_triangle_with_normal(1, 4, 3);
    push_triangle_with_normal(1, 3, 5);
    push_triangle_with_normal(1, 5, 2);
}

void DebugRenderer::DrawOrientedBox(const ozz::math::Float4x4& transform, const Fobb& obb, const ozz::math::Float4& color) {
    const ozz::math::Float3 center = ToFloat3(obb.m_translate);
    const ozz::math::Float3 axis_x = Scale(ToFloat3(obb.m_rotate.i), obb.m_halfsize.x);
    const ozz::math::Float3 axis_y = Scale(ToFloat3(obb.m_rotate.j), obb.m_halfsize.y);
    const ozz::math::Float3 axis_z = Scale(ToFloat3(obb.m_rotate.k), obb.m_halfsize.z);

    std::array<ozz::math::Float3, 8> corners{};
    int index = 0;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sy = -1; sy <= 1; sy += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                ozz::math::Float3 local = center;
                local = Add(local, Scale(axis_x, static_cast<float>(sx)));
                local = Add(local, Scale(axis_y, static_cast<float>(sy)));
                local = Add(local, Scale(axis_z, static_cast<float>(sz)));
                corners[index++] = TransformPoint(transform, local);
            }
        }
    }

    static constexpr int edges[12][2] = {
        {0, 1}, {0, 2}, {0, 4}, {1, 3},
        {1, 5}, {2, 3}, {2, 6}, {3, 7},
        {4, 5}, {4, 6}, {5, 7}, {6, 7},
    };

    for (const auto& edge : edges) {
        DrawLine(corners[edge[0]], corners[edge[1]], color);
    }
}

void DebugRenderer::DrawSphereShape(const ozz::math::Float4x4& transform, const Fsphere& sphere,
    const ozz::math::Float4& color, int segments) {
    const ozz::math::Float3 center_local = ToFloat3(sphere.P);
    const ozz::math::Float3 center_world = TransformPoint(transform, center_local);

    const float sx = ColumnLength(transform.cols[0]);
    const float sy = ColumnLength(transform.cols[1]);
    const float sz = ColumnLength(transform.cols[2]);
    const float uniform_scale = (sx + sy + sz) / 3.f;

    DrawSphere(center_world, sphere.R * uniform_scale, color, segments);
}

void DebugRenderer::DrawCapsuleShape(const ozz::math::Float4x4& transform, const Fcylinder& cylinder,
    const ozz::math::Float4& color, int segments) {
    segments = std::max(segments, 8);

    ozz::math::Float3 axis_dir = NormalizeSafe(ToFloat3(cylinder.m_direction), ozz::math::Float3::z_axis());
    const float half_height = std::max(0.f, cylinder.m_height * 0.5f);

    const ozz::math::Float3 center = ToFloat3(cylinder.m_center);
    const ozz::math::Float3 top_center = Add(center, Scale(axis_dir, half_height));
    const ozz::math::Float3 bottom_center = Sub(center, Scale(axis_dir, half_height));

    ozz::math::Float3 tangent = Cross(axis_dir, ozz::math::Float3::y_axis());
    if (Length(tangent) <= kEpsilon) {
        tangent = Cross(axis_dir, ozz::math::Float3::x_axis());
    }
    tangent = NormalizeSafe(tangent, ozz::math::Float3::x_axis());
    ozz::math::Float3 bitangent = NormalizeSafe(Cross(axis_dir, tangent), ozz::math::Float3::z_axis());

    const float angle_step = kTwoPi / static_cast<float>(segments);

    ozz::math::Float3 first_top{};
    ozz::math::Float3 prev_top{};
    ozz::math::Float3 first_bottom{};
    ozz::math::Float3 prev_bottom{};
    bool first_point = true;

    for (int i = 0; i < segments; ++i) {
        const float angle = angle_step * static_cast<float>(i);
        const float cos_a = std::cos(angle);
        const float sin_a = std::sin(angle);
        const ozz::math::Float3 offset =
            Add(Scale(tangent, cos_a * cylinder.m_radius), Scale(bitangent, sin_a * cylinder.m_radius));

        const ozz::math::Float3 top_world = TransformPoint(transform, Add(top_center, offset));
        const ozz::math::Float3 bottom_world = TransformPoint(transform, Add(bottom_center, offset));

        if (first_point) {
            first_top = top_world;
            first_bottom = bottom_world;
            first_point = false;
        } else {
            DrawLine(prev_top, top_world, color);
            DrawLine(prev_bottom, bottom_world, color);
        }

        DrawLine(top_world, bottom_world, color);

        prev_top = top_world;
        prev_bottom = bottom_world;
    }

    if (!first_point) {
        DrawLine(prev_top, first_top, color);
        DrawLine(prev_bottom, first_bottom, color);
    }

    const ozz::math::Float3 top_world = TransformPoint(transform, top_center);
    const ozz::math::Float3 bottom_world = TransformPoint(transform, bottom_center);
    DrawLine(top_world, bottom_world, color);
}

void DebugRenderer::GenerateUnitOctahedron() {
    // Generate unit octahedron centered at origin with vertices at:
    // Top (0,1,0), Bottom (0,-1,0), and 4 mid-ring vertices at distance 1
    xr_vector<InstanceVertex> vertices;
    vertices.reserve(24);  // 8 triangles × 3 vertices

    const ozz::math::Float3 top{0, 1, 0};
    const ozz::math::Float3 bottom{0, -1, 0};
    const ozz::math::Float3 v0{1, 0, 0};   // +X
    const ozz::math::Float3 v1{0, 0, 1};   // +Z
    const ozz::math::Float3 v2{-1, 0, 0};  // -X
    const ozz::math::Float3 v3{0, 0, -1};  // -Z

    auto push_triangle = [&](const ozz::math::Float3& a, const ozz::math::Float3& b, const ozz::math::Float3& c) {
        // Compute face normal
        ozz::math::Float3 edge1 = Sub(b, a);
        ozz::math::Float3 edge2 = Sub(c, a);
        ozz::math::Float3 normal = NormalizeSafe(Cross(edge1, edge2), ozz::math::Float3::y_axis());

        InstanceVertex v{};
        v.normal[0] = normal.x;
        v.normal[1] = normal.y;
        v.normal[2] = normal.z;

        v.position[0] = a.x; v.position[1] = a.y; v.position[2] = a.z;
        vertices.push_back(v);
        v.position[0] = b.x; v.position[1] = b.y; v.position[2] = b.z;
        vertices.push_back(v);
        v.position[0] = c.x; v.position[1] = c.y; v.position[2] = c.z;
        vertices.push_back(v);
    };

    // Top pyramid (4 triangles)
    push_triangle(top, v0, v1);
    push_triangle(top, v1, v2);
    push_triangle(top, v2, v3);
    push_triangle(top, v3, v0);

    // Bottom pyramid (4 triangles)
    push_triangle(bottom, v1, v0);
    push_triangle(bottom, v2, v1);
    push_triangle(bottom, v3, v2);
    push_triangle(bottom, v0, v3);

    unit_octahedron_vertex_count_ = static_cast<uint32_t>(vertices.size());
    VkDeviceSize buffer_size = static_cast<VkDeviceSize>(vertices.size() * sizeof(InstanceVertex));

    unit_octahedron_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    unit_octahedron_buffer_.Upload(vertices.data(), buffer_size);

    printf("* GenerateUnitOctahedron: %u vertices\n", unit_octahedron_vertex_count_);
}

void DebugRenderer::GenerateUnitSphere(int segments) {
    // Generate UV sphere with radius 1.0 centered at origin
    segments = std::max(segments, 8);
    const int rings = segments / 2;
    const float pi = 3.14159265359f;

    xr_vector<InstanceVertex> vertices;
    vertices.reserve(rings * segments * 6);  // Approximate

    auto push_triangle = [&](const ozz::math::Float3& p0, const ozz::math::Float3& p1, const ozz::math::Float3& p2) {
        // For unit sphere, normals are same as positions (pointing outward from origin)
        ozz::math::Float3 n0 = NormalizeSafe(p0, ozz::math::Float3::y_axis());
        ozz::math::Float3 n1 = NormalizeSafe(p1, ozz::math::Float3::y_axis());
        ozz::math::Float3 n2 = NormalizeSafe(p2, ozz::math::Float3::y_axis());

        InstanceVertex v{};
        v.position[0] = p0.x; v.position[1] = p0.y; v.position[2] = p0.z;
        v.normal[0] = n0.x; v.normal[1] = n0.y; v.normal[2] = n0.z;
        vertices.push_back(v);

        v.position[0] = p1.x; v.position[1] = p1.y; v.position[2] = p1.z;
        v.normal[0] = n1.x; v.normal[1] = n1.y; v.normal[2] = n1.z;
        vertices.push_back(v);

        v.position[0] = p2.x; v.position[1] = p2.y; v.position[2] = p2.z;
        v.normal[0] = n2.x; v.normal[1] = n2.y; v.normal[2] = n2.z;
        vertices.push_back(v);
    };

    // Generate sphere vertices
    for (int ring = 0; ring < rings; ++ring) {
        const float phi0 = pi * static_cast<float>(ring) / static_cast<float>(rings);
        const float phi1 = pi * static_cast<float>(ring + 1) / static_cast<float>(rings);

        for (int seg = 0; seg < segments; ++seg) {
            const float theta0 = kTwoPi * static_cast<float>(seg) / static_cast<float>(segments);
            const float theta1 = kTwoPi * static_cast<float>(seg + 1) / static_cast<float>(segments);

            // Compute sphere points (radius = 1.0)
            const float sin_phi0 = std::sin(phi0);
            const float cos_phi0 = std::cos(phi0);
            const float sin_phi1 = std::sin(phi1);
            const float cos_phi1 = std::cos(phi1);

            const ozz::math::Float3 p0{
                sin_phi0 * std::cos(theta0),
                cos_phi0,
                sin_phi0 * std::sin(theta0)
            };

            const ozz::math::Float3 p1{
                sin_phi0 * std::cos(theta1),
                cos_phi0,
                sin_phi0 * std::sin(theta1)
            };

            const ozz::math::Float3 p2{
                sin_phi1 * std::cos(theta0),
                cos_phi1,
                sin_phi1 * std::sin(theta0)
            };

            const ozz::math::Float3 p3{
                sin_phi1 * std::cos(theta1),
                cos_phi1,
                sin_phi1 * std::sin(theta1)
            };

            // Create two triangles for the quad
            push_triangle(p0, p2, p1);
            push_triangle(p1, p2, p3);
        }
    }

    unit_sphere_vertex_count_ = static_cast<uint32_t>(vertices.size());
    VkDeviceSize buffer_size = static_cast<VkDeviceSize>(vertices.size() * sizeof(InstanceVertex));

    unit_sphere_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    unit_sphere_buffer_.Upload(vertices.data(), buffer_size);

    printf("* GenerateUnitSphere: %u vertices\n", unit_sphere_vertex_count_);
}

void DebugRenderer::DrawBoneShape(const ozz::math::Float3& head, const ozz::math::Float3& tail,
    float radius, const ozz::math::Float4& color) {
    static int call_count = 0;
    static int frame_calls = 0;
    static int last_frame_report = 0;

    call_count++;
    frame_calls++;

    // Report every 60 frames
    if (call_count / 60 > last_frame_report) {
        printf("! WARNING: DrawBoneShape (NON-BATCHED) called %d times in last 60 frames - batching NOT active!\n",
               frame_calls);
        frame_calls = 0;
        last_frame_report = call_count / 60;
    }

    const float final_radius = std::max(radius, 0.005f);
    DrawOctahedronBone(head, tail, final_radius, color);

    const float joint_radius = final_radius * 0.7f;
    DrawSolidSphere(head, joint_radius, color, 12);
    DrawSolidSphere(tail, joint_radius, color, 12);
}

void DebugRenderer::DrawGrid(const ozz::math::Float3& center, float size, int divisions,
    const ozz::math::Float4& color_main, const ozz::math::Float4& color_sub) {
    divisions = std::max(divisions, 1);
    const float half_size = size * 0.5f;
    const float step = size / static_cast<float>(divisions);

    // Draw grid lines
    for (int i = 0; i <= divisions; ++i) {
        const float offset = -half_size + step * static_cast<float>(i);
        const bool is_center_line = (i == divisions / 2);
        const ozz::math::Float4& line_color = is_center_line ? color_main : color_sub;

        // Lines along X axis
        const ozz::math::Float3 x_start{center.x - half_size, center.y, center.z + offset};
        const ozz::math::Float3 x_end{center.x + half_size, center.y, center.z + offset};
        DrawLine(x_start, x_end, line_color);

        // Lines along Z axis
        const ozz::math::Float3 z_start{center.x + offset, center.y, center.z - half_size};
        const ozz::math::Float3 z_end{center.x + offset, center.y, center.z + half_size};
        DrawLine(z_start, z_end, line_color);
    }
}

void DebugRenderer::EndFrame() {
    if (line_buffer_dirty_) {
        const size_t vertex_count = line_vertices_.size();
        if (EnsureLineCapacity(vertex_count) && vertex_count > 0) {
            line_vertex_buffer_.Upload(line_vertices_.data(), VertexBufferSize(vertex_count));
        }
        line_buffer_dirty_ = false;
    }

    if (solid_buffer_dirty_) {
        const size_t vertex_count = solid_vertices_.size();
        if (EnsureSolidCapacity(vertex_count) && vertex_count > 0) {
            const VkDeviceSize upload_size = static_cast<VkDeviceSize>(vertex_count * sizeof(SolidVertex));
            solid_vertex_buffer_.Upload(solid_vertices_.data(), upload_size);
        }
        solid_buffer_dirty_ = false;
    }

    if (instance_buffer_dirty_) {
        // Combine bone_instances_ and sphere_instances_ into one buffer
        // We'll draw them separately with different first instance indices
        const size_t total_instances = bone_instances_.size() + sphere_instances_.size();
        if (EnsureInstanceCapacity(total_instances) && total_instances > 0) {
            // Combine both vectors into a single upload
            xr_vector<InstanceData> all_instances;
            all_instances.reserve(total_instances);
            all_instances.insert(all_instances.end(), bone_instances_.begin(), bone_instances_.end());
            all_instances.insert(all_instances.end(), sphere_instances_.begin(), sphere_instances_.end());

            const VkDeviceSize upload_size = static_cast<VkDeviceSize>(all_instances.size() * sizeof(InstanceData));
            instance_buffer_.Upload(all_instances.data(), upload_size);
        }
        instance_buffer_dirty_ = false;
    }
}

void DebugRenderer::Render(VkCommandBuffer cmd, const ozz::math::Float4x4& view_proj) {
    if (!initialized_) {
        return;
    }

    UpdateUniforms(view_proj);

    static int render_frame = 0;
    if (render_frame++ < 3) {
        printf("* DebugRenderer::Render - line_vertices=%zu, solid_vertices=%zu, bone_instances=%zu, sphere_instances=%zu\n",
               line_vertices_.size(), solid_vertices_.size(), bone_instances_.size(), sphere_instances_.size());
        if (!line_vertices_.empty()) {
            printf("  First line: (%.2f,%.2f,%.2f) -> (%.2f,%.2f,%.2f)\n",
                   line_vertices_[0].position[0], line_vertices_[0].position[1], line_vertices_[0].position[2],
                   line_vertices_[1].position[0], line_vertices_[1].position[1], line_vertices_[1].position[2]);
        }
        if (!bone_instances_.empty()) {
            const auto& first = bone_instances_[0];
            printf("  First bone instance transform: [%.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f]\n",
                   first.transform[0], first.transform[1], first.transform[2], first.transform[3],
                   first.transform[4], first.transform[5], first.transform[6], first.transform[7],
                   first.transform[8], first.transform[9], first.transform[10], first.transform[11],
                   first.transform[12], first.transform[13], first.transform[14], first.transform[15]);
        }
    }

    if (!line_vertices_.empty()) {
        line_pipeline_.Bind(cmd);
        VkBuffer buffers[] = { line_vertex_buffer_.GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, line_pipeline_.GetLayout(), 0, 1, &descriptor_set_, 0, nullptr);
        vkCmdDraw(cmd, static_cast<uint32_t>(line_vertices_.size()), 1, 0, 0);
    }

    if (!solid_vertices_.empty()) {
        solid_pipeline_.Bind(cmd);
        VkBuffer buffers[] = { solid_vertex_buffer_.GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, solid_pipeline_.GetLayout(), 0, 1, &descriptor_set_, 0, nullptr);
        vkCmdDraw(cmd, static_cast<uint32_t>(solid_vertices_.size()), 1, 0, 0);
    }

    // GPU instanced rendering for bones and spheres
    if (!bone_instances_.empty() || !sphere_instances_.empty()) {
        bone_instanced_pipeline_.Bind(cmd);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bone_instanced_pipeline_.GetLayout(), 0, 1, &descriptor_set_, 0, nullptr);

        // Draw octahedrons (bone shapes)
        if (!bone_instances_.empty()) {
            VkBuffer vertex_buffers[] = { unit_octahedron_buffer_.GetBuffer(), instance_buffer_.GetBuffer() };
            VkDeviceSize vertex_offsets[] = { 0, 0 };
            vkCmdBindVertexBuffers(cmd, 0, 2, vertex_buffers, vertex_offsets);
            // Use firstInstance=0 to read from start of instance buffer
            vkCmdDraw(cmd, unit_octahedron_vertex_count_, static_cast<uint32_t>(bone_instances_.size()), 0, 0);

            static int frame_count = 0;
            if (frame_count++ < 3) {
                printf("* GPU Instanced octahedrons: %u vertices × %zu instances = %zu total draws\n",
                       unit_octahedron_vertex_count_, bone_instances_.size(),
                       unit_octahedron_vertex_count_ * bone_instances_.size());
            }
        }

        // Draw spheres (joint shapes)
        if (!sphere_instances_.empty()) {
            VkBuffer vertex_buffers[] = { unit_sphere_buffer_.GetBuffer(), instance_buffer_.GetBuffer() };
            VkDeviceSize vertex_offsets[] = { 0, 0 };
            vkCmdBindVertexBuffers(cmd, 0, 2, vertex_buffers, vertex_offsets);
            // Use firstInstance to skip past bone instances in the buffer
            vkCmdDraw(cmd, unit_sphere_vertex_count_, static_cast<uint32_t>(sphere_instances_.size()), 0, static_cast<uint32_t>(bone_instances_.size()));

            static int frame_count = 0;
            if (frame_count++ < 3) {
                printf("* GPU Instanced spheres: %u vertices × %zu instances = %zu total draws\n",
                       unit_sphere_vertex_count_, sphere_instances_.size(),
                       unit_sphere_vertex_count_ * sphere_instances_.size());
            }
        }
    }
}

bool DebugRenderer::EnsureLineCapacity(size_t vertex_count) {
    if (vertex_count <= line_vertex_capacity_) {
        return true;
    }

    size_t new_capacity = std::max(vertex_count, line_vertex_capacity_ * 2);
    if (new_capacity == 0) {
        new_capacity = static_cast<size_t>(kDefaultVertexCapacity);
    }

    line_vertex_buffer_.Destroy();
    line_vertex_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), VertexBufferSize(new_capacity),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    if (line_vertex_buffer_.GetBuffer() == VK_NULL_HANDLE) {
        return false;
    }

    line_vertex_capacity_ = new_capacity;
    return true;
}

bool DebugRenderer::EnsureSolidCapacity(size_t vertex_count) {
    if (vertex_count <= solid_vertex_capacity_) {
        return true;
    }

    size_t new_capacity = std::max(vertex_count, solid_vertex_capacity_ * 2);
    if (new_capacity == 0) {
        new_capacity = static_cast<size_t>(kDefaultVertexCapacity);
    }

    solid_vertex_buffer_.Destroy();
    const VkDeviceSize buffer_size = static_cast<VkDeviceSize>(new_capacity * sizeof(SolidVertex));
    solid_vertex_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    if (solid_vertex_buffer_.GetBuffer() == VK_NULL_HANDLE) {
        return false;
    }

    solid_vertex_capacity_ = new_capacity;
    return true;
}

bool DebugRenderer::EnsureInstanceCapacity(size_t instance_count) {
    if (instance_count <= instance_buffer_capacity_) {
        return true;
    }

    size_t new_capacity = std::max(instance_count, instance_buffer_capacity_ * 2);
    if (new_capacity == 0) {
        new_capacity = 512;  // Default capacity for instances
    }

    instance_buffer_.Destroy();
    const VkDeviceSize buffer_size = static_cast<VkDeviceSize>(new_capacity * sizeof(InstanceData));
    instance_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    if (instance_buffer_.GetBuffer() == VK_NULL_HANDLE) {
        return false;
    }

    instance_buffer_capacity_ = new_capacity;
    return true;
}

bool DebugRenderer::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &binding;

    VkResult result = vkCreateDescriptorSetLayout(device_->GetDevice(), &layout_info, nullptr, &descriptor_set_layout_);
    return result == VK_SUCCESS;
}

bool DebugRenderer::CreateDescriptorPool() {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = 1;

    VkResult result = vkCreateDescriptorPool(device_->GetDevice(), &pool_info, nullptr, &descriptor_pool_);
    if (result != VK_SUCCESS) {
        return false;
    }

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor_set_layout_;

    result = vkAllocateDescriptorSets(device_->GetDevice(), &alloc_info, &descriptor_set_);
    return result == VK_SUCCESS;
}

bool DebugRenderer::CreateUniformBuffer() {
    uniform_buffer_.Create(device_->GetDevice(), device_->GetAllocator(), sizeof(ozz::math::Float4x4),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    return uniform_buffer_.GetBuffer() != VK_NULL_HANDLE;
}

bool DebugRenderer::CreateLinePipeline() {
    PipelineConfig config{};

#ifdef OZZ_SHADER_BINARY_DIR
    std::filesystem::path shader_root{ OZZ_SHADER_BINARY_DIR };
    config.vertex_shader_path = (shader_root / "debug_lines.vert.spv").string();
    config.fragment_shader_path = (shader_root / "debug_lines.frag.spv").string();
#else
    config.vertex_shader_path = "src/xrAnimation/tools/renderer/shaders/debug_lines.vert.spv";
    config.fragment_shader_path = "src/xrAnimation/tools/renderer/shaders/debug_lines.frag.spv";
#endif

    config.vertex_bindings.resize(1);
    config.vertex_bindings[0].binding = 0;
    config.vertex_bindings[0].stride = sizeof(LineVertex);
    config.vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    config.vertex_attributes.resize(2);
    config.vertex_attributes[0].location = 0;
    config.vertex_attributes[0].binding = 0;
    config.vertex_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    config.vertex_attributes[0].offset = offsetof(LineVertex, position);

    config.vertex_attributes[1].location = 1;
    config.vertex_attributes[1].binding = 0;
    config.vertex_attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    config.vertex_attributes[1].offset = offsetof(LineVertex, color);

    config.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    config.polygon_mode = VK_POLYGON_MODE_LINE;
    config.cull_mode = VK_CULL_MODE_NONE;
    config.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    config.depth_test_enable = true;
    config.depth_write_enable = false;
    config.blend_enable = true;
    config.line_width = 1.0f;
    config.descriptor_set_layouts.push_back(descriptor_set_layout_);

    return line_pipeline_.Create(device_->GetDevice(), config);
}

bool DebugRenderer::CreateSolidPipeline() {
    PipelineConfig config{};

#ifdef OZZ_SHADER_BINARY_DIR
    std::filesystem::path shader_root{ OZZ_SHADER_BINARY_DIR };
    config.vertex_shader_path = (shader_root / "shaded_bone.vert.spv").string();
    config.fragment_shader_path = (shader_root / "shaded_bone.frag.spv").string();
#else
    config.vertex_shader_path = "src/xrAnimation/tools/renderer/shaders/shaded_bone.vert.spv";
    config.fragment_shader_path = "src/xrAnimation/tools/renderer/shaders/shaded_bone.frag.spv";
#endif

    config.vertex_bindings.resize(1);
    config.vertex_bindings[0].binding = 0;
    config.vertex_bindings[0].stride = sizeof(SolidVertex);
    config.vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    config.vertex_attributes.resize(3);
    config.vertex_attributes[0].location = 0;
    config.vertex_attributes[0].binding = 0;
    config.vertex_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    config.vertex_attributes[0].offset = offsetof(SolidVertex, position);

    config.vertex_attributes[1].location = 1;
    config.vertex_attributes[1].binding = 0;
    config.vertex_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    config.vertex_attributes[1].offset = offsetof(SolidVertex, normal);

    config.vertex_attributes[2].location = 2;
    config.vertex_attributes[2].binding = 0;
    config.vertex_attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    config.vertex_attributes[2].offset = offsetof(SolidVertex, color);

    config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.polygon_mode = VK_POLYGON_MODE_FILL;
    config.cull_mode = VK_CULL_MODE_NONE;
    config.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    config.depth_test_enable = true;
    config.depth_write_enable = false;
    config.blend_enable = true;
    config.descriptor_set_layouts.push_back(descriptor_set_layout_);

    return solid_pipeline_.Create(device_->GetDevice(), config);
}

bool DebugRenderer::CreateBoneInstancedPipeline() {
    PipelineConfig config{};

#ifdef OZZ_SHADER_BINARY_DIR
    std::filesystem::path shader_root{ OZZ_SHADER_BINARY_DIR };
    config.vertex_shader_path = (shader_root / "bone_instanced.vert.spv").string();
    config.fragment_shader_path = (shader_root / "bone_instanced.frag.spv").string();
#else
    config.vertex_shader_path = "src/xrAnimation/tools/renderer/shaders/bone_instanced.vert.spv";
    config.fragment_shader_path = "src/xrAnimation/tools/renderer/shaders/bone_instanced.frag.spv";
#endif

    // Binding 0: Per-vertex data (position + normal)
    config.vertex_bindings.resize(2);
    config.vertex_bindings[0].binding = 0;
    config.vertex_bindings[0].stride = sizeof(InstanceVertex);
    config.vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Binding 1: Per-instance data (transform + color)
    config.vertex_bindings[1].binding = 1;
    config.vertex_bindings[1].stride = sizeof(InstanceData);
    config.vertex_bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    // Attributes: 0=position, 1=normal, 2-5=transform cols, 6=color
    config.vertex_attributes.resize(7);

    // Per-vertex attributes
    config.vertex_attributes[0].location = 0;
    config.vertex_attributes[0].binding = 0;
    config.vertex_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    config.vertex_attributes[0].offset = offsetof(InstanceVertex, position);

    config.vertex_attributes[1].location = 1;
    config.vertex_attributes[1].binding = 0;
    config.vertex_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    config.vertex_attributes[1].offset = offsetof(InstanceVertex, normal);

    // Per-instance attributes (mat4 transform = 4 vec4s)
    for (int i = 0; i < 4; ++i) {
        config.vertex_attributes[2 + i].location = 2 + i;
        config.vertex_attributes[2 + i].binding = 1;
        config.vertex_attributes[2 + i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        config.vertex_attributes[2 + i].offset = offsetof(InstanceData, transform) + i * sizeof(float) * 4;
    }

    // Per-instance color
    config.vertex_attributes[6].location = 6;
    config.vertex_attributes[6].binding = 1;
    config.vertex_attributes[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    config.vertex_attributes[6].offset = offsetof(InstanceData, color);

    config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.polygon_mode = VK_POLYGON_MODE_FILL;
    config.cull_mode = VK_CULL_MODE_NONE;
    config.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    config.depth_test_enable = true;
    config.depth_write_enable = false;
    config.blend_enable = true;
    config.descriptor_set_layouts.push_back(descriptor_set_layout_);

    return bone_instanced_pipeline_.Create(device_->GetDevice(), config);
}

void DebugRenderer::UpdateDescriptorSet() {
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = uniform_buffer_.GetBuffer();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(ozz::math::Float4x4);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_set_;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device_->GetDevice(), 1, &write, 0, nullptr);
}

void DebugRenderer::UpdateUniforms(const ozz::math::Float4x4& view_proj) {
    // Store matrix in column-major format for GLSL (std140 layout)
    float matrix_data[16];
    for (int col = 0; col < 4; ++col) {
        ozz::math::StorePtrU(view_proj.cols[col], matrix_data + col * 4);
    }
    uniform_buffer_.Upload(matrix_data, sizeof(matrix_data));
}

void DebugRenderer::DrawBoneShapesInstanced(const xr_vector<BoneInstance>& instances) {
    // GPU instancing: push per-instance transform + color data
    // The actual geometry (unit octahedron/sphere) is already on GPU
    static int call_count = 0;
    if (call_count++ < 5) {
        printf("* DrawBoneShapesInstanced (GPU) called with %zu instances (call #%d)\n",
               instances.size(), call_count);
        if (instances.size() > 0) {
            const auto& first = instances[0];
            printf("  First bone: head=(%.3f,%.3f,%.3f) tail=(%.3f,%.3f,%.3f) radius=%.3f\n",
                   first.head.x, first.head.y, first.head.z,
                   first.tail.x, first.tail.y, first.tail.z, first.radius);
        }
    }

    for (const auto& inst : instances) {
        // Compute bone direction and length
        const ozz::math::Float3 bone_vec = Sub(inst.tail, inst.head);
        const float length = std::sqrt(bone_vec.x * bone_vec.x + bone_vec.y * bone_vec.y + bone_vec.z * bone_vec.z);

        if (length <= kEpsilon) {
            continue; // Skip degenerate bones
        }

        const ozz::math::Float3 bone_dir = Scale(bone_vec, 1.0f / length);

        // Create perpendicular vectors for octahedron orientation
        ozz::math::Float3 tangent = Cross(bone_dir, ozz::math::Float3::y_axis());
        if (std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z) <= kEpsilon) {
            tangent = Cross(bone_dir, ozz::math::Float3::x_axis());
        }
        tangent = NormalizeSafe(tangent, ozz::math::Float3::x_axis());
        const ozz::math::Float3 bitangent = NormalizeSafe(Cross(bone_dir, tangent), ozz::math::Float3::z_axis());

        // Mid-point for octahedron
        const ozz::math::Float3 mid = Add(inst.head, Scale(bone_dir, length * 0.5f));

        // Build transform matrix for octahedron
        // Unit octahedron has height 2 (from -1 to +1 on Y), so we scale by length/2
        // and position at mid-point
        InstanceData bone_inst{};

        // Column 0: tangent * radius (X-axis)
        bone_inst.transform[0] = tangent.x * inst.radius;
        bone_inst.transform[1] = tangent.y * inst.radius;
        bone_inst.transform[2] = tangent.z * inst.radius;
        bone_inst.transform[3] = 0.0f;

        // Column 1: bone_dir * length/2 (Y-axis, for height)
        bone_inst.transform[4] = bone_dir.x * length * 0.5f;
        bone_inst.transform[5] = bone_dir.y * length * 0.5f;
        bone_inst.transform[6] = bone_dir.z * length * 0.5f;
        bone_inst.transform[7] = 0.0f;

        // Column 2: bitangent * radius (Z-axis)
        bone_inst.transform[8] = bitangent.x * inst.radius;
        bone_inst.transform[9] = bitangent.y * inst.radius;
        bone_inst.transform[10] = bitangent.z * inst.radius;
        bone_inst.transform[11] = 0.0f;

        // Column 3: translation (mid-point)
        bone_inst.transform[12] = mid.x;
        bone_inst.transform[13] = mid.y;
        bone_inst.transform[14] = mid.z;
        bone_inst.transform[15] = 1.0f;

        bone_inst.color[0] = inst.color.x;
        bone_inst.color[1] = inst.color.y;
        bone_inst.color[2] = inst.color.z;
        bone_inst.color[3] = inst.color.w;

        bone_instances_.push_back(bone_inst);

        // Add sphere instances for head and tail joints
        const float joint_radius = inst.radius * 0.7f;

        // Head sphere
        InstanceData head_sphere{};
        // Identity rotation, scale by joint_radius, translate to head
        head_sphere.transform[0] = joint_radius;
        head_sphere.transform[1] = 0.0f;
        head_sphere.transform[2] = 0.0f;
        head_sphere.transform[3] = 0.0f;

        head_sphere.transform[4] = 0.0f;
        head_sphere.transform[5] = joint_radius;
        head_sphere.transform[6] = 0.0f;
        head_sphere.transform[7] = 0.0f;

        head_sphere.transform[8] = 0.0f;
        head_sphere.transform[9] = 0.0f;
        head_sphere.transform[10] = joint_radius;
        head_sphere.transform[11] = 0.0f;

        head_sphere.transform[12] = inst.head.x;
        head_sphere.transform[13] = inst.head.y;
        head_sphere.transform[14] = inst.head.z;
        head_sphere.transform[15] = 1.0f;

        head_sphere.color[0] = inst.color.x;
        head_sphere.color[1] = inst.color.y;
        head_sphere.color[2] = inst.color.z;
        head_sphere.color[3] = inst.color.w;

        sphere_instances_.push_back(head_sphere);

        // Tail sphere
        InstanceData tail_sphere = head_sphere;
        tail_sphere.transform[12] = inst.tail.x;
        tail_sphere.transform[13] = inst.tail.y;
        tail_sphere.transform[14] = inst.tail.z;

        sphere_instances_.push_back(tail_sphere);
    }

    instance_buffer_dirty_ = true;
}

void DebugRenderer::DrawSpheresInstanced(const xr_vector<SphereInstance>& instances) {
    // Batch generate geometry for all sphere instances
    // This reduces per-sphere overhead by generating all geometry in one pass
    for (const auto& inst : instances) {
        DrawSolidSphere(inst.center, inst.radius, inst.color, 8);
    }
}

} // namespace renderer
} // namespace animation
} // namespace xray
