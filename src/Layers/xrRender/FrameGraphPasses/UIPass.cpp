// xrRender/FrameGraphPasses/UIPass.cpp
#include "stdafx.h"
#include "UIPass.h"
#include "xrEngine/IGame_Persistent.h"
#include "Layers/xrRender/UIRenderCollector.h"
#include "Layers/xrRender/NVRHIUIRenderer.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"

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
        // Initialize NVRHI UI renderer on first use
        if (!m_nvrhiUIInitialized) {
            m_uiRenderer->Initialize(m_device, m_materialCache.get());
            m_nvrhiUIInitialized = true;
        }

        // STEP 1: Collect UI geometry by temporarily swapping GEnv.UIRender
        IUIRender* oldRenderer = GEnv.UIRender;
        m_uiCollector->Clear();
        GEnv.UIRender = m_uiCollector.get();

        // ═══════════════════════════════════════════════════════
        //  COLLECT MAIN MENU UI (menus, options, credits, etc.)
        // ═══════════════════════════════════════════════════════
        g_pGamePersistent->OnRenderPPUI_main();

        // ═══════════════════════════════════════════════════════
        //  COLLECT IN-GAME UI (HUD, inventory, etc.)
        // ═══════════════════════════════════════════════════════
        // This is a completely separate rendering path from main menu!
        // OnRenderInGameUI() calls HUD()->RenderUI() which renders:
        // - Health/stamina bars
        // - Inventory UI
        // - Quest log
        // - Pause menu
        // - Dialog windows
        // - etc.
        g_pGamePersistent->OnRenderInGameUI();

        // ═══════════════════════════════════════════════════════
        //  COLLECT LOADING SCREEN UI (level loading backgrounds)
        // ═══════════════════════════════════════════════════════
        // Loading screen shows during level loading/transitions
        if (g_pGamePersistent->IsLoadingScreenShown()) {
            g_pGamePersistent->load_draw_internal();
        }

        // ═══════════════════════════════════════════════════════
        //  COLLECT UI SEQUENCER GEOMETRY (intro videos, tutorials)
        // ═══════════════════════════════════════════════════════
        // Sequencers render .ogm video intros (GSC/THQ logos, game intro)
        // and in-game tutorials. They emit geometry through GEnv.UIRender.
        g_pGamePersistent->OnRenderSequencers();

        // NOTE: Cursor is no longer collected here!
        // Cursor rendering has been moved to CursorPass (which executes after TextPass)
        // This ensures cursor renders on top of all UI elements AND text/fonts

        // Restore original renderer
        GEnv.UIRender = oldRenderer;

        // STEP 2: Update global constant buffers before rendering
        // UI shaders need static_globals for screen_res and other parameters
        if (!m_uiCollector->GetBatches().empty()) {
            StaticGlobals staticGlobalsCB = {};
            FillGlobalConstants(staticGlobalsCB);

            // Write static_globals to all UI PSOs
            // NOTE: We need to do this BEFORE rendering, while OUTSIDE the render pass
            for (const auto& batch : m_uiCollector->GetBatches()) {
                if (batch.shader && m_materialCache) {
                    MaterialPSO* matPSO = m_materialCache->GetOrCreateUIPSO(
                        batch.shader._get(),
                        batch.shaderElement,
                        framebuffer
                    );

                    if (matPSO) {
                        for (const auto& cbInfo : matPSO->constantBuffers) {
                            if (cbInfo.name == "static_globals") {
                                u32 sizeToWrite = std::min<u32>(sizeof(StaticGlobals), cbInfo.size);
                                ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &staticGlobalsCB, sizeToWrite);
                                break;  // Only need to update once per PSO
                            }
                        }
                    }
                }
            }

            // STEP 3: Render collected geometry via NVRHI
            m_uiRenderer->RenderBatches(
                cmdList,
                m_uiCollector->GetBatches(),
                framebuffer,
                m_config.width,
                m_config.height
            );

            m_uiStats.numBatches = static_cast<u32>(m_uiCollector->GetBatches().size());
        } else {
            m_uiStats.numBatches = 0;
        }
    } else {
        m_uiStats.numBatches = 0;
    }

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_uiStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    cmdList->endMarker();
}

} // namespace xray::render::passes
