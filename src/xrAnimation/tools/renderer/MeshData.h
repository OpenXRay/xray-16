#pragma once

#include "Common/Platform.hpp"

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
}
}

namespace xray {
namespace animation {
namespace renderer {

constexpr uint32_t kMaxVertexInfluences = 4;

struct SkinnedMeshVertex {
    float position[3];
    float normal[3];
    float uv[2];
    float weights[kMaxVertexInfluences];
    uint16_t joints[kMaxVertexInfluences];
};

struct MeshBounds {
    float min[3];
    float max[3];
};

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

bool BuildMeshUploadData(const ozz::sample::Mesh& mesh, MeshUploadData& out_data, xr_string* out_error = nullptr);

}
}
}
