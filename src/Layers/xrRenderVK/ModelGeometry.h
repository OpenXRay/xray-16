#pragma once

#include "VisualCatalog.h"
#include "LevelModels.h"

namespace xray::render::vulkan
{
// OGF mesh data independent of the rendering API. Skinning weights use the
// engine's 1W/2W/3W/4W conventions; a skeleton pose is supplied separately.
struct ModelVertex
{
    float position[3]{};
    float normal[3]{};
    float uv[2]{};
    uint16_t bones[4]{};
    float weights[4]{1, 0, 0, 0};
};

struct ModelGeometry
{
    uint8_t type{};
    std::string shader, texture;
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<ModelGeometry> children;
};

// Decodes embedded static/progressive and skinned child meshes of animated
// and rigid skeletons. External motion references and bone transforms belong
// to IKinematicsAnimated and are deliberately not inferred from geometry.
bool decode_model_geometry(const VisualRecord& source, ModelGeometry& result,
    std::string& error);

// Matrices have the engine's Fmatrix layout (i, j, k, c columns in memory).
// Their render transforms already include the inverse bind pose. The caller
// owns pose calculation and waits for any GPU use before replacing vertices.
bool skin_model_mesh(const ModelGeometry& source, const float* matrices,
    size_t bone_count, std::vector<LevelVertex>& result, std::string& error);
}
