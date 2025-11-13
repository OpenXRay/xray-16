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
    Msg("  [MenuCompositePass::Setup] Registered pass with FrameGraph");
}

bool MenuCompositePass::Initialize() {
    Msg("  [MenuCompositePass] Initializing alpha-blend compositing resources...");

    nvrhi::IDevice* nvrhiDevice = m_device->GetNVRHIDevice();

    framegraph::ShaderLoader shaderLoader(m_device, m_device->GetSlangCompiler());

    m_vertexShader = shaderLoader.LoadVertexShader("ui_composite");
    m_pixelShader = shaderLoader.LoadPixelShader("ui_composite");

    if (!m_vertexShader || !m_pixelShader) {
        Msg("! [MenuCompositePass] Failed to load shaders");
        return false;
    }

    Msg("  ✓ Shaders loaded (ui_composite.vs, ui_composite.ps)");

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
    m_linearSampler = nvrhiDevice->createSampler(samplerDesc);

    if (!m_linearSampler) {
        Msg("! [MenuCompositePass] Failed to create sampler");
        return false;
    }

    Msg("  ✓ Sampler created");

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::Pixel;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        nvrhi::BindingLayoutItem::Texture_SRV(1),
        nvrhi::BindingLayoutItem::Sampler(0)
    };

    m_bindingLayout = nvrhiDevice->createBindingLayout(layoutDesc);

    if (!m_bindingLayout) {
        Msg("! [MenuCompositePass] Failed to create binding layout");
        return false;
    }

    Msg("  ✓ Binding layout created");

    nvrhi::GraphicsPipelineDesc psoDesc;

    psoDesc.VS = m_vertexShader;
    psoDesc.PS = m_pixelShader;

    psoDesc.primType = nvrhi::PrimitiveType::TriangleList;

    psoDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    psoDesc.renderState.rasterState.fillMode = nvrhi::RasterFillMode::Solid;
    psoDesc.renderState.rasterState.frontCounterClockwise = false;

    psoDesc.renderState.blendState.targets[0].blendEnable = false;
    psoDesc.renderState.blendState.targets[0].colorWriteMask = nvrhi::ColorMask::All;

    psoDesc.renderState.depthStencilState.depthTestEnable = false;
    psoDesc.renderState.depthStencilState.depthWriteEnable = false;
    psoDesc.renderState.depthStencilState.stencilEnable = false;

    psoDesc.bindingLayouts = { m_bindingLayout };

    nvrhi::FramebufferInfoEx framebufferInfo;
    framebufferInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);

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

    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("UICompositePass");

    if (!m_initialized) {
        if (!Initialize()) {
            Msg("! [MenuCompositePass::Execute] Failed to initialize");
            cmdList->endMarker();
            return;
        }
    }

    bool hasScene = m_inputScene.is_valid();

    nvrhi::ITexture* sceneTexture = hasScene ? fg.GetPhysicalTexture(m_inputScene) : nullptr;
    nvrhi::ITexture* uiTexture = fg.GetPhysicalTexture(m_inputUI);
    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);

    if (!uiTexture || !outputTexture) {
        Msg("! [MenuCompositePass::Execute] Failed to get physical textures (UI or output missing)");
        cmdList->endMarker();
        return;
    }

    if (!hasScene || !sceneTexture) {
        Msg("  [MenuCompositePass] No scene input - copying UI directly to output (main menu mode)");
        cmdList->copyTexture(outputTexture, nvrhi::TextureSlice(), uiTexture, nvrhi::TextureSlice());
        cmdList->endMarker();
        return;
    }

    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        nvrhi::BindingSetItem::Texture_SRV(0, sceneTexture),
        nvrhi::BindingSetItem::Texture_SRV(1, uiTexture),
        nvrhi::BindingSetItem::Sampler(0, m_linearSampler)
    };

    m_bindingSet = m_device->GetNVRHIDevice()->createBindingSet(bindingSetDesc, m_bindingLayout);

    if (!m_bindingSet) {
        Msg("! [MenuCompositePass] Failed to create binding set");
        cmdList->endMarker();
        return;
    }

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = outputTexture;
    passDesc.numRenderTargets = 1;
    passDesc.depthStencil = nullptr;
    passDesc.clearColor = false;
    passDesc.clearDepth = false;

    ctx.BeginRenderPass(passDesc);

    ctx.SetViewport(0, 0, (float)m_config.width, (float)m_config.height);

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    ctx.SetPipeline(m_pipeline);

    ctx.SetBindingSet(0, m_bindingSet.Get());

    ctx.Draw(3, 0);

    ctx.EndRenderPass();

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_menuCompositeStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    cmdList->endMarker();
    Msg("  [MenuCompositePass] Execute complete (%.2f ms)", m_menuCompositeStats.cpuTimeMs);
}

}
