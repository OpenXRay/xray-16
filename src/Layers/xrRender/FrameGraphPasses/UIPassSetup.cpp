#include "stdafx.h"
#include "UIPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/UIRenderCollector.h"
#include "Layers/xrRender/NVRHIUIRenderer.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/GameFont.h"
#include "xrUICore/ui_base.h"
#include "xrUICore/FontManager/FontManager.h"
#include "Layers/xrRender/dxFontRender.h"
#include "xrCore/Text/StringConversion.hpp"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/SH_Atomic.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"

namespace xray::render::RENDER_NAMESPACE::passes {

using RENDER_NAMESPACE::dxFontRender;
using namespace xray::render::fgconstants;

static void UploadStaticGlobals(FGConstantSystem& constants, const StaticGlobals& cb) {

    Fmatrix m_V;
    m_V._11 = cb.m_V[0];  m_V._12 = cb.m_V[4];  m_V._13 = cb.m_V[8];   m_V._14 = 0.0f;
    m_V._21 = cb.m_V[1];  m_V._22 = cb.m_V[5];  m_V._23 = cb.m_V[9];   m_V._24 = 0.0f;
    m_V._31 = cb.m_V[2];  m_V._32 = cb.m_V[6];  m_V._33 = cb.m_V[10];  m_V._34 = 0.0f;
    m_V._41 = cb.m_V[3];  m_V._42 = cb.m_V[7];  m_V._43 = cb.m_V[11];  m_V._44 = 1.0f;
    constants.SetStatic("m_V", m_V);

    Fmatrix m_P;
    m_P._11 = cb.m_P[0];  m_P._12 = cb.m_P[4];  m_P._13 = cb.m_P[8];   m_P._14 = cb.m_P[12];
    m_P._21 = cb.m_P[1];  m_P._22 = cb.m_P[5];  m_P._23 = cb.m_P[9];   m_P._24 = cb.m_P[13];
    m_P._31 = cb.m_P[2];  m_P._32 = cb.m_P[6];  m_P._33 = cb.m_P[10];  m_P._34 = cb.m_P[14];
    m_P._41 = cb.m_P[3];  m_P._42 = cb.m_P[7];  m_P._43 = cb.m_P[11];  m_P._44 = cb.m_P[15];
    constants.SetStatic("m_P", m_P);

    Fmatrix m_VP;
    m_VP._11 = cb.m_VP[0];  m_VP._12 = cb.m_VP[4];  m_VP._13 = cb.m_VP[8];   m_VP._14 = cb.m_VP[12];
    m_VP._21 = cb.m_VP[1];  m_VP._22 = cb.m_VP[5];  m_VP._23 = cb.m_VP[9];   m_VP._24 = cb.m_VP[13];
    m_VP._31 = cb.m_VP[2];  m_VP._32 = cb.m_VP[6];  m_VP._33 = cb.m_VP[10];  m_VP._34 = cb.m_VP[14];
    m_VP._41 = cb.m_VP[3];  m_VP._42 = cb.m_VP[7];  m_VP._43 = cb.m_VP[11];  m_VP._44 = cb.m_VP[15];
    constants.SetStatic("m_VP", m_VP);

    constants.SetStatic("timers", cb.timers);

    constants.SetStatic("fog_plane", cb.fog_plane);
    constants.SetStatic("fog_params", cb.fog_params);
    constants.SetStatic("fog_color", cb.fog_color);

    constants.SetStatic("L_ambient", cb.L_ambient);
    Fvector4 L_sun_color(cb.L_sun_color.x, cb.L_sun_color.y, cb.L_sun_color.z, 0.0f);
    constants.SetStatic("L_sun_color", L_sun_color);
    Fvector4 L_sun_dir_w(cb.L_sun_dir_w.x, cb.L_sun_dir_w.y, cb.L_sun_dir_w.z, 0.0f);
    constants.SetStatic("L_sun_dir_w", L_sun_dir_w);
    constants.SetStatic("L_hemi_color", cb.L_hemi_color);

    Fvector4 eye_pos(cb.eye_position.x, cb.eye_position.y, cb.eye_position.z, 0.0f);
    constants.SetStatic("eye_position", eye_pos);

    constants.SetStatic("pos_decompression_params", cb.pos_decompression_params);
    constants.SetStatic("pos_decompression_params2", cb.pos_decompression_params2);

    constants.SetStatic("parallax", cb.parallax);

    constants.SetStatic("screen_res", cb.screen_res);
}

framegraph::VirtualResourceHandle setupUIPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneTarget,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    struct UIPassData {
        VirtualResourceHandle sceneInput;
        VirtualResourceHandle sceneOutput;
        u32 width;
        u32 height;
    };

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
           ng::RenderContext* ctx) {

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

            auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.FrameGraphRenderer);
            auto* uiCollector = fgRenderer->GetUICollector();
            auto* uiRenderer = fgRenderer->GetUIRenderer();
            auto* uiMatCache = fgRenderer->GetUIMaterialCache();

            if (!uiCollector || !uiRenderer || !uiMatCache) {
                Msg("! [UIPass] UI infrastructure not initialized");
                return;
            }

            g_pGamePersistent->OnRenderPPUI_main();
            g_pGamePersistent->OnRenderInGameUI();
            if (g_pGamePersistent->IsLoadingScreenShown()) {
                g_pGamePersistent->load_draw_internal();
            }
            g_pGamePersistent->OnRenderSequencers();

            if (!uiCollector->GetBatches().empty()) {
                StaticGlobals staticGlobalsCB = {};
                FillGlobalConstants(staticGlobalsCB);

                for (const auto& batch : uiCollector->GetBatches()) {
                    if (batch.uiShader && uiMatCache) {
                        MaterialPSO* matPSO = uiMatCache->GetOrCreateUIPSO(
                            batch.uiShader,
                            batch.shaderElement,
                            framebuffer
                        );

                        if (matPSO) {
                            FGConstantSystem constants(matPSO);
                            UploadStaticGlobals(constants, staticGlobalsCB);
                            constants.CommitStatic(ctx);
                        }
                    }
                }

                uiRenderer->RenderBatches(cmdList, uiCollector->GetBatches(), framebuffer, data.width, data.height);
            }
        }
    );

    return passData.sceneOutput;
}

framegraph::VirtualResourceHandle setupTextPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    struct TextPassData {
        VirtualResourceHandle uiTarget;
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<TextPassData>(
        "Text",

        [uiTarget, width, height](FrameGraph& builder, PassHandle passHandle, TextPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;

            data.uiTarget = passBuilder.readWrite(uiTarget, ResourceState::RenderTarget);
        },

        [](const TextPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            auto* uiRT = fg.GetPhysicalTexture(data.uiTarget);

            if (!uiRT) {
                Msg("! [TextPass] Failed to get UI texture");
                return;
            }

            auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.FrameGraphRenderer);
            auto* device = fgRenderer->GetRenderDevice();
            auto* textMatCache = fgRenderer->GetTextMaterialCache();

            if (!device || !textMatCache) {
                Msg("! [TextPass] Text infrastructure not initialized");
                return;
            }
            extern ENGINE_API Fvector2 g_current_font_scale;

            static nvrhi::BufferHandle s_vertexBuffer;
            static nvrhi::BufferHandle s_indexBuffer;
            static nvrhi::BufferHandle s_constantBuffer;
            static bool s_initialized = false;

            if (!s_initialized) {
                constexpr u32 MAX_TEXT_VERTICES = 16384;
                constexpr u32 MAX_TEXT_INDICES = MAX_TEXT_VERTICES / 4 * 6;

                nvrhi::IDevice* nvrhiDevice = device->GetNVRHIDevice();

                nvrhi::BufferDesc vbDesc;
                vbDesc.byteSize = MAX_TEXT_VERTICES * 24;
                vbDesc.debugName = "TextPass Vertex Buffer";
                vbDesc.isVertexBuffer = true;
                vbDesc.isVolatile = false;
                vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
                s_vertexBuffer = nvrhiDevice->createBuffer(vbDesc);

                xr_vector<u16> quadIndices;
                quadIndices.reserve(MAX_TEXT_INDICES);
                for (u16 quadIdx = 0; quadIdx < MAX_TEXT_VERTICES / 4; quadIdx++) {
                    u16 baseVertex = quadIdx * 4;
                    quadIndices.push_back(baseVertex + 0);
                    quadIndices.push_back(baseVertex + 1);
                    quadIndices.push_back(baseVertex + 2);
                    quadIndices.push_back(baseVertex + 1);
                    quadIndices.push_back(baseVertex + 3);
                    quadIndices.push_back(baseVertex + 2);
                }

                nvrhi::BufferDesc ibDesc;
                ibDesc.byteSize = MAX_TEXT_INDICES * sizeof(u16);
                ibDesc.debugName = "TextPass Index Buffer";
                ibDesc.isIndexBuffer = true;
                ibDesc.isVolatile = false;
                ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
                s_indexBuffer = nvrhiDevice->createBuffer(ibDesc);

                cmdList->beginTrackingBufferState(s_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);
                cmdList->beginTrackingBufferState(s_indexBuffer, nvrhi::ResourceStates::IndexBuffer);

                cmdList->writeBuffer(s_indexBuffer, quadIndices.data(), quadIndices.size() * sizeof(u16));

                nvrhi::BufferDesc cbDesc;
                cbDesc.byteSize = 16;
                cbDesc.isConstantBuffer = true;
                cbDesc.debugName = "TextPass screen_res CB";
                cbDesc.isVolatile = true;
                cbDesc.maxVersions = 16;
                cbDesc.keepInitialState = true;
                cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
                s_constantBuffer = nvrhiDevice->createBuffer(cbDesc);

                s_initialized = true;
                Msg("* [TextPass] Initialized buffers");
            }

            cmdList->beginTrackingBufferState(s_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);
            cmdList->beginTrackingBufferState(s_indexBuffer, nvrhi::ResourceStates::IndexBuffer);

            struct TextVertex {
                float x, y, z, w;
                u32 color;
                float u, v;
            };

            struct FontBatch {
                CGameFont* font;
                xr_vector<TextVertex> vertices;
                u32 numStrings;
            };

            xr_vector<FontBatch> fontBatches;

            if (!GEnv.isDedicatedServer && GEnv.UI) {
                CFontManager* fontMgr = GEnv.UI->m_pFontManager;
                if (fontMgr) {
                    for (auto fontPtrPtr : fontMgr->m_all_fonts) {
                        CGameFont* font = *fontPtrPtr;
                        if (!font || font->strings.empty()) continue;

                        if (!(font->uFlags & CGameFont::fsValid)) {
                            auto* fontRender = static_cast<dxFontRender*>(font->pFontRender);
                            if (fontRender && fontRender->m_textureName.size() > 0) {
                                auto* resMgr = device->GetFGResourceManager();
                                auto* texMgr = resMgr->GetTextureManager();
                                ng::TextureHandle texHandle = texMgr->LoadTexture(fontRender->m_textureName.c_str());
                                if (texHandle.IsValid()) {
                                    nvrhi::ITexture* nvrhiTex = texMgr->GetNVRHITexture(texHandle);
                                    if (nvrhiTex) {
                                        const nvrhi::TextureDesc& desc = nvrhiTex->getDesc();
                                        font->vTS.set((int)desc.width, (int)desc.height);
                                        font->fTCHeight = font->fHeight / float(font->vTS.y);
                                        font->uFlags |= CGameFont::fsValid;
                                    }
                                }
                            }
                        }

                        FontBatch batch;
                        batch.font = font;
                        batch.numStrings = 0;

                        for (const auto& str : font->strings) {
                            xr_wide_char wsStr[MAX_MB_CHARS];
                            const u16 len = font->IsMultibyte()
                                ? mbhMulti2Wide(wsStr, nullptr, MAX_MB_CHARS, str.string)
                                : xr_strlen(str.string);

                            if (len == 0) continue;

                            float X = float(iFloor(str.x));
                            float Y = float(iFloor(str.y));
                            float S = str.height * g_current_font_scale.y;
                            float Y2 = Y + S;

                            float fSize = 0;
                            if (str.align) {
                                fSize = font->IsMultibyte() ? font->SizeOf_(wsStr) : font->SizeOf_(str.string);
                            }

                            switch (str.align) {
                                case CGameFont::alCenter: X -= (iFloor(fSize * 0.5f)) * g_current_font_scale.x; break;
                                case CGameFont::alRight: X -= iFloor(fSize); break;
                            }

                            u32 clr = str.c;
                            u32 clr2 = str.c;

                            if (font->uFlags & CGameFont::fsGradient) {
                                const u32 r = color_get_R(clr) / 2;
                                const u32 g = color_get_G(clr) / 2;
                                const u32 b = color_get_B(clr) / 2;
                                const u32 a = color_get_A(clr);
                                clr2 = color_rgba(r, g, b, a);
                            }

                            X -= 0.5f;
                            Y -= 0.5f;
                            Y2 -= 0.5f;

                            for (u16 j = 0; j < len; j++) {
                                const Fvector& charTC = font->GetCharTC(
                                    font->IsMultibyte() ? wsStr[1 + j] : (u16)(u8)str.string[j]
                                );

                                float scw = charTC.z * g_current_font_scale.x;
                                float fTCWidth = charTC.z / font->vTS.x;

                                if (!fis_zero(charTC.z)) {
                                    float tu = (charTC.x / font->vTS.x);
                                    float tv = (charTC.y / font->vTS.y);

                                    TextVertex v0 = { X, Y2, 0.0f, 1.0f, clr2, tu, tv + font->fTCHeight };
                                    TextVertex v1 = { X, Y, 0.0f, 1.0f, clr, tu, tv };
                                    TextVertex v2 = { X + scw, Y2, 0.0f, 1.0f, clr2, tu + fTCWidth, tv + font->fTCHeight };
                                    TextVertex v3 = { X + scw, Y, 0.0f, 1.0f, clr, tu + fTCWidth, tv };

                                    batch.vertices.push_back(v0);
                                    batch.vertices.push_back(v1);
                                    batch.vertices.push_back(v2);
                                    batch.vertices.push_back(v3);
                                }

                                X += scw * font->vInterval.x;
                                if (font->IsMultibyte()) {
                                    X -= 2;
                                    if (IsNeedSpaceCharacter(wsStr[1 + j]))
                                        X += font->fXStep;
                                }
                            }

                            batch.numStrings++;
                        }

                        if (!batch.vertices.empty()) {
                            fontBatches.push_back(std::move(batch));
                        }
                    }
                }
            }

            if (fontBatches.empty()) {
                return;
            }

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(uiRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            ng::RenderPassDesc passDesc;
            passDesc.passName = "UI Pass";
            passDesc.renderTargets[0] = uiRT;
            passDesc.numRenderTargets = 1;
            passDesc.clearColor = false;
            ctx->BeginRenderPass(passDesc);

            struct ScreenResCB {
                float width, height, invWidth, invHeight;
            };
            ScreenResCB cbData;
            cbData.width = (float)data.width;
            cbData.height = (float)data.height;
            cbData.invWidth = 1.0f / cbData.width;
            cbData.invHeight = 1.0f / cbData.height;
            ctx->WriteBuffer(s_constantBuffer.Get(), &cbData, sizeof(ScreenResCB));

            ctx->SetViewport(0, 0, (float)data.width, (float)data.height);

            ng::Rect scissor;
            scissor.x = 0;
            scissor.y = 0;
            scissor.width = data.width;
            scissor.height = data.height;
            ctx->SetScissor(scissor);

            StaticGlobals staticGlobalsCB = {};
            FillGlobalConstants(staticGlobalsCB);

            for (const auto& batch : fontBatches) {
                auto* fontRender = static_cast<dxFontRender*>(batch.font->pFontRender);
                if (!fontRender) continue;

                if (!fontRender->m_vsHandle || !fontRender->m_psHandle) {
                    Msg("! [TextPass] Font shader not loaded for font");
                    continue;
                }

                MaterialPSO* pso = textMatCache->GetOrCreateFontPSO(fontRender, framebuffer);
                if (!pso) {
                    Msg("! [TextPass] Failed to create PSO for font");
                    continue;
                }

                FGConstantSystem constants(pso);
                UploadStaticGlobals(constants, staticGlobalsCB);
                constants.CommitStatic(ctx);

                textMatCache->GetOrCreateBindingSet(pso);

                if (!pso->vsBindingSet || !pso->psBindingSet) {
                    Msg("! [TextPass] Binding sets not created");
                    continue;
                }

                cmdList->writeBuffer(s_vertexBuffer, batch.vertices.data(), batch.vertices.size() * sizeof(TextVertex));

                cmdList->setBufferState(s_vertexBuffer, nvrhi::ResourceStates::VertexBuffer);

                ctx->SetPipeline(pso->pso->GetNativePipeline());
                ctx->SetVertexBuffer(0, s_vertexBuffer.Get(), 0);
                ctx->SetIndexBuffer(s_indexBuffer.Get(), nvrhi::Format::R16_UINT, 0);
                ctx->SetBindingSet(0, pso->vsBindingSet.Get());
                ctx->SetBindingSet(1, pso->psBindingSet.Get());

                const u32 numQuads = static_cast<u32>(batch.vertices.size() / 4);
                const u32 numIndices = numQuads * 6;
                ctx->DrawIndexed(numIndices, 0, 0);
            }

            ctx->EndRenderPass();

            if (!GEnv.isDedicatedServer && GEnv.UI) {
                CFontManager* fontMgr = GEnv.UI->m_pFontManager;
                if (fontMgr) {
                    for (auto fontPtrPtr : fontMgr->m_all_fonts) {
                        CGameFont* font = *fontPtrPtr;
                        if (font) {
                            font->strings.clear();
                        }
                    }
                }
            }
        }
    );

    return passData.uiTarget;
}

framegraph::VirtualResourceHandle setupCursorPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    struct CursorPassData {
        VirtualResourceHandle uiTarget;
        u32 width;
        u32 height;
    };

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
           ng::RenderContext* ctx) {

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

            auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.FrameGraphRenderer);
            auto* uiCollector = fgRenderer->GetUICollector();
            auto* uiRenderer = fgRenderer->GetUIRenderer();
            auto* uiMatCache = fgRenderer->GetUIMaterialCache();

            if (!uiCollector || !uiRenderer || !uiMatCache) {
                Msg("! [CursorPass] UI infrastructure not initialized");
                return;
            }

            IUIRender* oldRenderer = GEnv.UIRender;
            uiCollector->Clear();
            GEnv.UIRender = uiCollector;

            g_pGamePersistent->OnRenderCursor();

            GEnv.UIRender = oldRenderer;

            if (!uiCollector->GetBatches().empty()) {
                StaticGlobals staticGlobalsCB = {};
                FillGlobalConstants(staticGlobalsCB);

                for (const auto& batch : uiCollector->GetBatches()) {
                    if (batch.uiShader && uiMatCache) {
                        MaterialPSO* matPSO = uiMatCache->GetOrCreateUIPSO(
                            batch.uiShader,
                            batch.shaderElement,
                            framebuffer
                        );

                        if (matPSO) {
                            FGConstantSystem constants(matPSO);
                            UploadStaticGlobals(constants, staticGlobalsCB);
                            constants.CommitStatic(ctx);
                        }
                    }
                }

                uiRenderer->RenderBatches(cmdList, uiCollector->GetBatches(), framebuffer, data.width, data.height);
            }
        }
    );

    return passData.uiTarget;
}

}
