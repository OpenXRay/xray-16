#include "stdafx.h"
#include "CursorPass.h"
#include "xrEngine/IGame_Persistent.h"
#include "Layers/xrRender/UIRenderCollector.h"
#include "Layers/xrRender/NVRHIUIRenderer.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"

namespace xray::render::passes {

CursorPass::CursorPass(const CursorPassConfig& config)
    : m_device(device)
    , m_config(config)
    , m_cursorStats{}
    , m_outputRT{}
    , m_depthStencil{}
{
    

    m_vcbPool = xr_make_unique<framegraph::VolatileConstantBufferPool>(device);

    m_materialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_vcbPool.get()
    );

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
    Msg("  [CursorPass::Setup] Registered pass with FrameGraph");
}

void CursorPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("CursorPass");

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [CursorPass::Execute] Failed to get physical textures");
        cmdList->endMarker();
        return;
    }

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = GEnv.FrameGraphRenderer->GetRenderDevice()->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [CursorPass::Execute] Failed to create framebuffer");
        cmdList->endMarker();
        return;
    }

    if (g_pGamePersistent) {
        if (!m_nvrhiUIInitialized) {
            m_uiRenderer->Initialize(m_materialCache.get());
            m_nvrhiUIInitialized = true;
        }

        IUIRender* oldRenderer = GEnv.UIRender;
        m_uiCollector->Clear();
        GEnv.UIRender = m_uiCollector.get();

        g_pGamePersistent->OnRenderCursor();

        GEnv.UIRender = oldRenderer;

        if (!m_uiCollector->GetBatches().empty()) {
            StaticGlobals staticGlobalsCB = {};
            FillGlobalConstants(staticGlobalsCB);

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

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_cursorStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    cmdList->endMarker();
}

}
