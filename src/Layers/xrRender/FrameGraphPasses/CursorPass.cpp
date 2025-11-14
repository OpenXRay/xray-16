// xrRender/FrameGraphPasses/CursorPass.cpp
#include "stdafx.h"
#include "CursorPass.h"
#include "xrEngine/IGame_Persistent.h"
#include "Layers/xrRender/UIRenderCollector.h"
#include "Layers/xrRender/NVRHIUIRenderer.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"

namespace xray::render::passes {

CursorPass::CursorPass(ng::RenderDevice* device, const CursorPassConfig& config)
    : m_device(device)
    , m_config(config)
    , m_cursorStats{}
    , m_outputRT{}
    , m_depthStencil{}
{
    VERIFY(m_device != nullptr);

    // Create VCB pool for dynamic constant buffer management
    m_vcbPool = xr_make_unique<framegraph::VolatileConstantBufferPool>(device);

    // Create material cache (CursorPass owns its own MaterialCache with VCB pool)
    m_materialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),  // Pass FGResourceManager for native texture loading
        m_vcbPool.get()
    );

    // Create UI rendering infrastructure (reuse UIPass renderer)
    m_uiCollector = xr_make_unique<ui::UIRenderCollector>();
    m_uiRenderer = xr_make_unique<ui::NVRHIUIRenderer>();

    Msg("* [CursorPass] Created (resolution: %ux%u)", config.width, config.height);
}

CursorPass::~CursorPass() {
    Msg("* [CursorPass] Destroyed");
}

void CursorPass::SetOutputs(framegraph::VirtualResourceHandle uiMain, framegraph::VirtualResourceHandle depth) {
    m_outputRT = uiMain;
    m_depthStencil = depth;
}

void CursorPass::Setup(framegraph::FrameGraph& fg) {
    // CursorPass uses externally-created resources (rt_MenuMain, rt_Depth)
    // No need to create or declare them - they're already registered in BuildFrameGraphStructure()
    Msg("  [CursorPass::Setup] Registered pass with FrameGraph");
}

void CursorPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    // Get command list for PIX marker
    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("CursorPass");

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [CursorPass::Execute] Failed to get physical textures");
        cmdList->endMarker();
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE FRAMEBUFFER
    // ═══════════════════════════════════════════════════════

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [CursorPass::Execute] Failed to create framebuffer");
        cmdList->endMarker();
        return;
    }

    // NOTE: No clear operation - we're compositing on top of UIPass + TextPass

    // ═══════════════════════════════════════════════════════
    //  COLLECT CURSOR GEOMETRY
    // ═══════════════════════════════════════════════════════

    if (g_pGamePersistent) {
        // Initialize NVRHI UI renderer on first use
        if (!m_nvrhiUIInitialized) {
            m_uiRenderer->Initialize(m_device, m_materialCache.get());
            m_nvrhiUIInitialized = true;
        }

        // STEP 1: Collect cursor geometry by temporarily swapping GEnv.UIRender
        IUIRender* oldRenderer = GEnv.UIRender;
        m_uiCollector->Clear();
        GEnv.UIRender = m_uiCollector.get();

        // Collect cursor (must be last so it renders on top)
        g_pGamePersistent->OnRenderCursor();

        // Restore original renderer
        GEnv.UIRender = oldRenderer;

        // STEP 2: Update global constant buffers before rendering
        // Cursor shaders need static_globals for screen_res and other parameters
        if (!m_uiCollector->GetBatches().empty()) {
            StaticGlobals staticGlobalsCB = {};
            FillGlobalConstants(staticGlobalsCB);

            // Write static_globals to all cursor PSOs
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
                                break;
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

            Msg("  [CursorPass::Execute] NVRHI cursor rendering complete");
            m_cursorStats.numBatches = static_cast<u32>(m_uiCollector->GetBatches().size());
        } else {
            Msg("  [CursorPass::Execute] No cursor geometry collected");
            m_cursorStats.numBatches = 0;
        }
    } else {
        Msg("  [CursorPass::Execute] No GamePersistent - skipping cursor");
        m_cursorStats.numBatches = 0;
    }

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_cursorStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    cmdList->endMarker();
}

} // namespace xray::render::passes
