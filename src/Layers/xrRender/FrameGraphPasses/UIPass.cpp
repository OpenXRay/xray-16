// xrRender/FrameGraphPasses/UIPass.cpp
#include "stdafx.h"
#include "UIPass.h"
#include "xrEngine/IGame_Persistent.h"
#include "Layers/xrRender/UIRenderCollector.h"
#include "Layers/xrRender/NVRHIUIRenderer.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"

namespace xray::render::passes {

UIPass::UIPass(ng::RenderDevice* device, const UIPassConfig& config)
    : m_device(device)
    , m_config(config)
    , m_uiStats{}
    , m_outputRT{}
    , m_depthStencil{}
{
    VERIFY(m_device != nullptr);

    // Create VCB pool for dynamic constant buffer management
    m_vcbPool = xr_make_unique<framegraph::VolatileConstantBufferPool>(device);

    // Create material cache (UIPass owns its own MaterialCache with VCB pool)
    m_materialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),  // Pass FGResourceManager for native texture loading
        m_vcbPool.get()
    );

    // Create UI rendering infrastructure
    m_uiCollector = xr_make_unique<ui::UIRenderCollector>();
    m_uiRenderer = xr_make_unique<ui::NVRHIUIRenderer>();

    Msg("* [UIPass] Created (resolution: %ux%u)", config.width, config.height);
}

UIPass::~UIPass() {
    Msg("* [UIPass] Destroyed");
}

void UIPass::SetOutputs(framegraph::VirtualResourceHandle uiMain, framegraph::VirtualResourceHandle depth) {
    m_outputRT = uiMain;
    m_depthStencil = depth;
}

void UIPass::Setup(framegraph::FrameGraph& fg) {
    // UIPass uses externally-created resources (rt_UIMain, rt_Depth)
    // No need to create or declare them - they're already registered in BuildFrameGraphStructure()
    Msg("  [UIPass::Setup] Registered pass with FrameGraph");
}

void UIPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    // Get command list for PIX marker
    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("UIPass");

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [UIPass::Execute] Failed to get physical textures");
        cmdList->endMarker();
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  BEGIN RENDER PASS
    // ═══════════════════════════════════════════════════════

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [UIPass::Execute] Failed to create framebuffer");
        cmdList->endMarker();
        return;
    }

    // Simple clear operation - no render state needed yet
    cmdList->open();

    // Clear render target to transparent
    cmdList->clearTextureFloat(outputTexture, nvrhi::AllSubresources,
        nvrhi::Color(m_config.clearColor[0], m_config.clearColor[1],
                     m_config.clearColor[2], m_config.clearColor[3]));

    // Clear depth buffer
    cmdList->clearDepthStencilTexture(depthTexture, nvrhi::AllSubresources, true, m_config.clearDepth, true, m_config.clearStencil);

    cmdList->close();
    m_device->GetNVRHIDevice()->executeCommandList(cmdList);

    // ═══════════════════════════════════════════════════════
    //  NVRHI UI RENDERING (New Path)
    // ═══════════════════════════════════════════════════════
    // Use NVRHI-based UI renderer instead of legacy D3D11 path

    if (g_pGamePersistent) {
        Msg("  [UIPass::Execute] Rendering UI via NVRHI");

        // Initialize NVRHI UI renderer on first use
        if (!m_nvrhiUIInitialized) {
            m_uiRenderer->Initialize(m_device, m_materialCache.get());
            m_nvrhiUIInitialized = true;
        }

        // STEP 1: Collect UI geometry by temporarily swapping GEnv.UIRender
        IUIRender* oldRenderer = GEnv.UIRender;
        m_uiCollector->Clear();
        GEnv.UIRender = m_uiCollector.get();

        // Call legacy UI rendering - it will record geometry instead of rendering
        g_pGamePersistent->OnRenderPPUI_main();

        // Restore original renderer
        GEnv.UIRender = oldRenderer;

        Msg("  [UIPass::Execute] Collected %zu UI batches", m_uiCollector->GetBatches().size());

        // STEP 2: Render collected geometry via NVRHI
        if (!m_uiCollector->GetBatches().empty()) {
            m_uiRenderer->RenderBatches(
                cmdList,
                m_uiCollector->GetBatches(),
                framebuffer,
                m_config.width,
                m_config.height
            );

            Msg("  [UIPass::Execute] NVRHI UI rendering complete");
            m_uiStats.numBatches = static_cast<u32>(m_uiCollector->GetBatches().size());
        } else {
            Msg("  [UIPass::Execute] No UI geometry collected");
            m_uiStats.numBatches = 0;
        }
    } else {
        Msg("  [UIPass::Execute] No GamePersistent - clearing to transparent only");
        m_uiStats.numBatches = 0;
    }

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_uiStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    cmdList->endMarker();
    Msg("  [UIPass] Execute complete (%.2f ms, %u batches)", m_uiStats.cpuTimeMs, m_uiStats.numBatches);
}

} // namespace xray::render::passes
