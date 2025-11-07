#include "stdafx.h"
#include "MenuDistortPass.h"
#include "xrEngine/IGame_Persistent.h"

namespace xray::render::passes {

MenuDistortPass::MenuDistortPass(ng::RenderDevice* device, const MenuDistortPassConfig& config)
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
    Msg("  [MenuDistortPass::Setup] Registered pass with FrameGraph");
}

void MenuDistortPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [MenuDistortPass::Execute] Failed to get physical textures");
        return;
    }

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [MenuDistortPass::Execute] Failed to create framebuffer");
        return;
    }

    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);

    cmdList->open();

    cmdList->clearTextureFloat(outputTexture, nvrhi::AllSubresources,
        nvrhi::Color(m_config.clearColor[0], m_config.clearColor[1],
                     m_config.clearColor[2], m_config.clearColor[3]));

    cmdList->close();
    m_device->GetNVRHIDevice()->executeCommandList(cmdList);

    Msg("  [MenuDistortPass::Execute] Cleared rt_MenuDistort to neutral (no legacy distortion yet)");

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_menuDistortStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();
    m_menuDistortStats.numEffects = 0;

    Msg("  [MenuDistortPass] Execute complete (%.2f ms)", m_menuDistortStats.cpuTimeMs);
}

}
