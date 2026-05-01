#include "stdafx.h"
#include "fgStatGraphRender.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::fg
{
namespace
{
    constexpr size_t kCBSize = 256;

    struct DebugScreenCB
    {
        float invHalfScreen[2];
        float pad[2];
    };
}

FGStatGraphRender::FGStatGraphRender() = default;
FGStatGraphRender::~FGStatGraphRender() = default;

void FGStatGraphRender::Copy(IStatGraphRender& _in)
{
    auto& other = static_cast<FGStatGraphRender&>(_in);
    m_vertices = other.m_vertices;
    m_drawItems = other.m_drawItems;
}

void FGStatGraphRender::OnDeviceCreate() {}
void FGStatGraphRender::OnDeviceDestroy()
{
    Clear();
    m_pipelineTri = nullptr;
    m_pipelineLine = nullptr;
    m_bindingSet = nullptr;
    m_bindingLayout = nullptr;
    m_inputLayout = nullptr;
    m_constantBuffer = nullptr;
    m_vertexBuffer = nullptr;
    m_vertexBufferCapacity = 0;
    m_vs = nullptr;
    m_ps = nullptr;
}

bool FGStatGraphRender::EnsurePipelines(nvrhi::IDevice* device, nvrhi::IFramebuffer* framebuffer)
{
    if (m_pipelineTri && m_pipelineLine)
        return true;

    auto* shaderLoader = RImplementation.GetShaderLoader();
    if (!shaderLoader)
        return false;

    if (!m_vs)
    {
        auto vs = shaderLoader->LoadVertexShader("debug_screen_color", "main");
        if (!vs.handle)
            return false;
        m_vs = vs.handle;
    }
    if (!m_ps)
    {
        auto ps = shaderLoader->LoadPixelShader("debug_screen_color", "main");
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
        cbDesc.debugName = "FGStatGraphRender_CB";
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
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::ConstantBuffer(0),
        };
        m_bindingLayout = device->createBindingLayout(layoutDesc);
        if (!m_bindingLayout)
            return false;
    }

    if (!m_bindingSet)
    {
        nvrhi::BindingSetDesc bsDesc;
        bsDesc.bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
        };
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

    if (!m_pipelineTri)
    {
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        m_pipelineTri = device->createGraphicsPipeline(pipeDesc, framebuffer);
        if (!m_pipelineTri)
            return false;
    }
    if (!m_pipelineLine)
    {
        pipeDesc.primType = nvrhi::PrimitiveType::LineList;
        m_pipelineLine = device->createGraphicsPipeline(pipeDesc, framebuffer);
        if (!m_pipelineLine)
            return false;
    }
    return true;
}

void FGStatGraphRender::EnsureGeometryCapacity(nvrhi::IDevice* device, size_t vertexCount)
{
    if (vertexCount <= m_vertexBufferCapacity && m_vertexBuffer)
        return;
    const size_t newCapacity = std::max<size_t>(vertexCount * 2, 1024);
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = newCapacity * sizeof(Vertex);
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "FGStatGraphRender_VB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    m_vertexBuffer = device->createBuffer(vbDesc);
    m_vertexBufferCapacity = newCapacity;
}

nvrhi::GraphicsPipelineHandle FGStatGraphRender::GetPipeline(nvrhi::PrimitiveType topology, nvrhi::IDevice*, nvrhi::IFramebuffer*)
{
    return topology == nvrhi::PrimitiveType::LineList ? m_pipelineLine : m_pipelineTri;
}

void FGStatGraphRender::Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, u32 screenWidth, u32 screenHeight)
{
    if (m_drawItems.empty() || m_vertices.empty() || !cmdList || !framebuffer)
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
    EnsureGeometryCapacity(device, m_vertices.size());

    cmdList->writeBuffer(m_vertexBuffer, m_vertices.data(), m_vertices.size() * sizeof(Vertex));

    DebugScreenCB cb{};
    cb.invHalfScreen[0] = 2.0f / static_cast<float>(screenWidth);
    cb.invHalfScreen[1] = 2.0f / static_cast<float>(screenHeight);
    u8 cbData[kCBSize] = {};
    std::memcpy(cbData, &cb, sizeof(cb));
    cmdList->writeBuffer(m_constantBuffer, cbData, kCBSize);

    cmdList->setBufferState(m_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);
    cmdList->setBufferState(m_constantBuffer, nvrhi::ResourceStates::ConstantBuffer);

    for (const DrawItem& item : m_drawItems)
    {
        if (item.vertexCount == 0) continue;

        nvrhi::GraphicsState state;
        state.pipeline = GetPipeline(item.topology, device, framebuffer);
        state.framebuffer = framebuffer;
        state.bindings = { m_bindingSet };
        nvrhi::VertexBufferBinding vbb;
        vbb.buffer = m_vertexBuffer;
        vbb.slot = 0;
        vbb.offset = 0;
        state.vertexBuffers = { vbb };
        state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(
            nvrhi::Viewport(static_cast<float>(screenWidth), static_cast<float>(screenHeight)));
        cmdList->setGraphicsState(state);

        nvrhi::DrawArguments args;
        args.vertexCount = item.vertexCount;
        args.instanceCount = 1;
        args.startVertexLocation = item.vertexOffset;
        cmdList->draw(args);
    }
    Clear();
}


void FGStatGraphRender::Clear()
{
    m_vertices.clear();
    m_drawItems.clear();
    m_inItem = false;
}

void FGStatGraphRender::Begin(nvrhi::PrimitiveType topology)
{
    if (m_inItem)
    {
        m_currentItem.vertexCount = static_cast<u32>(m_vertices.size()) - m_currentItem.vertexOffset;
        if (m_currentItem.vertexCount > 0)
            m_drawItems.push_back(m_currentItem);
    }
    m_currentItem.topology     = topology;
    m_currentItem.vertexOffset = static_cast<u32>(m_vertices.size());
    m_currentItem.vertexCount  = 0;
    m_inItem = true;
}

void FGStatGraphRender::Push(float x, float y, u32 color)
{
    m_vertices.push_back({x, y, 0.0f, color});
}

void FGStatGraphRender::OnRender(CStatGraph& owner)
{
    Clear();

    RenderBack(owner);

    Begin(nvrhi::PrimitiveType::TriangleList);
    for (auto& sg : owner.subgraphs)
    {
        if (sg.style == CStatGraph::stBar)
            RenderBars(owner, sg.elements);
    }

    Begin(nvrhi::PrimitiveType::LineList);
    for (auto& sg : owner.subgraphs)
    {
        if (sg.style == CStatGraph::stCurve)
            RenderLines(owner, sg.elements);
        else if (sg.style == CStatGraph::stBarLine)
            RenderBarLines(owner, sg.elements);
    }

    if (!owner.m_Markers.empty())
    {
        Begin(nvrhi::PrimitiveType::LineList);
        RenderMarkers(owner, owner.m_Markers);
    }

    if (m_inItem)
    {
        m_currentItem.vertexCount = static_cast<u32>(m_vertices.size()) - m_currentItem.vertexOffset;
        if (m_currentItem.vertexCount > 0)
            m_drawItems.push_back(m_currentItem);
        m_inItem = false;
    }
}

void FGStatGraphRender::RenderBack(CStatGraph& owner)
{
    Begin(nvrhi::PrimitiveType::TriangleList);
    Push(static_cast<float>(owner.lt.x), static_cast<float>(owner.rb.y), owner.back_color);
    Push(static_cast<float>(owner.lt.x), static_cast<float>(owner.lt.y), owner.back_color);
    Push(static_cast<float>(owner.rb.x), static_cast<float>(owner.rb.y), owner.back_color);
    Push(static_cast<float>(owner.lt.x), static_cast<float>(owner.lt.y), owner.back_color);
    Push(static_cast<float>(owner.rb.x), static_cast<float>(owner.lt.y), owner.back_color);
    Push(static_cast<float>(owner.rb.x), static_cast<float>(owner.rb.y), owner.back_color);

    Begin(nvrhi::PrimitiveType::LineList);
    auto edge = [&](float x0, float y0, float x1, float y1, u32 c)
    {
        Push(x0, y0, c);
        Push(x1, y1, c);
    };
    const float lt_x = static_cast<float>(owner.lt.x);
    const float lt_y = static_cast<float>(owner.lt.y);
    const float rb_x = static_cast<float>(owner.rb.x - 1);
    const float rb_y = static_cast<float>(owner.rb.y);
    edge(lt_x, lt_y, rb_x, lt_y, owner.rect_color);
    edge(rb_x, lt_y, rb_x, rb_y, owner.rect_color);
    edge(rb_x, rb_y, lt_x, rb_y, owner.rect_color);
    edge(lt_x, rb_y, lt_x, lt_y, owner.rect_color);

    const float elem_factor = float(owner.rb.y - owner.lt.y) / float(owner.mx - owner.mn);
    const float base_y      = float(owner.rb.y) + (owner.mn * elem_factor);

    int p_up   = int((base_y - float(owner.lt.y)) / (owner.grid_step.y * elem_factor));
    int p_down = int((float(owner.rb.y) - base_y) / (owner.grid_step.y * elem_factor));
    int n_up   = (owner.grid.y < p_up) ? owner.grid.y : p_up;
    int n_down = (owner.grid.y < p_up) ? owner.grid.y : p_down;

    Push(static_cast<float>(owner.lt.x), base_y, owner.base_color);
    Push(static_cast<float>(owner.rb.x), base_y, owner.base_color);
    m_currentItem.vertexCount += 2;

    for (int gx = 1; gx <= owner.grid.x; ++gx)
    {
        const float x = owner.lt.x + gx * owner.grid_step.x * elem_factor;
        Push(x, static_cast<float>(owner.lt.y), owner.grid_color);
        Push(x, static_cast<float>(owner.rb.y), owner.grid_color);
    }
    for (int gy = 1; gy <= n_down; ++gy)
    {
        const float y = base_y + gy * owner.grid_step.y * elem_factor;
        Push(static_cast<float>(owner.lt.x), y, owner.grid_color);
        Push(static_cast<float>(owner.rb.x), y, owner.grid_color);
    }
    for (int gy = 1; gy <= n_up; ++gy)
    {
        const float y = base_y - gy * owner.grid_step.y * elem_factor;
        Push(static_cast<float>(owner.lt.x), y, owner.grid_color);
        Push(static_cast<float>(owner.rb.x), y, owner.grid_color);
    }
}

void FGStatGraphRender::RenderBars(CStatGraph& owner, CStatGraph::ElementsDeq& pelements)
{
    const float elem_offs   = float(owner.rb.x - owner.lt.x) / owner.max_item_count;
    const float elem_factor = float(owner.rb.y - owner.lt.y) / float(owner.mx - owner.mn);
    const float base_y      = float(owner.rb.y) + (owner.mn * elem_factor);

    float column_width = elem_offs;
    if (column_width > 1) column_width--;

    for (auto it = pelements.begin(); it != pelements.end(); ++it)
    {
        const float X  = float(it - pelements.begin()) * elem_offs + owner.lt.x;
        const float Y0 = base_y;
        const float Y1 = base_y - it->data * elem_factor;
        const float yt = std::min(Y0, Y1);
        const float yb = std::max(Y0, Y1);

        Push(X,                Y1 > Y0 ? Y1 : Y0, it->color);
        Push(X,                Y1 > Y0 ? Y0 : Y1, it->color);
        Push(X + column_width, Y1 > Y0 ? Y1 : Y0, it->color);
        Push(X,                Y1 > Y0 ? Y0 : Y1, it->color);
        Push(X + column_width, Y1 > Y0 ? Y0 : Y1, it->color);
        Push(X + column_width, Y1 > Y0 ? Y1 : Y0, it->color);
        (void)yt; (void)yb;
    }
}

void FGStatGraphRender::RenderLines(CStatGraph& owner, CStatGraph::ElementsDeq& pelements)
{
    if (pelements.size() <= 1) return;
    const float elem_offs   = float(owner.rb.x - owner.lt.x) / owner.max_item_count;
    const float elem_factor = float(owner.rb.y - owner.lt.y) / float(owner.mx - owner.mn);
    const float base_y      = float(owner.rb.y) + (owner.mn * elem_factor);

    for (auto it = pelements.begin() + 1; it != pelements.end(); ++it)
    {
        auto prev = it - 1;
        const float X0 = float(prev - pelements.begin()) * elem_offs + owner.lt.x;
        const float Y0 = base_y - prev->data * elem_factor;
        const float X1 = float(it   - pelements.begin()) * elem_offs + owner.lt.x;
        const float Y1 = base_y - it->data * elem_factor;
        Push(X0, Y0, it->color);
        Push(X1, Y1, it->color);
    }
}

void FGStatGraphRender::RenderBarLines(CStatGraph& owner, CStatGraph::ElementsDeq& pelements)
{
    if (pelements.size() <= 1) return;
    const float elem_offs   = float(owner.rb.x - owner.lt.x) / owner.max_item_count;
    const float elem_factor = float(owner.rb.y - owner.lt.y) / float(owner.mx - owner.mn);
    const float base_y      = float(owner.rb.y) + (owner.mn * elem_factor);

    for (auto it = pelements.begin() + 1; it != pelements.end(); ++it)
    {
        auto prev = it - 1;
        const float X0 = float(prev - pelements.begin()) * elem_offs + owner.lt.x + elem_offs;
        const float Y0 = base_y - prev->data * elem_factor;
        const float X1 = float(it   - pelements.begin()) * elem_offs + owner.lt.x;
        const float Y1 = base_y - it->data * elem_factor;
        Push(X0, Y0, it->color);
        Push(X1, Y1, it->color);
        Push(X1,             Y1, it->color);
        Push(X1 + elem_offs, Y1, it->color);
    }
}

void FGStatGraphRender::RenderMarkers(CStatGraph& owner, CStatGraph::MarkersDeq& pmarkers)
{
    const float elem_offs   = float(owner.rb.x - owner.lt.x) / owner.max_item_count;
    const float elem_factor = float(owner.rb.y - owner.lt.y) / float(owner.mx - owner.mn);
    const float base_y      = float(owner.rb.y) + (owner.mn * elem_factor);

    for (CStatGraph::SMarker& m : pmarkers)
    {
        float X0 = 0, Y0 = 0, X1 = 0, Y1 = 0;
        if (m.m_eStyle == CStatGraph::stVert)
        {
            X0 = m.m_fPos * elem_offs + owner.lt.x;
            clamp(X0, float(owner.lt.x), float(owner.rb.x));
            X1 = X0;
            Y0 = float(owner.lt.y);
            Y1 = float(owner.rb.y);
        }
        else if (m.m_eStyle == CStatGraph::stHor)
        {
            X0 = float(owner.lt.x);
            X1 = float(owner.rb.x);
            Y0 = base_y - m.m_fPos * elem_factor;
            clamp(Y0, float(owner.lt.y), float(owner.rb.y));
            Y1 = Y0;
        }
        Push(X0, Y0, m.m_dwColor);
        Push(X1, Y1, m.m_dwColor);
    }
}
}
