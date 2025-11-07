// xrRender/FrameGraphPasses/MenuUIPass.cpp
#include "stdafx.h"
#include "MenuUIPass.h"
#include "xrEngine/IGame_Persistent.h"

namespace xray::render::passes {

MenuUIPass::MenuUIPass(ng::RenderDevice* device, const MenuUIPassConfig& config)
    : m_device(device)
    , m_config(config)
    , m_menuStats{}
    , m_outputRT{}
    , m_depthStencil{}
{
    VERIFY(m_device != nullptr);
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
    // MenuUIPass uses externally-created resources (rt_MenuMain, rt_Depth)
    // No need to create or declare them - they're already registered in BuildFrameGraphStructure()
    Msg("  [MenuUIPass::Setup] Registered pass with FrameGraph");
}

void MenuUIPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [MenuUIPass::Execute] Failed to get physical textures");
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
        Msg("! [MenuUIPass::Execute] Failed to create framebuffer");
        return;
    }

    // Get command list from RenderContext
    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);

    // Simple clear operation - no render state needed yet
    cmdList->open();

    // Clear render target to black
    cmdList->clearTextureFloat(outputTexture, nvrhi::AllSubresources,
        nvrhi::Color(m_config.clearColor[0], m_config.clearColor[1],
                     m_config.clearColor[2], m_config.clearColor[3]));

    // Clear depth buffer
    cmdList->clearDepthStencilTexture(depthTexture, nvrhi::AllSubresources, true, m_config.clearDepth, true, m_config.clearStencil);

    cmdList->close();
    m_device->GetNVRHIDevice()->executeCommandList(cmdList);

    // ═══════════════════════════════════════════════════════
    //  RENDER LEGACY UI (BRIDGE CODE - Phase 6)
    // ═══════════════════════════════════════════════════════
    // TODO: Phase 6 will add the bridge to call legacy UI rendering here:
    //
    // if (g_pGamePersistent) {
    //     RCache.SetFrameGraphContext(&ctx);  // Inject RenderContext into legacy system
    //     g_pGamePersistent->OnRenderPPUI_main();  // Render main UI dialogs
    //     RCache.ClearFrameGraphContext();
    // }
    //
    // For now, just clear to black (no UI rendering yet)

    Msg("  [MenuUIPass::Execute] Cleared rt_MenuMain to black (no legacy UI yet)");

    // ═══════════════════════════════════════════════════════
    //  END RENDER PASS
    // ═══════════════════════════════════════════════════════

    // Nothing more to do for now (Phase 6 will add UI rendering)

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_menuStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();
    m_menuStats.numDialogs = 0;  // TODO: Count from legacy UI system

    Msg("  [MenuUIPass] Execute complete (%.2f ms)", m_menuStats.cpuTimeMs);
}

} // namespace xray::render::passes
