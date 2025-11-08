#include "stdafx.h"
#include "MenuUIPass.h"
#include "xrEngine/IGame_Persistent.h"
#include "Layers/xrRender/UIRenderCollector.h"
#include "Layers/xrRender/NVRHIUIRenderer.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"

namespace xray::render::passes {

MenuUIPass::MenuUIPass(ng::RenderDevice* device, const MenuUIPassConfig& config)
    : m_device(device)
    , m_config(config)
    , m_menuStats{}
    , m_outputRT{}
    , m_depthStencil{}
{
    VERIFY(m_device != nullptr);

    m_vcbPool = xr_make_unique<framegraph::VolatileConstantBufferPool>(device);

    m_materialCache = xr_make_unique<MaterialCache>(device, m_vcbPool.get());

    m_uiCollector = xr_make_unique<ui::UIRenderCollector>();
    m_uiRenderer = xr_make_unique<ui::NVRHIUIRenderer>();

    Msg("* [MenuUIPass] Created (resolution: %ux%u)", config.width, config.height);
}

MenuUIPass::~MenuUIPass() {
    Msg("* [MenuUIPass] Destroyed");
}

void MenuUIPass::SetOutputs(framegraph::VirtualResourceHandle menuMain, framegraph::VirtualResourceHandle depth) {
    m_outputRT = menuMain;
    m_depthStencil = depth;
}

void MenuUIPass::Setup(framegraph::FrameGraph& fg) {
    Msg("  [MenuUIPass::Setup] Registered pass with FrameGraph");
}

void MenuUIPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [MenuUIPass::Execute] Failed to get physical textures");
        return;
    }

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [MenuUIPass::Execute] Failed to create framebuffer");
        return;
    }

    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);

    cmdList->open();

    cmdList->clearTextureFloat(outputTexture, nvrhi::AllSubresources,
        nvrhi::Color(m_config.clearColor[0], m_config.clearColor[1],
                     m_config.clearColor[2], m_config.clearColor[3]));

    cmdList->clearDepthStencilTexture(depthTexture, nvrhi::AllSubresources, true, m_config.clearDepth, true, m_config.clearStencil);

    cmdList->close();
    m_device->GetNVRHIDevice()->executeCommandList(cmdList);

    if (g_pGamePersistent) {
        Msg("  [MenuUIPass::Execute] Rendering UI via NVRHI");

        if (!m_nvrhiUIInitialized) {
            m_uiRenderer->Initialize(m_device, m_materialCache.get());
            m_nvrhiUIInitialized = true;
        }

        IUIRender* oldRenderer = GEnv.UIRender;
        m_uiCollector->Clear();
        GEnv.UIRender = m_uiCollector.get();

        g_pGamePersistent->OnRenderPPUI_main();

        GEnv.UIRender = oldRenderer;

        Msg("  [MenuUIPass::Execute] Collected %zu UI batches", m_uiCollector->GetBatches().size());

        if (!m_uiCollector->GetBatches().empty()) {
            m_uiRenderer->RenderBatches(
                cmdList,
                m_uiCollector->GetBatches(),
                framebuffer,
                m_config.width,
                m_config.height
            );

            Msg("  [MenuUIPass::Execute] NVRHI UI rendering complete");
        } else {
            Msg("  [MenuUIPass::Execute] No UI geometry collected");
        }
    } else {
        Msg("  [MenuUIPass::Execute] No GamePersistent - clearing to black only");
    }

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_menuStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();
    m_menuStats.numDialogs = 0;

    Msg("  [MenuUIPass] Execute complete (%.2f ms)", m_menuStats.cpuTimeMs);
}

}
