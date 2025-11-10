// xrRender/FrameGraphPasses/MenuCompositePass.cpp
#include "stdafx.h"
#include "MenuCompositePass.h"

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

void MenuCompositePass::SetInputs(framegraph::VirtualResourceHandle menuMain, framegraph::VirtualResourceHandle menuDistort) {
    m_inputMenuMain = menuMain;
    m_inputMenuDistort = menuDistort;
}

void MenuCompositePass::SetOutput(framegraph::VirtualResourceHandle finalOutput) {
    m_outputRT = finalOutput;
}

void MenuCompositePass::Setup(framegraph::FrameGraph& fg) {
    // MenuCompositePass uses externally-created resources
    // Inputs: rt_MenuMain, rt_MenuDistort (read-only)
    // Output: final output RT (write)
    Msg("  [MenuCompositePass::Setup] Registered pass with FrameGraph");
}

void MenuCompositePass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    // Get command list for PIX marker
    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("UICompositePass");

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* menuMainTexture = fg.GetPhysicalTexture(m_inputMenuMain);
    nvrhi::ITexture* menuDistortTexture = fg.GetPhysicalTexture(m_inputMenuDistort);
    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);

    if (!menuMainTexture || !menuDistortTexture || !outputTexture) {
        Msg("! [MenuCompositePass::Execute] Failed to get physical textures");
        cmdList->endMarker();
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  SIMPLE COPY COMPOSITE (Phase 5 stub)
    // ═══════════════════════════════════════════════════════
    // For now, just copy rt_MenuMain to output (ignore distortion)
    // Phase 6 will implement proper s_menu shader compositing with distortion
    //
    // TODO: Implement fullscreen quad rendering with s_menu shader:
    // 1. Load s_menu shader (samples t0=MenuMain, t1=MenuDistort)
    // 2. Create fullscreen quad geometry (or use screen-aligned triangle)
    // 3. Bind MenuMain to sampler 0, MenuDistort to sampler 1
    // 4. Render fullscreen quad with shader
    // 5. Output composited result to finalOutput

    Msg("  [MenuCompositePass::Execute] Copying rt_MenuMain to output (no shader composite yet)");

    // Simple copy for now
    ctx.CopyTexture(outputTexture, menuMainTexture);

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_menuCompositeStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    cmdList->endMarker();
    Msg("  [MenuCompositePass] Execute complete (%.2f ms)", m_menuCompositeStats.cpuTimeMs);
}

} // namespace xray::render::passes
