#pragma once

#include <nvrhi/nvrhi.h>
#include "FVF.h"

namespace xray::render::fg
{

class FGDebugDraw
{
public:
    struct Vertex
    {
        float x, y, z;
        u32   color;
    };

    FGDebugDraw() = default;
    ~FGDebugDraw() = default;

    bool HasWork() const { return !m_lineVerts.empty() || !m_triVerts.empty(); }
    void Clear();

    void AddLine(const Fvector& p1, const Fvector& p2, u32 color);
    void AddLines(const FVF::L* verts, u32 count);
    void AddIndexedLines(const FVF::L* verts, u32 vertexCount, const u16* indices, u32 indexCount);
    void AddTriangle(const Fvector& p1, const Fvector& p2, const Fvector& p3, u32 color);
    void AddTriangles(const FVF::L* verts, u32 count);
    void AddPrimitive(nvrhi::PrimitiveType prim, const FVF::L* verts, u32 count);
    void AddPrimitive(nvrhi::PrimitiveType prim, const FVF::LIT* verts, u32 count);

    void DrawLine(const Fmatrix& T, const Fvector& p1, const Fvector& p2, u32 color);
    void DrawTRI(const Fmatrix& T, const Fvector& p1, const Fvector& p2, const Fvector& p3, u32 color);
    void DrawOBB(const Fmatrix& T, const Fvector& halfdim, u32 color);
    void DrawEllipse(const Fmatrix& T, u32 color);

    void Render(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, const Fmatrix& viewProj);

private:
    bool EnsurePipelines(nvrhi::IDevice* device, nvrhi::IFramebuffer* framebuffer);
    void EnsureLineCapacity(nvrhi::IDevice* device, size_t vertexCount);
    void EnsureTriCapacity(nvrhi::IDevice* device, size_t vertexCount);

    xr_vector<Vertex> m_lineVerts;
    xr_vector<Vertex> m_triVerts;

    nvrhi::ShaderHandle           m_vs;
    nvrhi::ShaderHandle           m_ps;
    nvrhi::InputLayoutHandle      m_inputLayout;
    nvrhi::BindingLayoutHandle    m_bindingLayout;
    nvrhi::BindingSetHandle       m_bindingSet;
    nvrhi::BufferHandle           m_constantBuffer;
    nvrhi::BufferHandle           m_lineVB;
    nvrhi::BufferHandle           m_triVB;
    size_t                        m_lineCapacity = 0;
    size_t                        m_triCapacity = 0;
    nvrhi::GraphicsPipelineHandle m_pipelineLine;
    nvrhi::GraphicsPipelineHandle m_pipelineTri;
};

extern FGDebugDraw g_debug_draw;

}
