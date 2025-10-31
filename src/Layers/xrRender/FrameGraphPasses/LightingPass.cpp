#include "stdafx.h"
#include "LightingPass.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::passes {

using namespace framegraph;

LightingPass::LightingPass(ng::RenderDevice* device, const LightingPassConfig& config)
    : m_device(device)
    , m_config(config)
{
    VERIFY(m_device != nullptr);

    Msg("* [LightingPass] Created (%ux%u)", config.width, config.height);

    if (!LoadShaders())
    {
        Msg("! [LightingPass] Failed to load shaders");
    }
}

LightingPass::~LightingPass() {
    Msg("* [LightingPass] Destroyed");
}

bool LightingPass::LoadShaders()
{
    ShaderLoader loader(m_device);

    m_vertexShader = loader.LoadVertexShader("fullscreen");
    if (!m_vertexShader)
    {
        Msg("! [LightingPass] Failed to load fullscreen vertex shader");
        return false;
    }

    m_pixelShader = loader.LoadPixelShader("lighting");
    if (!m_pixelShader)
    {
        Msg("! [LightingPass] Failed to load lighting pixel shader");
        return false;
    }

    Msg("  ✓ Lighting shaders loaded successfully");
    return true;
}

bool LightingPass::CreatePipeline(nvrhi::ITexture* hdrTexture)
{
    VERIFY(m_vertexShader != nullptr);
    VERIFY(m_pixelShader != nullptr);
    VERIFY(hdrTexture != nullptr);

    nvrhi::GraphicsPipelineDesc pipelineDesc;

    pipelineDesc.VS = m_vertexShader;
    pipelineDesc.PS = m_pixelShader;

    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

    nvrhi::RasterState& rasterState = pipelineDesc.renderState.rasterState;
    rasterState.cullMode = nvrhi::RasterCullMode::None;
    rasterState.fillMode = nvrhi::RasterFillMode::Solid;
    rasterState.frontCounterClockwise = false;

    nvrhi::BlendState& blendState = pipelineDesc.renderState.blendState;
    blendState.targets[0].enableBlend = false;
    blendState.targets[0].colorWriteMask = nvrhi::ColorMask::All;

    nvrhi::DepthStencilState& depthState = pipelineDesc.renderState.depthStencilState;
    depthState.depthTestEnable = false;
    depthState.depthWriteEnable = false;
    depthState.stencilEnable = false;

    auto hdrDesc = hdrTexture->getDesc();
    pipelineDesc.renderState.targetCount = 1;
    pipelineDesc.renderState.renderTargetFormats[0] = hdrDesc.format;

    m_pipeline = m_device->GetNVRHIDevice()->createGraphicsPipeline(pipelineDesc, nullptr);

    if (!m_pipeline)
    {
        Msg("! [LightingPass] Failed to create graphics pipeline");
        return false;
    }

    Msg("  ✓ LightingPass pipeline created successfully");
    return true;
}

LightingPassOutput LightingPass::Setup(
    FrameGraph& fg,
    const GBufferOutputs& gbuffer
) {
    Msg("~ [LightingPass] Setting up in FrameGraph");

    LightingPassOutput output;

    ResourceDesc hdrDesc;
    hdrDesc.type = ResourceDesc::Type::Texture2D;
    hdrDesc.width = m_config.width;
    hdrDesc.height = m_config.height;
    hdrDesc.format = m_config.hdrFormat;
    hdrDesc.isRenderTarget = true;
    hdrDesc.isTransient = true;
    hdrDesc.debugName = "HDR";

    output.hdrColor = fg.CreateTexture("HDR", hdrDesc);

    PassHandle lightingPass = fg.AddPass("Lighting");

    fg.PassRead(lightingPass, gbuffer.albedo, ResourceState::ShaderResource);
    fg.PassRead(lightingPass, gbuffer.normal, ResourceState::ShaderResource);
    fg.PassRead(lightingPass, gbuffer.material, ResourceState::ShaderResource);
    fg.PassRead(lightingPass, gbuffer.depth, ResourceState::ShaderResource);

    fg.PassWrite(lightingPass, output.hdrColor, ResourceState::RenderTarget);

    fg.SetPassCallback(lightingPass, [this, gbuffer, output](ng::RenderContext& ctx, const FrameGraph& fg) {
        this->Execute(ctx, fg, gbuffer, output);
    });

    Msg("  ✓ Lighting pass configured");

    return output;
}

void LightingPass::Execute(
    ng::RenderContext& ctx,
    const FrameGraph& fg,
    const GBufferOutputs& gbuffer,
    const LightingPassOutput& output
) {
    Msg("~ [LightingPass] Executing");

    auto executeStart = std::chrono::high_resolution_clock::now();

    m_stats = Stats{};

    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(gbuffer.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(gbuffer.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(gbuffer.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(gbuffer.depth);
    nvrhi::ITexture* hdr = fg.GetPhysicalTexture(output.hdrColor);

    if (!m_pipeline)
    {
        if (!CreatePipeline(hdr))
        {
            Msg("! [LightingPass] Failed to create pipeline");
            ctx.EndRenderPass();
            return;
        }
    }

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = hdr;
    passDesc.numRenderTargets = 1;
    passDesc.clearColor = true;
    passDesc.clearValue.color[0] = 0.0f;
    passDesc.clearValue.color[1] = 0.0f;
    passDesc.clearValue.color[2] = 0.0f;
    passDesc.clearValue.color[3] = 1.0f;

    ctx.BeginRenderPass(passDesc);

    ctx.SetViewport(0, 0,
        static_cast<float>(m_config.width),
        static_cast<float>(m_config.height));

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    ctx.SetPipeline(m_pipeline.Get());

    ctx.Draw(3, 0);

    Msg("  ✓ Lighting draw complete (3 vertices)");

    ctx.EndRenderPass();

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_stats.cpuTimeMs = std::chrono::duration<float, std::milli>(
        executeEnd - executeStart
    ).count();

    m_stats.numLights = 1;

    Msg("  ✓ Lighting pass complete: %.2f ms", m_stats.cpuTimeMs);
}

}
