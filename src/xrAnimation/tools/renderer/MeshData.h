#pragma once

#include "Common/Platform.hpp"

// Define XRCORE_API for linking against xrCore.lib (not building it)
#ifndef XRCORE_API
#  define XRCORE_API XR_IMPORT
#  define TRACY_IMPORTS
#endif

#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_string.h"

#include <cstdint>

#include "framework/mesh.h"

namespace ozz {
namespace math {
struct Float4x4;
} // namespace math
} // namespace ozz

namespace xray {
namespace animation {
namespace renderer {

// Maximum number of bone influences supported per vertex by the GPU pipeline.
constexpr uint32_t kMaxVertexInfluences = 4;

// Interleaved vertex format used by the Vulkan skinned mesh renderer.
struct SkinnedMeshVertex {
    float position[3];
    float normal[3];
    float uv[2];
    float weights[kMaxVertexInfluences];
    uint16_t joints[kMaxVertexInfluences];
};

// Axis-aligned bounding box for converted mesh vertices.
struct MeshBounds {
    float min[3];
    float max[3];
};

// GPU-ready mesh payload built from an ozz::sample::Mesh surface.
struct MeshUploadData {
    xr_vector<SkinnedMeshVertex> vertices;
    xr_vector<uint32_t> indices;
    xr_vector<uint16_t> joint_remaps;
    xr_vector<ozz::math::Float4x4> inverse_bind_poses;
    ozz::sample::XRayMeshMetadata metadata;

    MeshBounds bounds{};
    uint32_t max_influences = 0;
    bool skinned = false;
    bool has_normals = false;
    bool has_uvs = false;
};

// Converts an ozz::sample::Mesh into GPU-ready data for the Vulkan renderer.
// Returns true on success; on failure, out_error (if provided) contains
// a descriptive message.
bool BuildMeshUploadData(const ozz::sample::Mesh& mesh, MeshUploadData& out_data, xr_string* out_error = nullptr);

} // namespace renderer
} // namespace animation
} // namespace xray
