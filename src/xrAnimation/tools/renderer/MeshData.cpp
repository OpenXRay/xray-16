#include "stdafx.h"
#include "MeshData.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace xray {
namespace animation {
namespace renderer {

namespace {

constexpr float kWeightEpsilon = 1e-6f;

void SetError(xr_string* out_error, const char* message) {
    if (out_error) {
        *out_error = message ? xr_string(message) : xr_string();
    }
}

float SafeNormalizedWeight(float weight) {
    if (!std::isfinite(weight)) {
        return 0.f;
    }
    return std::clamp(weight, 0.f, 1.f);
}

} // namespace

bool BuildMeshUploadData(const ozz::sample::Mesh& mesh, MeshUploadData& out_data, xr_string* out_error) {
    out_data = MeshUploadData{};

    const int vertex_count = mesh.vertex_count();
    if (vertex_count <= 0) {
        SetError(out_error, "mesh contains no vertices");
        return false;
    }

    const size_t total_vertex_count = static_cast<size_t>(vertex_count);
    out_data.vertices.resize(total_vertex_count);

    const size_t triangle_index_count = static_cast<size_t>(mesh.triangle_index_count());
    out_data.indices.resize(triangle_index_count);

    out_data.joint_remaps.assign(mesh.joint_remaps.begin(), mesh.joint_remaps.end());
    out_data.inverse_bind_poses.assign(mesh.inverse_bind_poses.begin(), mesh.inverse_bind_poses.end());
    out_data.metadata = mesh.xray_metadata;
    out_data.skinned = mesh.skinned();

    if (out_data.skinned && out_data.joint_remaps.size() != out_data.inverse_bind_poses.size()) {
        SetError(out_error, "joint remap count does not match inverse bind pose count");
        return false;
    }

    const float positive_infinity = std::numeric_limits<float>::infinity();
    const float negative_infinity = -std::numeric_limits<float>::infinity();
    out_data.bounds.min[0] = out_data.bounds.min[1] = out_data.bounds.min[2] = positive_infinity;
    out_data.bounds.max[0] = out_data.bounds.max[1] = out_data.bounds.max[2] = negative_infinity;

    size_t global_vertex_index = 0;
    xr_vector<std::pair<float, uint16_t>> influence_scratch;
    influence_scratch.reserve(kMaxVertexInfluences * 2);

    for (const ozz::sample::Mesh::Part& part : mesh.parts) {
        const int part_vertex_count = part.vertex_count();
        if (part_vertex_count == 0) {
            continue;
        }

        const size_t expected_positions = static_cast<size_t>(part_vertex_count * ozz::sample::Mesh::Part::kPositionsCpnts);
        if (part.positions.size() != expected_positions) {
            SetError(out_error, "mesh part positions array has unexpected size");
            return false;
        }

        const bool part_has_normals = part.normals.size() == static_cast<size_t>(part_vertex_count * ozz::sample::Mesh::Part::kNormalsCpnts);
        const bool part_has_uvs = part.uvs.size() == static_cast<size_t>(part_vertex_count * ozz::sample::Mesh::Part::kUVsCpnts);
        out_data.has_normals = out_data.has_normals || part_has_normals;
        out_data.has_uvs = out_data.has_uvs || part_has_uvs;

        const int source_influences = part.influences_count();
        if (source_influences < 0) {
            SetError(out_error, "mesh part reported negative influence count");
            return false;
        }

        const size_t expected_joint_indices = static_cast<size_t>(part_vertex_count * std::max(source_influences, 0));
        if (part.joint_indices.size() != expected_joint_indices) {
            SetError(out_error, "mesh part joint indices array has unexpected size");
            return false;
        }

        const size_t expected_joint_weights = source_influences > 0
            ? static_cast<size_t>(part_vertex_count * (source_influences - 1))
            : 0u;
        if (part.joint_weights.size() != expected_joint_weights) {
            SetError(out_error, "mesh part joint weights array has unexpected size");
            return false;
        }

        for (int local_vertex = 0; local_vertex < part_vertex_count; ++local_vertex, ++global_vertex_index) {
            if (global_vertex_index >= total_vertex_count) {
                SetError(out_error, "mesh part vertex count exceeds aggregated vertex count");
                return false;
            }

            SkinnedMeshVertex& vertex = out_data.vertices[global_vertex_index];

            const size_t position_offset = static_cast<size_t>(local_vertex * ozz::sample::Mesh::Part::kPositionsCpnts);
            vertex.position[0] = part.positions[position_offset + 0];
            vertex.position[1] = part.positions[position_offset + 1];
            vertex.position[2] = part.positions[position_offset + 2];

            out_data.bounds.min[0] = std::min(out_data.bounds.min[0], vertex.position[0]);
            out_data.bounds.min[1] = std::min(out_data.bounds.min[1], vertex.position[1]);
            out_data.bounds.min[2] = std::min(out_data.bounds.min[2], vertex.position[2]);
            out_data.bounds.max[0] = std::max(out_data.bounds.max[0], vertex.position[0]);
            out_data.bounds.max[1] = std::max(out_data.bounds.max[1], vertex.position[1]);
            out_data.bounds.max[2] = std::max(out_data.bounds.max[2], vertex.position[2]);

            if (part_has_normals) {
                const size_t normal_offset = static_cast<size_t>(local_vertex * ozz::sample::Mesh::Part::kNormalsCpnts);
                vertex.normal[0] = part.normals[normal_offset + 0];
                vertex.normal[1] = part.normals[normal_offset + 1];
                vertex.normal[2] = part.normals[normal_offset + 2];
            } else {
                vertex.normal[0] = 0.f;
                vertex.normal[1] = 1.f;
                vertex.normal[2] = 0.f;
            }

            if (part_has_uvs) {
                const size_t uv_offset = static_cast<size_t>(local_vertex * ozz::sample::Mesh::Part::kUVsCpnts);
                vertex.uv[0] = part.uvs[uv_offset + 0];
                vertex.uv[1] = part.uvs[uv_offset + 1];
            } else {
                vertex.uv[0] = 0.f;
                vertex.uv[1] = 0.f;
            }

            influence_scratch.clear();

            if (source_influences <= 0 || !out_data.skinned) {
                influence_scratch.emplace_back(1.f, uint16_t{0});
            } else {
                const size_t joint_index_base = static_cast<size_t>(local_vertex * source_influences);
                const size_t weight_base = static_cast<size_t>(local_vertex * std::max(source_influences - 1, 0));

                float accumulated_weight = 0.f;
                for (int influence = 0; influence < source_influences; ++influence) {
                    if (joint_index_base + influence >= part.joint_indices.size()) {
                        SetError(out_error, "mesh joint index buffer access out of bounds");
                        return false;
                    }

                    uint16_t joint = part.joint_indices[joint_index_base + influence];
                    if (joint >= out_data.joint_remaps.size() && out_data.skinned) {
                        SetError(out_error, "joint index exceeds remap table range");
                        return false;
                    }

                    float weight = 0.f;
                    if (influence < source_influences - 1) {
                        if (weight_base + influence >= part.joint_weights.size()) {
                            SetError(out_error, "mesh joint weight buffer access out of bounds");
                            return false;
                        }
                        weight = part.joint_weights[weight_base + influence];
                    } else {
                        weight = 1.f - accumulated_weight;
                    }

                    weight = SafeNormalizedWeight(weight);
                    accumulated_weight += weight;

                    if (weight > kWeightEpsilon) {
                        influence_scratch.emplace_back(weight, joint);
                    }
                }
            }

            if (influence_scratch.empty()) {
                influence_scratch.emplace_back(1.f, uint16_t{0});
            }

            if (influence_scratch.size() > kMaxVertexInfluences) {
                std::partial_sort(influence_scratch.begin(),
                    influence_scratch.begin() + kMaxVertexInfluences,
                    influence_scratch.end(),
                    [](const std::pair<float, uint16_t>& a, const std::pair<float, uint16_t>& b) {
                        return a.first > b.first;
                    });
                influence_scratch.resize(kMaxVertexInfluences);
            }

            float weight_sum = 0.f;
            for (const auto& entry : influence_scratch) {
                weight_sum += entry.first;
            }
            if (weight_sum <= kWeightEpsilon) {
                influence_scratch.clear();
                influence_scratch.emplace_back(1.f, uint16_t{0});
                weight_sum = 1.f;
            }

            const float inv_sum = 1.f / weight_sum;
            size_t influence_index = 0;
            for (; influence_index < influence_scratch.size(); ++influence_index) {
                const float normalized_weight = SafeNormalizedWeight(influence_scratch[influence_index].first * inv_sum);
                vertex.weights[influence_index] = normalized_weight;
                vertex.joints[influence_index] = influence_scratch[influence_index].second;
            }

            for (; influence_index < kMaxVertexInfluences; ++influence_index) {
                vertex.weights[influence_index] = 0.f;
                vertex.joints[influence_index] = 0;
            }

            out_data.max_influences = std::max<uint32_t>(
                out_data.max_influences,
                static_cast<uint32_t>(influence_scratch.size()));
        }
    }

    if (global_vertex_index != total_vertex_count) {
        SetError(out_error, "aggregated vertex count mismatch after conversion");
        return false;
    }

    for (size_t index = 0; index < triangle_index_count; ++index) {
        const uint16_t source_index = mesh.triangle_indices[index];
        if (source_index >= total_vertex_count) {
            SetError(out_error, "mesh triangle index references out-of-range vertex");
            return false;
        }
        out_data.indices[index] = static_cast<uint32_t>(source_index);
    }

    if (!out_data.skinned) {
        out_data.max_influences = 0;
    }

    return true;
}

} // namespace renderer
} // namespace animation
} // namespace xray
