// xrRender/FrameGraphPasses/UIPassSetup.cpp
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
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"

namespace xray::render::fg::passes {

using namespace xray::render::fgconstants;

static void UploadStaticGlobals(FGConstantSystem& constants, const StaticGlobals& cb) {
    constants.SetStatic("m_V", cb.m_V);
    constants.SetStatic("m_P", cb.m_P);
    constants.SetStatic("m_VP", cb.m_VP);
    constants.SetStatic("timers", cb.timers);
    constants.SetStatic("fog_plane", cb.fog_plane);
    constants.SetStatic("fog_params", cb.fog_params);
    constants.SetStatic("fog_color", cb.fog_color);
    constants.SetStatic("L_ambient", cb.L_ambient);
    constants.SetStatic("L_sun_color", Fvector4(cb.L_sun_color.x, cb.L_sun_color.y, cb.L_sun_color.z, 0.0f));
    constants.SetStatic("L_sun_dir_w", Fvector4(cb.L_sun_dir_w.x, cb.L_sun_dir_w.y, cb.L_sun_dir_w.z, 0.0f));
    constants.SetStatic("L_hemi_color", cb.L_hemi_color);
    constants.SetStatic("eye_position", Fvector4(cb.eye_position.x, cb.eye_position.y, cb.eye_position.z, 0.0f));
    constants.SetStatic("pos_decompression_params", cb.pos_decompression_params);
    constants.SetStatic("pos_decompression_params2", cb.pos_decompression_params2);
    constants.SetStatic("parallax", cb.parallax);
    constants.SetStatic("screen_res", cb.screen_res);
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

        // Setup lambda
        [sceneTarget, width, height](FrameGraph& builder, PassHandle passHandle, UIPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;

            // Read and write to scene HDR target (UI renders on top with alpha blending)
            data.sceneInput = passBuilder.read(sceneTarget);
            data.sceneOutput = passBuilder.write(sceneTarget, ResourceState::RenderTarget);
        },

        // Execute lambda
        [](const UIPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            // Get physical scene target
            auto* sceneRT = fg.GetPhysicalTexture(data.sceneOutput);

            if (!sceneRT) {
                return;
            }

            // Create framebuffer (no depth buffer - UI is screen-space overlay)
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(sceneRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            if (!g_pGamePersistent) {
                Msg("* [UIPass] No GamePersistent");
                return;
            }

            // Get UI infrastructure from FrameGraphRenderer
            auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
            auto* uiRender = fgRenderer->GetUIRender();
            auto* uiMatCache = fgRenderer->GetUIMaterialCache();

            if (!uiRender || !uiMatCache) {
                Msg("! [UIPass] UI infrastructure not initialized");
                return;
            }

            // Collect UI geometry
            g_pGamePersistent->OnRenderPPUI_main();
            g_pGamePersistent->OnRenderInGameUI();
            if (g_pGamePersistent->IsLoadingScreenShown()) {
                g_pGamePersistent->load_draw_internal();
            }
            g_pGamePersistent->OnRenderSequencers();

            if (!uiRender->GetBatches().empty()) {
                StaticGlobals staticGlobalsCB = {};
                FillGlobalConstants(staticGlobalsCB);

                // Upload static_globals using FGConstantSystem (type-safe, automatic VCB lookup)
                for (const auto& batch : uiRender->GetBatches()) {
                    if (batch.uiShader && uiMatCache) {
                        MaterialPSO* matPSO = uiMatCache->GetOrCreateUIPSO(
                            batch.uiShader,
                            batch.shaderElement,
                            framebuffer,
                            GetBatchTopology(batch)
                        );

                        if (matPSO) {
                            // Use FGConstantSystem with STATIC constant API
                            FGConstantSystem constants(matPSO);
                            UploadStaticGlobals(constants, staticGlobalsCB);
                            constants.CommitStatic(ctx);
                        }
                    }
                }

                uiRender->Draw(cmdList, framebuffer, data.width, data.height);
            }
        }
    );

    return passData.sceneOutput;
}

// Cursor pass - renders cursor on top of everything
framegraph::VirtualResourceHandle setupCursorPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<CursorPassData>(
        "Cursor",

        // Setup lambda
        [uiTarget, width, height](FrameGraph& builder, PassHandle passHandle, CursorPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;

            // Read-write UI target (cursor renders on top)
            data.uiTarget = passBuilder.readWrite(uiTarget, ResourceState::RenderTarget);
        },

        // Execute lambda
        [](const CursorPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            // Get physical resource
            auto* uiRT = fg.GetPhysicalTexture(data.uiTarget);

            if (!uiRT) {
                Msg("! [CursorPass] Failed to get UI texture");
                return;
            }

            // Clear framebuffer (no need, cursor renders on top of existing UI)
            // Create framebuffer
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(uiRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            if (!g_pGamePersistent) {
                Msg("* [CursorPass] No GamePersistent");
                return;
            }

            // Get UI infrastructure from FrameGraphRenderer
            auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
            auto* uiRender = fgRenderer->GetUIRender();
            auto* uiMatCache = fgRenderer->GetUIMaterialCache();

            if (!uiRender || !uiMatCache) {
                Msg("! [CursorPass] UI infrastructure not initialized");
                return;
            }

            // Collect cursor geometry
            IUIRender* oldRenderer = GEnv.UIRender;
            uiRender->Clear();
            GEnv.UIRender = uiRender;

            // Call cursor rendering callback
            g_pGamePersistent->OnRenderCursor();

            GEnv.UIRender = oldRenderer;

            // Render cursor batches if any were collected
            if (!uiRender->GetBatches().empty()) {
                StaticGlobals staticGlobalsCB = {};
                FillGlobalConstants(staticGlobalsCB);

                // Upload static_globals using FGConstantSystem (type-safe, automatic VCB lookup)
                for (const auto& batch : uiRender->GetBatches()) {
                    if (batch.uiShader && uiMatCache) {
                        MaterialPSO* matPSO = uiMatCache->GetOrCreateUIPSO(
                            batch.uiShader,
                            batch.shaderElement,
                            framebuffer,
                            GetBatchTopology(batch)
                        );

                        if (matPSO) {
                            // Use FGConstantSystem with STATIC constant API
                            FGConstantSystem constants(matPSO);
                            UploadStaticGlobals(constants, staticGlobalsCB);
                            constants.CommitStatic(ctx);
                        }
                    }
                }

                uiRender->Draw(cmdList, framebuffer, data.width, data.height);
            }
        }
    );

    return passData.uiTarget;
}

} // namespace xray::render::fg::passes
