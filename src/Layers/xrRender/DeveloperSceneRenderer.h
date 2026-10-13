#pragma once

#include "Layers/xrRender/VertexLayout.h"
#include "Layers/xrRender/r__buffer_pool.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"
#include "xrEngine/DeveloperScene.h"

namespace xray::render::fg
{
class DeveloperSceneRenderer
{
public:
    class VertexData
    {
    public:
        float position[3];
        float normal[3];
        float uv[2];
    };

    static constexpr u32 VERTEX_STRIDE = 32;
    static constexpr u32 MAX_POOL_VERTICES = 65535;
    static constexpr u32 NAME_LIMIT = 32;

    static void BuildVertexDeclaration(VertexDeclarator& declaration);
    static void ComputeMeshBounds(const xray::render::DeveloperSceneMesh& mesh, Fbox& box, Fvector& center, float& radius);
    static xr_string SanitizeName(const xr_string& name);
    static u64 HashScene(const xr_string& sceneName, const Fbox& bounds,
        const xr_vector<xray::render::DeveloperSceneMaterial>& materials,
        const xr_vector<xray::render::DeveloperSceneMesh>& meshes);

private:
    static u64 FoldHash(u64 hash, const void* data, size_t size);
};

static_assert(sizeof(DeveloperSceneRenderer::VertexData) == DeveloperSceneRenderer::VERTEX_STRIDE,
    "developer vertex layout must match its declaration");
}
