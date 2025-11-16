// xrRender/FrameGraphPasses/MenuDistortPass.cpp
#include "stdafx.h"
#include "MenuDistortPass.h"
#include "xrEngine/IGame_Persistent.h"

namespace xray::render::passes {

MenuDistortPass::MenuDistortPass(const MenuDistortPassConfig& config)
    : m_device(device)
    , m_config(config)
    , m_menuDistortStats{}
    , m_outputRT{}
    , m_depthStencil{}
{
    VERIFY(m_device != nullptr);
    Msg("* [MenuDistortPass] Created (resolution: %ux%u)", config.width, config.height);
}

MenuDistortPass::~MenuDistortPass() {
    Msg("* [MenuDistortPass] Destroyed");
}

void MenuDistortPass::SetOutputs(framegraph::VirtualResourceHandle menuDistort, framegraph::VirtualResourceHandle depth) {
    m_outputRT = menuDistort;
    m_depthStencil = depth;
}

void MenuDistortPass::Setup(framegraph::FrameGraph& fg) {
    // MenuDistortPass uses externally-created resources (rt_MenuDistort, rt_Depth)
    // No need to create or declare them - they're already registered in BuildFrameGraphStructure()
    Msg("  [MenuDistortPass::Setup] Registered pass with FrameGraph");
}

void MenuDistortPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    // Get command list for PIX marker
    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("UIDistortPass");

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [MenuDistortPass::Execute] Failed to get physical textures");
        cmdList->endMarker();
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  BEGIN RENDER PASS
    // ═══════════════════════════════════════════════════════

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = GEnv.FrameGraphRenderer->GetRenderDevice()->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [MenuDistortPass::Execute] Failed to create framebuffer");
        cmdList->endMarker();
        return;
    }

    // Simple clear operation
    cmdList->open();

    // Clear render target to neutral distortion (127, 127, 0, 127)
    // This represents "no distortion" in the distortion map
    cmdList->clearTextureFloat(outputTexture, nvrhi::AllSubresources,
        nvrhi::Color(m_config.clearColor[0], m_config.clearColor[1],
                     m_config.clearColor[2], m_config.clearColor[3]));

    cmdList->close();
    GEnv.FrameGraphRenderer->GetRenderDevice()->GetNVRHIDevice()->executeCommandList(cmdList);

    // ═══════════════════════════════════════════════════════
    //  RENDER DISTORTION EFFECTS (BRIDGE CODE - Phase 6)
    // ═══════════════════════════════════════════════════════
    // TODO: Phase 6 will add the bridge to call legacy distortion rendering:
    //
    // if (g_pGamePersistent) {
    //     RCache.SetFrameGraphContext(&ctx);  // Inject RenderContext into legacy system
    //     g_pGamePersistent->OnRenderPPUI_PP();  // Render post-process UI effects
    //     RCache.ClearFrameGraphContext();
    // }
    //
    // For now, just clear to neutral distortion

    Msg("  [MenuDistortPass::Execute] Cleared rt_MenuDistort to neutral (no legacy distortion yet)");

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_menuDistortStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();
    m_menuDistortStats.numEffects = 0;  // TODO: Count from legacy UI system

    cmdList->endMarker();
    Msg("  [MenuDistortPass] Execute complete (%.2f ms)", m_menuDistortStats.cpuTimeMs);
}

} // namespace xray::render::passes
