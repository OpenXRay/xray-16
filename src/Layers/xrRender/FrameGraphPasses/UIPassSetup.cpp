#include "stdafx.h"

#include "UIPassSetup.h"

#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/fgUIRender.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrUICore/ui_base.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/SH_Atomic.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::fg::passes {

static void UploadStaticGlobals(nvrhi::ICommandList* cmdList, MaterialPSO* matPSO, const StaticGlobals& cb)
{
    if (!cmdList || !matPSO)
        return;
    for (auto& cbInfo : matPSO->constantBuffers)
    {
        if (!cbInfo.nvrhiBuffer)
            continue;
        if (cbInfo.name != "static_globals")
            continue;
        const u32 bytes = std::min(cbInfo.size, (u32)sizeof(StaticGlobals));
        if (bytes == 0)
            continue;
        cmdList->writeBuffer(cbInfo.nvrhiBuffer, &cb, bytes);
    }
}

static fg::PrimitiveTopology GetBatchTopology(const ui::UIGeometryBatch& batch)
{
    switch (batch.primitiveType)
    {
    case ui::UIPrimitiveType::LineList:
        return fg::PrimitiveTopology::LineList;
    case ui::UIPrimitiveType::LineStrip:
        return fg::PrimitiveTopology::LineStrip;
    case ui::UIPrimitiveType::TriStrip:
        return fg::PrimitiveTopology::TriangleStrip;
    case ui::UIPrimitiveType::TriList:
    default:
        return fg::PrimitiveTopology::TriangleList;
    }
}

framegraph::VirtualResourceHandle setupUIPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneTarget,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<UIPassData>(
        "UI",

        [sceneTarget, width, height](FrameGraph& builder, PassHandle passHandle, UIPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;

            data.sceneInput = passBuilder.read(sceneTarget);
            data.sceneOutput = passBuilder.write(sceneTarget, ResourceState::RenderTarget);
        },

        [](const UIPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            auto* sceneRT = fg.GetPhysicalTexture(data.sceneOutput);

            if (!sceneRT) {
                return;
            }

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(sceneRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            if (!g_pGamePersistent) {
                Msg("* [UIPass] No GamePersistent");
                return;
            }

            auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
            auto* uiRender = fgRenderer->GetUIRender();
            auto* uiMatCache = fgRenderer->GetUIMaterialCache();

            if (!uiRender || !uiMatCache) {
                Msg("! [UIPass] UI infrastructure not initialized");
                return;
            }

            g_pGamePersistent->OnRenderPPUI_main();
            g_pGamePersistent->OnRenderPPUI_PP();
            g_pGamePersistent->OnRenderInGameUI();
            if (g_pGamePersistent->IsLoadingScreenShown()) {
                g_pGamePersistent->load_draw_internal();
            }
            g_pGamePersistent->OnRenderSequencers();

            if (!uiRender->GetBatches().empty()) {
                StaticGlobals staticGlobalsCB = {};
                FillGlobalConstants(staticGlobalsCB);
                const float uiW = float(std::max(1u, data.width));
                const float uiH = float(std::max(1u, data.height));
                staticGlobalsCB.screen_res.set(uiW, uiH, 1.0f / uiW, 1.0f / uiH);

                for (const auto& batch : uiRender->GetBatches()) {
                    if (batch.uiShader && uiMatCache) {
                        MaterialPSO* matPSO = uiMatCache->GetOrCreateUIPSO(
                            batch.uiShader,
                            batch.shaderElement,
                            framebuffer,
                            GetBatchTopology(batch)
                        );
                        if (matPSO)
                            UploadStaticGlobals(cmdList, matPSO, staticGlobalsCB);
                    }
                }

                uiRender->Draw(cmdList, framebuffer, data.width, data.height);
            }
        }
    );

    return passData.sceneOutput;
}

framegraph::VirtualResourceHandle setupCursorPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<CursorPassData>(
        "Cursor",

        [uiTarget, width, height](FrameGraph& builder, PassHandle passHandle, CursorPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;

            data.uiTarget = passBuilder.readWrite(uiTarget, ResourceState::RenderTarget);
        },

        [](const CursorPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            auto* uiRT = fg.GetPhysicalTexture(data.uiTarget);

            if (!uiRT) {
                Msg("! [CursorPass] Failed to get UI texture");
                return;
            }

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(uiRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            if (!g_pGamePersistent) {
                Msg("* [CursorPass] No GamePersistent");
                return;
            }

            auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
            auto* uiRender = fgRenderer->GetUIRender();
            auto* uiMatCache = fgRenderer->GetUIMaterialCache();

            if (!uiRender || !uiMatCache) {
                Msg("! [CursorPass] UI infrastructure not initialized");
                return;
            }

            IUIRender* oldRenderer = GEnv.UIRender;
            uiRender->Clear();
            GEnv.UIRender = uiRender;

            g_pGamePersistent->OnRenderCursor();

            GEnv.UIRender = oldRenderer;

            if (!uiRender->GetBatches().empty()) {
                StaticGlobals staticGlobalsCB = {};
                FillGlobalConstants(staticGlobalsCB);
                const float uiW = float(std::max(1u, data.width));
                const float uiH = float(std::max(1u, data.height));
                staticGlobalsCB.screen_res.set(uiW, uiH, 1.0f / uiW, 1.0f / uiH);

                for (const auto& batch : uiRender->GetBatches()) {
                    if (batch.uiShader && uiMatCache) {
                        MaterialPSO* matPSO = uiMatCache->GetOrCreateUIPSO(
                            batch.uiShader,
                            batch.shaderElement,
                            framebuffer,
                            GetBatchTopology(batch)
                        );
                        if (matPSO)
                            UploadStaticGlobals(cmdList, matPSO, staticGlobalsCB);
                    }
                }

                uiRender->Draw(cmdList, framebuffer, data.width, data.height);
            }
        }
    );

    return passData.uiTarget;
}

}
