// xrRender/FrameGraphPasses/MenuCompositePass.cpp
#include "stdafx.h"
#include "MenuCompositePass.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::passes {

MenuCompositePass::MenuCompositePass(ng::RenderDevice* device, const MenuCompositePassConfig& config)
    : m_device(device)
    , m_config(config)
    , m_menuCompositeStats{}
    , m_inputMenuMain{}
    , m_inputMenuDistort{}
    , m_outputRT{}
{
    VERIFY(m_device != nullptr);
    Msg("* [MenuCompositePass] Created (resolution: %ux%u)", config.width, config.height);
}

MenuCompositePass::~MenuCompositePass() {
    Msg("* [MenuCompositePass] Destroyed");
}

void MenuCompositePass::SetInputs(framegraph::VirtualResourceHandle sceneRT, framegraph::VirtualResourceHandle uiRT) {
    m_inputScene = sceneRT;
    m_inputUI = uiRT;
}

void MenuCompositePass::SetOutput(framegraph::VirtualResourceHandle finalOutput) {
    m_outputRT = finalOutput;
}

void MenuCompositePass::Setup(framegraph::FrameGraph& fg) {
    // MenuCompositePass uses externally-created resources
    // Inputs: scene RT + UI RT (read-only)
    // Output: final output RT (write)
    Msg("  [MenuCompositePass::Setup] Registered pass with FrameGraph");
}

bool MenuCompositePass::Initialize() {
    Msg("  [MenuCompositePass] Initializing alpha-blend compositing resources...");

    nvrhi::IDevice* nvrhiDevice = m_device->GetNVRHIDevice();

    // ═══════════════════════════════════════════════════════
    //  LOAD SHADERS
    // ═══════════════════════════════════════════════════════
    framegraph::ShaderLoader shaderLoader(m_device, m_device->GetSlangCompiler());

    m_vertexShader = shaderLoader.LoadVertexShader("ui_composite");
    m_pixelShader = shaderLoader.LoadPixelShader("ui_composite");

    if (!m_vertexShader || !m_pixelShader) {
        Msg("! [MenuCompositePass] Failed to load shaders");
        return false;
    }

    Msg("  ✓ Shaders loaded (ui_composite.vs, ui_composite.ps)");

    // ═══════════════════════════════════════════════════════
    //  CREATE SAMPLER
    // ═══════════════════════════════════════════════════════
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);  // Linear filtering
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
    m_linearSampler = nvrhiDevice->createSampler(samplerDesc);

    if (!m_linearSampler) {
        Msg("! [MenuCompositePass] Failed to create sampler");
        return false;
    }

    Msg("  ✓ Sampler created");

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING LAYOUT
    // ═══════════════════════════════════════════════════════
    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::Pixel;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::Texture_SRV(0),  // sceneTexture (t0)
        nvrhi::BindingLayoutItem::Texture_SRV(1),  // uiTexture (t1)
        nvrhi::BindingLayoutItem::Sampler(0)       // linearSampler (s0)
    };

    m_bindingLayout = nvrhiDevice->createBindingLayout(layoutDesc);

    if (!m_bindingLayout) {
        Msg("! [MenuCompositePass] Failed to create binding layout");
        return false;
    }

    Msg("  ✓ Binding layout created");

    // ═══════════════════════════════════════════════════════
    //  CREATE PIPELINE STATE
    // ═══════════════════════════════════════════════════════
    nvrhi::GraphicsPipelineDesc psoDesc;

    // Shaders
    psoDesc.VS = m_vertexShader;
    psoDesc.PS = m_pixelShader;

    // Vertex input - NONE (fullscreen triangle uses SV_VertexID)
    psoDesc.primType = nvrhi::PrimitiveType::TriangleList;

    // Rasterizer state
    psoDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    psoDesc.renderState.rasterState.fillMode = nvrhi::RasterFillMode::Solid;
    psoDesc.renderState.rasterState.frontCounterClockwise = false;

    // Blend state - OPAQUE (no hardware blending)
    // Alpha blending is done manually in pixel shader!
    psoDesc.renderState.blendState.targets[0].blendEnable = false;
    psoDesc.renderState.blendState.targets[0].colorWriteMask = nvrhi::ColorMask::All;

    // Depth/stencil state - NO DEPTH (fullscreen composite)
    psoDesc.renderState.depthStencilState.depthTestEnable = false;
    psoDesc.renderState.depthStencilState.depthWriteEnable = false;
    psoDesc.renderState.depthStencilState.stencilEnable = false;

    // Binding layout
    psoDesc.bindingLayouts = { m_bindingLayout };

    // Create framebuffer info to specify render target format
    nvrhi::FramebufferInfoEx framebufferInfo;
    framebufferInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);  // HDR format

    m_pipeline = nvrhiDevice->createGraphicsPipeline(psoDesc, framebufferInfo);

    if (!m_pipeline) {
        Msg("! [MenuCompositePass] Failed to create graphics pipeline");
        return false;
    }

    Msg("  ✓ Pipeline created");
    Msg("  ✓ MenuCompositePass initialization complete");

    m_initialized = true;
    return true;
}

void MenuCompositePass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    // Get command list for PIX marker
    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("UICompositePass");

    // Initialize on first use
    if (!m_initialized) {
        if (!Initialize()) {
            Msg("! [MenuCompositePass::Execute] Failed to initialize");
            cmdList->endMarker();
            return;
        }
    }

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════
    // Check if we have a valid scene input (only present in-game, not in main menu)
    bool hasScene = m_inputScene.is_valid();

    nvrhi::ITexture* sceneTexture = hasScene ? fg.GetPhysicalTexture(m_inputScene) : nullptr;
    nvrhi::ITexture* uiTexture = fg.GetPhysicalTexture(m_inputUI);
    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);

    if (!uiTexture || !outputTexture) {
        Msg("! [MenuCompositePass::Execute] Failed to get physical textures (UI or output missing)");
        cmdList->endMarker();
        return;
    }

    // If no scene, just copy UI directly to output
    if (!hasScene || !sceneTexture) {
        Msg("  [MenuCompositePass] No scene input - copying UI directly to output (main menu mode)");
        cmdList->copyTexture(outputTexture, nvrhi::TextureSlice(), uiTexture, nvrhi::TextureSlice());
        cmdList->endMarker();
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE/UPDATE BINDING SET
    // ═══════════════════════════════════════════════════════
    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        nvrhi::BindingSetItem::Texture_SRV(0, sceneTexture),  // Base scene layer
        nvrhi::BindingSetItem::Texture_SRV(1, uiTexture),     // UI layer with alpha
        nvrhi::BindingSetItem::Sampler(0, m_linearSampler)
    };

    m_bindingSet = m_device->GetNVRHIDevice()->createBindingSet(bindingSetDesc, m_bindingLayout);

    if (!m_bindingSet) {
        Msg("! [MenuCompositePass] Failed to create binding set");
        cmdList->endMarker();
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  BEGIN RENDER PASS
    // ═══════════════════════════════════════════════════════
    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = outputTexture;
    passDesc.numRenderTargets = 1;
    passDesc.depthStencil = nullptr;  // No depth for fullscreen composite
    passDesc.clearColor = false;       // Don't clear - we're blending
    passDesc.clearDepth = false;

    ctx.BeginRenderPass(passDesc);

    // ═══════════════════════════════════════════════════════
    //  SET VIEWPORT & SCISSOR
    // ═══════════════════════════════════════════════════════
    ctx.SetViewport(0, 0, (float)m_config.width, (float)m_config.height);

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    // ═══════════════════════════════════════════════════════
    //  SET PIPELINE & BINDINGS
    // ═══════════════════════════════════════════════════════
    ctx.SetPipeline(m_pipeline);

    // Bind textures (slot 0 = pixel shader bindings)
    ctx.SetBindingSet(0, m_bindingSet.Get());

    // ═══════════════════════════════════════════════════════
    //  DRAW FULLSCREEN TRIANGLE
    // ═══════════════════════════════════════════════════════
    // Industry standard optimization:
    // - Draw 1 triangle with 3 vertices (not 2 triangles/6 vertices for quad)
    // - No index buffer needed
    // - Better cache coherency
    // - Vertices generated in VS using SV_VertexID:
    //   vertexID=0 → (-1, -1)  bottom-left  (off-screen)
    //   vertexID=1 → (-1,  3)  top-left     (off-screen)
    //   vertexID=2 → ( 3, -1)  bottom-right (off-screen)
    //   Triangle covers entire viewport with minimal overdraw

    ctx.Draw(3, 0);

    ctx.EndRenderPass();

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════
    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_menuCompositeStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    cmdList->endMarker();
    Msg("  [MenuCompositePass] Execute complete (%.2f ms)", m_menuCompositeStats.cpuTimeMs);
}

} // namespace xray::render::passes
