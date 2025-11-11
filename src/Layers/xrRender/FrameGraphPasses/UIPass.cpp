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

    m_vcbPool = xr_make_unique<framegraph::VolatileConstantBufferPool>(device);

    m_materialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_vcbPool.get()
    );

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
    Msg("  [UIPass::Setup] Registered pass with FrameGraph");
}

void UIPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("UIPass");

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [UIPass::Execute] Failed to get physical textures");
        cmdList->endMarker();
        return;
    }

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [UIPass::Execute] Failed to create framebuffer");
        cmdList->endMarker();
        return;
    }

    cmdList->open();

    cmdList->clearTextureFloat(outputTexture, nvrhi::AllSubresources,
        nvrhi::Color(m_config.clearColor[0], m_config.clearColor[1],
                     m_config.clearColor[2], m_config.clearColor[3]));

    cmdList->clearDepthStencilTexture(depthTexture, nvrhi::AllSubresources, true, m_config.clearDepth, true, m_config.clearStencil);

    cmdList->close();
    m_device->GetNVRHIDevice()->executeCommandList(cmdList);

    if (g_pGamePersistent) {
        Msg("  [UIPass::Execute] Rendering UI via NVRHI");

        if (!m_nvrhiUIInitialized) {
            m_uiRenderer->Initialize(m_device, m_materialCache.get());
            m_nvrhiUIInitialized = true;
        }

        IUIRender* oldRenderer = GEnv.UIRender;
        m_uiCollector->Clear();
        GEnv.UIRender = m_uiCollector.get();

        g_pGamePersistent->OnRenderPPUI_main();
        Msg("  [UIPass] After menu UI: %zu batches", m_uiCollector->GetBatches().size());

        g_pGamePersistent->OnRenderInGameUI();
        Msg("  [UIPass] After in-game UI: %zu batches", m_uiCollector->GetBatches().size());

        g_pGamePersistent->OnRenderCursor();
        Msg("  [UIPass] After cursor: %zu batches", m_uiCollector->GetBatches().size());

        GEnv.UIRender = oldRenderer;

        Msg("  [UIPass::Execute] Collected %zu UI batches", m_uiCollector->GetBatches().size());

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

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_uiStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    cmdList->endMarker();
    Msg("  [UIPass] Execute complete (%.2f ms, %u batches)", m_uiStats.cpuTimeMs, m_uiStats.numBatches);
}

}
