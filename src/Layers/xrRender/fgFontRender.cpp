#include "stdafx.h"
#include "fgFontRender.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "xrCore/Text/StringConversion.hpp"

extern ENGINE_API bool g_bRendering;
extern ENGINE_API Fvector2 g_current_font_scale;

namespace xray::render::fg
{
namespace
{
    constexpr u32 kMaxQuads      = 4096;
    constexpr u32 kInitialVerts  = 1024;

    struct FontCB
    {
        Fvector4 screen_res;
    };
}

FGFontRender::FGFontRender()
{
    InitResources();
}

FGFontRender::~FGFontRender()
{
}

void FGFontRender::InitResources()
{
    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* renderDevice = fgRenderer->GetRenderDevice();
    m_device = renderDevice->GetNVRHIDevice();
    R_ASSERT(m_device);

    auto* shaderLoader = RImplementation.GetShaderLoader();
    R_ASSERT(shaderLoader);

    auto vsResult = shaderLoader->LoadVertexShader("font_basic", "main");
    R_ASSERT2(vsResult.handle, "FGFontRender: failed to load font_basic.vs");
    m_vs = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("font_basic", "main");
    R_ASSERT2(psResult.handle, "FGFontRender: failed to load font_basic.ps");
    m_ps = psResult.handle;

    nvrhi::VertexAttributeDesc vertexAttrs[] = {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("COLOR")   .setFormat(nvrhi::Format::RGBA8_UNORM) .setOffset(16).setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT)  .setOffset(20).setElementStride(sizeof(Vertex)),
    };
    m_inputLayout = m_device->createInputLayout(vertexAttrs, 3, m_vs);
    R_ASSERT2(m_inputLayout, "FGFontRender: createInputLayout failed");

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(FontCB);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;
    cbDesc.debugName = "FGFontRender_CB";
    m_constantBuffer = m_device->createBuffer(cbDesc);
    R_ASSERT2(m_constantBuffer, "FGFontRender: createBuffer(CB) failed");

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
    samplerDesc.setAllFilters(true);
    m_sampler = m_device->createSampler(samplerDesc);
    R_ASSERT2(m_sampler, "FGFontRender: createSampler failed");

    nvrhi::BindingLayoutDesc bindingLayoutDesc;
    bindingLayoutDesc.visibility = nvrhi::ShaderType::All;
    bindingLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        nvrhi::BindingLayoutItem::Sampler(0),
    };
    m_bindingLayout = m_device->createBindingLayout(bindingLayoutDesc);
    R_ASSERT2(m_bindingLayout, "FGFontRender: createBindingLayout failed");

    xr_vector<u16> quadIndices;
    quadIndices.reserve(kMaxQuads * 6);
    for (u16 q = 0; q < kMaxQuads; ++q)
    {
        const u16 base = q * 4;
        quadIndices.push_back(base + 0);
        quadIndices.push_back(base + 1);
        quadIndices.push_back(base + 2);
        quadIndices.push_back(base + 1);
        quadIndices.push_back(base + 3);
        quadIndices.push_back(base + 2);
    }

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = quadIndices.size() * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.debugName = "FGFontRender_IB";
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;
    m_indexBuffer = m_device->createBuffer(ibDesc);
    R_ASSERT2(m_indexBuffer, "FGFontRender: createBuffer(IB) failed");

    nvrhi::CommandListHandle uploadCL = m_device->createCommandList();
    uploadCL->open();
    uploadCL->writeBuffer(m_indexBuffer, quadIndices.data(), ibDesc.byteSize);
    uploadCL->close();
    m_device->executeCommandList(uploadCL);

    EnsureVertexCapacity(kInitialVerts);
}

void FGFontRender::Initialize(cpcstr, cpcstr cTexture)
{
    R_ASSERT(m_device);
    if (!cTexture)
        return;

    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* textureManager = fgRenderer->GetRenderDevice()->GetFGResourceManager()->GetTextureManager();
    R_ASSERT(textureManager);

    auto handle = textureManager->LoadTexture(cTexture);
    m_texture = textureManager->GetNVRHITexture(handle);
    R_ASSERT2(m_texture, "FGFontRender: failed to load font texture");

    const auto& desc = m_texture->getDesc();
    m_textureSize.set(float(desc.width), float(desc.height));

    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
        nvrhi::BindingSetItem::Texture_SRV(0, m_texture),
        nvrhi::BindingSetItem::Sampler(0, m_sampler),
    };
    m_bindingSet = m_device->createBindingSet(bindingSetDesc, m_bindingLayout);
    R_ASSERT2(m_bindingSet, "FGFontRender: createBindingSet failed");
}

void FGFontRender::EnsureVertexCapacity(size_t vertexCount)
{
    if (vertexCount <= m_vertexCapacity && m_vertexBuffer)
        return;

    const size_t newCap = std::max<size_t>(vertexCount * 2, kInitialVerts);
    nvrhi::BufferDesc d;
    d.byteSize = newCap * sizeof(Vertex);
    d.isVertexBuffer = true;
    d.debugName = "FGFontRender_VB";
    d.initialState = nvrhi::ResourceStates::VertexBuffer;
    d.keepInitialState = true;
    m_vertexBuffer = m_device->createBuffer(d);
    m_vertexCapacity = newCap;
}

void FGFontRender::EnsurePipeline(nvrhi::IFramebuffer* framebuffer)
{
    if (m_pipeline)
        return;

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.VS = m_vs;
    pipelineDesc.PS = m_ps;
    pipelineDesc.inputLayout = m_inputLayout;
    pipelineDesc.bindingLayouts = { m_bindingLayout };
    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipelineDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    pipelineDesc.renderState.depthStencilState.depthTestEnable = false;
    pipelineDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipelineDesc.renderState.blendState.targets[0]
        .setBlendEnable(true)
        .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
        .setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
        .setSrcBlendAlpha(nvrhi::BlendFactor::One)
        .setDestBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha);

    m_pipeline = m_device->createGraphicsPipeline(pipelineDesc, framebuffer);
    R_ASSERT2(m_pipeline, "FGFontRender: createGraphicsPipeline failed");
}

void FGFontRender::BuildGeometry(CGameFont& owner)
{
    if (owner.strings.empty())
        return;

    if (!(owner.uFlags & CGameFont::fsValid) && m_textureSize.x > 0.f)
    {
        owner.vTS.set(int(m_textureSize.x), int(m_textureSize.y));
        owner.fTCHeight = owner.fHeight / m_textureSize.y;
        owner.uFlags |= CGameFont::fsValid;
    }

    for (const auto& str : owner.strings)
    {
        xr_wide_char wsStr[MAX_MB_CHARS];
        const u16 len = owner.IsMultibyte()
            ? mbhMulti2Wide(wsStr, nullptr, MAX_MB_CHARS, str.string)
            : xr_strlen(str.string);
        if (len == 0)
            continue;

        float X = float(iFloor(str.x));
        float Y = float(iFloor(str.y));
        const float S = str.height * g_current_font_scale.y;
        float Y2 = Y + S;

        float fSize = 0.f;
        if (str.align)
            fSize = owner.IsMultibyte() ? owner.SizeOf_(wsStr) : owner.SizeOf_(str.string);

        switch (str.align)
        {
        case CGameFont::alCenter: X -= iFloor(fSize * 0.5f) * g_current_font_scale.x; break;
        case CGameFont::alRight:  X -= iFloor(fSize); break;
        }

        const u32 clr = str.c;
        u32 clr2 = clr;
        if (owner.uFlags & CGameFont::fsGradient)
        {
            const u32 r = color_get_R(clr) / 2;
            const u32 g = color_get_G(clr) / 2;
            const u32 b = color_get_B(clr) / 2;
            const u32 a = color_get_A(clr);
            clr2 = color_rgba(r, g, b, a);
        }

        X  -= 0.5f;
        Y  -= 0.5f;
        Y2 -= 0.5f;

        for (u16 j = 0; j < len; ++j)
        {
            const u16 wc = owner.IsMultibyte() ? wsStr[1 + j] : (u16)(u8)str.string[j];
            const Fvector charTC = owner.GetCharTC(wc);

            const float scw = charTC.z * g_current_font_scale.x;
            const float fTCWidth = charTC.z / owner.vTS.x;

            if (!fis_zero(charTC.z))
            {
                const float tu = charTC.x / owner.vTS.x;
                const float tv = charTC.y / owner.vTS.y;

                m_vertices.push_back({ X,       Y2, 0.f, 1.f, clr2, tu,            tv + owner.fTCHeight });
                m_vertices.push_back({ X,       Y,  0.f, 1.f, clr,  tu,            tv });
                m_vertices.push_back({ X + scw, Y2, 0.f, 1.f, clr2, tu + fTCWidth, tv + owner.fTCHeight });
                m_vertices.push_back({ X + scw, Y,  0.f, 1.f, clr,  tu + fTCWidth, tv });
            }

            X += scw * owner.vInterval.x;
            if (owner.IsMultibyte())
            {
                X -= 2;
                if (IsNeedSpaceCharacter(wsStr[1 + j]))
                    X += owner.fXStep;
            }
        }
    }
}

void FGFontRender::OnRender(CGameFont& owner)
{
    BuildGeometry(owner);
    owner.strings.clear();
}

void FGFontRender::Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer)
{
    if (m_vertices.empty() || !m_bindingSet || !cmdList || !framebuffer)
    {
        m_vertices.clear();
        return;
    }

    EnsurePipeline(framebuffer);
    if (!m_pipeline)
    {
        m_vertices.clear();
        return;
    }

    const u32 maxVerts = kMaxQuads * 4;
    if (m_vertices.size() > maxVerts)
        m_vertices.resize(maxVerts);

    EnsureVertexCapacity(m_vertices.size());

    cmdList->writeBuffer(m_vertexBuffer, m_vertices.data(), m_vertices.size() * sizeof(Vertex));

    FontCB cb{};
    cb.screen_res.set(
        float(Device.dwWidth),
        float(Device.dwHeight),
        1.f / float(Device.dwWidth),
        1.f / float(Device.dwHeight));
    cmdList->writeBuffer(m_constantBuffer, &cb, sizeof(cb));

    cmdList->setBufferState(m_vertexBuffer,   nvrhi::ResourceStates::VertexBuffer);
    cmdList->setBufferState(m_constantBuffer, nvrhi::ResourceStates::ConstantBuffer);

    const auto& fbInfo = framebuffer->getFramebufferInfo();

    nvrhi::VertexBufferBinding vb;
    vb.buffer = m_vertexBuffer;
    vb.slot = 0;
    vb.offset = 0;

    nvrhi::GraphicsState state;
    state.pipeline = m_pipeline;
    state.framebuffer = framebuffer;
    state.bindings = { m_bindingSet };
    state.vertexBuffers = { vb };
    state.indexBuffer.buffer = m_indexBuffer;
    state.indexBuffer.format = nvrhi::Format::R16_UINT;
    state.indexBuffer.offset = 0;
    state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(
        nvrhi::Viewport(float(fbInfo.width), float(fbInfo.height)));
    cmdList->setGraphicsState(state);

    const u32 quadCount = u32(m_vertices.size()) / 4;
    nvrhi::DrawArguments args;
    args.vertexCount = quadCount * 6;
    args.instanceCount = 1;
    cmdList->drawIndexed(args);

    m_vertices.clear();
}
}
