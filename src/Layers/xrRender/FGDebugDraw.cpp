#include "stdafx.h"

#include "FGDebugDraw.h"
#include "r_FrameGraphRenderer.h"
#include "FrameGraph/ShaderLoader.h"
#include "RenderContext/RenderDevice.h"

namespace xray::render::fg
{
FGDebugDraw g_debug_draw;

namespace
{
constexpr size_t kCBSize = 256;

struct DebugWorldCB
{
    float viewProj[16];
};

void push_vertex(xr_vector<FGDebugDraw::Vertex>& dst, const Fvector& p, u32 color)
{
    FGDebugDraw::Vertex v{ p.x, p.y, p.z, color };
    dst.push_back(v);
}
}

void FGDebugDraw::Clear()
{
    m_lineVerts.clear();
    m_triVerts.clear();
}

void FGDebugDraw::AddLine(const Fvector& p1, const Fvector& p2, u32 color)
{
    push_vertex(m_lineVerts, p1, color);
    push_vertex(m_lineVerts, p2, color);
}

void FGDebugDraw::AddLines(const FVF::L* verts, u32 count)
{
    m_lineVerts.reserve(m_lineVerts.size() + count);
    for (u32 i = 0; i < count; ++i)
        push_vertex(m_lineVerts, verts[i].p, verts[i].color);
}

void FGDebugDraw::AddIndexedLines(const FVF::L* verts, u32 vertexCount, const u16* indices, u32 indexCount)
{
    m_lineVerts.reserve(m_lineVerts.size() + indexCount);
    for (u32 i = 0; i < indexCount; ++i)
    {
        const u16 idx = indices[i];
        VERIFY(idx < vertexCount);
        push_vertex(m_lineVerts, verts[idx].p, verts[idx].color);
    }
}

void FGDebugDraw::AddTriangle(const Fvector& p1, const Fvector& p2, const Fvector& p3, u32 color)
{
    push_vertex(m_triVerts, p1, color);
    push_vertex(m_triVerts, p2, color);
    push_vertex(m_triVerts, p3, color);
}

void FGDebugDraw::AddTriangles(const FVF::L* verts, u32 count)
{
    m_triVerts.reserve(m_triVerts.size() + count);
    for (u32 i = 0; i < count; ++i)
        push_vertex(m_triVerts, verts[i].p, verts[i].color);
}

void FGDebugDraw::AddPrimitive(nvrhi::PrimitiveType prim, const FVF::L* verts, u32 count)
{
    switch (prim)
    {
    case nvrhi::PrimitiveType::LineList:
        AddLines(verts, count * 2);
        break;
    case nvrhi::PrimitiveType::LineStrip:
        if (count == 0) return;
        m_lineVerts.reserve(m_lineVerts.size() + count * 2);
        for (u32 i = 0; i < count; ++i)
        {
            push_vertex(m_lineVerts, verts[i].p, verts[i].color);
            push_vertex(m_lineVerts, verts[i + 1].p, verts[i + 1].color);
        }
        break;
    case nvrhi::PrimitiveType::TriangleList:
        AddTriangles(verts, count * 3);
        break;
    case nvrhi::PrimitiveType::TriangleFan:
        if (count == 0) return;
        m_triVerts.reserve(m_triVerts.size() + count * 3);
        for (u32 i = 0; i < count; ++i)
        {
            push_vertex(m_triVerts, verts[0].p, verts[0].color);
            push_vertex(m_triVerts, verts[i + 1].p, verts[i + 1].color);
            push_vertex(m_triVerts, verts[i + 2].p, verts[i + 2].color);
        }
        break;
    case nvrhi::PrimitiveType::TriangleStrip:
        if (count == 0) return;
        m_triVerts.reserve(m_triVerts.size() + count * 3);
        for (u32 i = 0; i < count; ++i)
        {
            const u32 a = (i & 1) ? i + 1 : i;
            const u32 b = (i & 1) ? i : i + 1;
            const u32 c = i + 2;
            push_vertex(m_triVerts, verts[a].p, verts[a].color);
            push_vertex(m_triVerts, verts[b].p, verts[b].color);
            push_vertex(m_triVerts, verts[c].p, verts[c].color);
        }
        break;
    default:
        break;
    }
}

void FGDebugDraw::AddPrimitive(nvrhi::PrimitiveType prim, const FVF::LIT* verts, u32 count)
{
    auto vertCount = [&](nvrhi::PrimitiveType p, u32 c) -> u32 {
        switch (p)
        {
        case nvrhi::PrimitiveType::LineList:      return c * 2;
        case nvrhi::PrimitiveType::LineStrip:     return c + 1;
        case nvrhi::PrimitiveType::TriangleList:  return c * 3;
        case nvrhi::PrimitiveType::TriangleFan:   return c + 2;
        case nvrhi::PrimitiveType::TriangleStrip: return c + 2;
        default: return c;
        }
    }(prim, count);
    xr_vector<FVF::L> tmp(vertCount);
    for (u32 i = 0; i < vertCount; ++i)
    {
        tmp[i].p = verts[i].p;
        tmp[i].color = verts[i].color;
    }
    AddPrimitive(prim, tmp.data(), count);
}

void FGDebugDraw::DrawLine(const Fmatrix& T, const Fvector& p1, const Fvector& p2, u32 color)
{
    Fvector q1, q2;
    T.transform_tiny(q1, p1);
    T.transform_tiny(q2, p2);
    AddLine(q1, q2, color);
}

void FGDebugDraw::DrawTRI(const Fmatrix& T, const Fvector& p1, const Fvector& p2, const Fvector& p3, u32 color)
{
    Fvector q1, q2, q3;
    T.transform_tiny(q1, p1);
    T.transform_tiny(q2, p2);
    T.transform_tiny(q3, p3);
    AddLine(q1, q2, color);
    AddLine(q2, q3, color);
    AddLine(q3, q1, color);
}

void FGDebugDraw::DrawOBB(const Fmatrix& T, const Fvector& halfdim, u32 color)
{
    Fvector p[8];
    p[0].set(-halfdim.x, -halfdim.y, -halfdim.z);
    p[1].set(+halfdim.x, -halfdim.y, -halfdim.z);
    p[2].set(+halfdim.x, +halfdim.y, -halfdim.z);
    p[3].set(-halfdim.x, +halfdim.y, -halfdim.z);
    p[4].set(-halfdim.x, -halfdim.y, +halfdim.z);
    p[5].set(+halfdim.x, -halfdim.y, +halfdim.z);
    p[6].set(+halfdim.x, +halfdim.y, +halfdim.z);
    p[7].set(-halfdim.x, +halfdim.y, +halfdim.z);
    Fvector w[8];
    for (int i = 0; i < 8; ++i)
        T.transform_tiny(w[i], p[i]);
    static const u16 edges[24] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7,
    };
    for (int i = 0; i < 24; i += 2)
        AddLine(w[edges[i]], w[edges[i + 1]], color);
}

void FGDebugDraw::DrawEllipse(const Fmatrix& T, u32 color)
{
    constexpr int SEGMENTS = 24;
    auto sample = [&](float u, float v, float w) -> Fvector {
        Fvector local{ u, v, w };
        Fvector world;
        T.transform_tiny(world, local);
        return world;
    };
    Fvector prev = sample(1.f, 0.f, 0.f);
    for (int i = 1; i <= SEGMENTS; ++i)
    {
        const float a = float(i) * 2.f * PI / float(SEGMENTS);
        Fvector cur = sample(_cos(a), _sin(a), 0);
        AddLine(prev, cur, color);
        prev = cur;
    }
    prev = sample(0.f, 1.f, 0.f);
    for (int i = 1; i <= SEGMENTS; ++i)
    {
        const float a = float(i) * 2.f * PI / float(SEGMENTS);
        Fvector cur = sample(0, _cos(a), _sin(a));
        AddLine(prev, cur, color);
        prev = cur;
    }
    prev = sample(1.f, 0.f, 0.f);
    for (int i = 1; i <= SEGMENTS; ++i)
    {
        const float a = float(i) * 2.f * PI / float(SEGMENTS);
        Fvector cur = sample(_cos(a), 0, _sin(a));
        AddLine(prev, cur, color);
        prev = cur;
    }
}

bool FGDebugDraw::EnsurePipelines(nvrhi::IDevice* device, nvrhi::IFramebuffer* framebuffer)
{
    if (m_pipelineLine && m_pipelineTri)
        return true;

    auto* shaderLoader = RImplementation.GetShaderLoader();
    if (!shaderLoader)
        return false;

    if (!m_vs)
    {
        auto vs = shaderLoader->LoadVertexShader("debug_world_color", "main");
        if (!vs.handle)
            return false;
        m_vs = vs.handle;
    }
    if (!m_ps)
    {
        auto ps = shaderLoader->LoadPixelShader("debug_world_color", "main");
        if (!ps.handle)
            return false;
        m_ps = ps.handle;
    }
    if (!m_inputLayout)
    {
        nvrhi::VertexAttributeDesc attrs[] = {
            nvrhi::VertexAttributeDesc()
                .setName("POSITION")
                .setFormat(nvrhi::Format::RGB32_FLOAT)
                .setOffset(0)
                .setElementStride(sizeof(Vertex)),
            nvrhi::VertexAttributeDesc()
                .setName("COLOR")
                .setFormat(nvrhi::Format::RGBA8_UNORM)
                .setOffset(12)
                .setElementStride(sizeof(Vertex)),
        };
        m_inputLayout = device->createInputLayout(attrs, 2, m_vs);
        if (!m_inputLayout)
            return false;
    }
    if (!m_constantBuffer)
    {
        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = kCBSize;
        cbDesc.isConstantBuffer = true;
        cbDesc.debugName = "FGDebugDraw_CB";
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        cbDesc.keepInitialState = true;
        m_constantBuffer = device->createBuffer(cbDesc);
        if (!m_constantBuffer)
            return false;
    }
    if (!m_bindingLayout)
    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::All;
        layoutDesc.bindings = { nvrhi::BindingLayoutItem::ConstantBuffer(0) };
        m_bindingLayout = device->createBindingLayout(layoutDesc);
        if (!m_bindingLayout)
            return false;
    }
    if (!m_bindingSet)
    {
        nvrhi::BindingSetDesc bsDesc;
        bsDesc.bindings = { nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer) };
        m_bindingSet = device->createBindingSet(bsDesc, m_bindingLayout);
        if (!m_bindingSet)
            return false;
    }

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = m_vs;
    pipeDesc.PS = m_ps;
    pipeDesc.inputLayout = m_inputLayout;
    pipeDesc.bindingLayouts = { m_bindingLayout };
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    pipeDesc.renderState.depthStencilState.depthTestEnable = false;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipeDesc.renderState.blendState.targets[0]
        .setBlendEnable(true)
        .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
        .setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
        .setSrcBlendAlpha(nvrhi::BlendFactor::One)
        .setDestBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha);

    if (!m_pipelineLine)
    {
        pipeDesc.primType = nvrhi::PrimitiveType::LineList;
        m_pipelineLine = device->createGraphicsPipeline(pipeDesc, framebuffer);
        if (!m_pipelineLine)
            return false;
    }
    if (!m_pipelineTri)
    {
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        m_pipelineTri = device->createGraphicsPipeline(pipeDesc, framebuffer);
        if (!m_pipelineTri)
            return false;
    }
    return true;
}

void FGDebugDraw::EnsureLineCapacity(nvrhi::IDevice* device, size_t vertexCount)
{
    if (vertexCount <= m_lineCapacity && m_lineVB)
        return;
    const size_t newCapacity = std::max<size_t>(vertexCount * 2, 1024);
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = newCapacity * sizeof(Vertex);
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "FGDebugDraw_LineVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    m_lineVB = device->createBuffer(vbDesc);
    m_lineCapacity = newCapacity;
}

void FGDebugDraw::EnsureTriCapacity(nvrhi::IDevice* device, size_t vertexCount)
{
    if (vertexCount <= m_triCapacity && m_triVB)
        return;
    const size_t newCapacity = std::max<size_t>(vertexCount * 2, 1024);
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = newCapacity * sizeof(Vertex);
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "FGDebugDraw_TriVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    m_triVB = device->createBuffer(vbDesc);
    m_triCapacity = newCapacity;
}

void FGDebugDraw::Render(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, const Fmatrix& viewProj)
{
    if (!HasWork() || !cmdList || !framebuffer)
    {
        Clear();
        return;
    }
    nvrhi::IDevice* device = cmdList->getDevice();
    if (!EnsurePipelines(device, framebuffer))
    {
        Clear();
        return;
    }

    DebugWorldCB cb{};
    std::memcpy(cb.viewProj, &viewProj, sizeof(cb.viewProj));
    u8 cbData[kCBSize] = {};
    std::memcpy(cbData, &cb, sizeof(cb));
    cmdList->writeBuffer(m_constantBuffer, cbData, kCBSize);
    cmdList->setBufferState(m_constantBuffer, nvrhi::ResourceStates::ConstantBuffer);

    const auto& fbInfo = framebuffer->getFramebufferInfo();
    const float fbW = static_cast<float>(fbInfo.width);
    const float fbH = static_cast<float>(fbInfo.height);

    if (!m_lineVerts.empty())
    {
        EnsureLineCapacity(device, m_lineVerts.size());
        cmdList->writeBuffer(m_lineVB, m_lineVerts.data(), m_lineVerts.size() * sizeof(Vertex));
        cmdList->setBufferState(m_lineVB, nvrhi::ResourceStates::VertexBuffer);

        nvrhi::GraphicsState state;
        state.pipeline = m_pipelineLine;
        state.framebuffer = framebuffer;
        state.bindings = { m_bindingSet };
        nvrhi::VertexBufferBinding vbb;
        vbb.buffer = m_lineVB;
        vbb.slot = 0;
        vbb.offset = 0;
        state.vertexBuffers = { vbb };
        state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(nvrhi::Viewport(fbW, fbH));
        cmdList->setGraphicsState(state);

        nvrhi::DrawArguments args;
        args.vertexCount = static_cast<u32>(m_lineVerts.size());
        args.instanceCount = 1;
        args.startVertexLocation = 0;
        cmdList->draw(args);
    }

    if (!m_triVerts.empty())
    {
        EnsureTriCapacity(device, m_triVerts.size());
        cmdList->writeBuffer(m_triVB, m_triVerts.data(), m_triVerts.size() * sizeof(Vertex));
        cmdList->setBufferState(m_triVB, nvrhi::ResourceStates::VertexBuffer);

        nvrhi::GraphicsState state;
        state.pipeline = m_pipelineTri;
        state.framebuffer = framebuffer;
        state.bindings = { m_bindingSet };
        nvrhi::VertexBufferBinding vbb;
        vbb.buffer = m_triVB;
        vbb.slot = 0;
        vbb.offset = 0;
        state.vertexBuffers = { vbb };
        state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(nvrhi::Viewport(fbW, fbH));
        cmdList->setGraphicsState(state);

        nvrhi::DrawArguments args;
        args.vertexCount = static_cast<u32>(m_triVerts.size());
        args.instanceCount = 1;
        args.startVertexLocation = 0;
        cmdList->draw(args);
    }

    Clear();
}

}
