#include "stdafx.h"
#include "TextPass.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrUICore/ui_base.h"
#include "xrUICore/FontManager/FontManager.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/SH_Atomic.h"
#include "Layers/xrRender/dxFontRender.h"
#include "xrCore/Text/StringConversion.hpp"

namespace xray::render::passes {

using namespace xray::render::RENDER_NAMESPACE;

TextPass::TextPass(ng::RenderDevice* device, const TextPassConfig& config)
    : m_device(device)
    , m_config(config)
    , m_textStats{}
    , m_outputRT{}
    , m_depthStencil{}
{
    VERIFY(m_device != nullptr);

    Msg("* [TextPass] Created (resolution: %ux%u)", config.width, config.height);

    m_vcbPool = xr_make_unique<framegraph::VolatileConstantBufferPool>(device);

    m_materialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),
        m_vcbPool.get()
    );

    Msg("  [TextPass] MaterialCache and VCB pool created");
}

TextPass::~TextPass() {
    Msg("* [TextPass] Destroyed");
}

void TextPass::SetOutputs(framegraph::VirtualResourceHandle uiMain, framegraph::VirtualResourceHandle depth) {
    m_outputRT = uiMain;
    m_depthStencil = depth;
}

void TextPass::Setup(framegraph::FrameGraph& fg) {
    Msg("  [TextPass::Setup] Registered pass with FrameGraph");
}

void TextPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("TextPass");

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [TextPass::Execute] Failed to get physical textures");
        cmdList->endMarker();
        return;
    }

    if (!m_initialized) {
        Msg("  [TextPass] Initializing NVRHI resources...");

        nvrhi::IDevice* nvrhiDevice = m_device->GetNVRHIDevice();
        VERIFY(nvrhiDevice != nullptr);

        constexpr u32 MAX_TEXT_VERTICES = 16384;

        nvrhi::BufferDesc vbDesc;
        vbDesc.byteSize = MAX_TEXT_VERTICES * sizeof(TextVertex);
        vbDesc.structStride = 0;
        vbDesc.debugName = "TextPass Vertex Buffer";
        vbDesc.canHaveUAVs = false;
        vbDesc.isVertexBuffer = true;
        vbDesc.isVolatile = true;

        m_vertexBuffer = nvrhiDevice->createBuffer(vbDesc);
        if (!m_vertexBuffer) {
            Msg("! [TextPass] Failed to create vertex buffer");
            cmdList->endMarker();
            return;
        }
        Msg("  [TextPass] Created dynamic vertex buffer (%u vertices, %u bytes)",
            MAX_TEXT_VERTICES, vbDesc.byteSize);

        constexpr u32 MAX_TEXT_INDICES = MAX_TEXT_VERTICES / 4 * 6;
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
        ibDesc.canHaveUAVs = false;
        ibDesc.isIndexBuffer = true;
        ibDesc.isVolatile = false;
        ibDesc.cpuAccess = nvrhi::CpuAccessMode::None;
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

        m_indexBuffer = nvrhiDevice->createBuffer(ibDesc);
        if (!m_indexBuffer) {
            Msg("! [TextPass] Failed to create index buffer");
            cmdList->endMarker();
            return;
        }

        cmdList->writeBuffer(m_indexBuffer, quadIndices.data(), quadIndices.size() * sizeof(u16));
        Msg("  [TextPass] Created static index buffer (%u indices, %u bytes)",
            MAX_TEXT_INDICES, ibDesc.byteSize);

        std::vector<nvrhi::VertexAttributeDesc> vertexAttributes;
        vertexAttributes.reserve(3);

        nvrhi::VertexAttributeDesc positionAttr;
        positionAttr.name = "POSITION";
        positionAttr.format = nvrhi::Format::RGBA32_FLOAT;
        positionAttr.offset = 0;
        positionAttr.bufferIndex = 0;
        positionAttr.elementStride = sizeof(TextVertex);
        vertexAttributes.push_back(positionAttr);

        nvrhi::VertexAttributeDesc colorAttr;
        colorAttr.name = "COLOR";
        colorAttr.format = nvrhi::Format::RGBA8_UNORM;
        colorAttr.offset = offsetof(TextVertex, color);
        colorAttr.bufferIndex = 0;
        colorAttr.elementStride = sizeof(TextVertex);
        vertexAttributes.push_back(colorAttr);

        nvrhi::VertexAttributeDesc texcoordAttr;
        texcoordAttr.name = "TEXCOORD";
        texcoordAttr.format = nvrhi::Format::RG32_FLOAT;
        texcoordAttr.offset = offsetof(TextVertex, u);
        texcoordAttr.bufferIndex = 0;
        texcoordAttr.elementStride = sizeof(TextVertex);
        vertexAttributes.push_back(texcoordAttr);

        Msg("  [TextPass] Input layout defined (%zu attributes, stride %u bytes)",
            vertexAttributes.size(), sizeof(TextVertex));

        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = 16;
        cbDesc.isConstantBuffer = true;
        cbDesc.debugName = "TextPass screen_res CB";
        cbDesc.isVolatile = true;
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        m_constantBuffer = nvrhiDevice->createBuffer(cbDesc);
        if (!m_constantBuffer) {
            Msg("! [TextPass] Failed to create constant buffer");
            cmdList->endMarker();
            return;
        }

        Msg("  [TextPass] Created constant buffer for screen_res (16 bytes)");

        m_initialized = true;
        Msg("  [TextPass] NVRHI buffer/layout/CB initialization complete");
    }

    CollectTextGeometry();

    if (m_fontBatches.empty()) {
        m_textStats.numStrings = 0;
        m_textStats.numCharacters = 0;

        auto executeEnd = std::chrono::high_resolution_clock::now();
        m_textStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

        cmdList->endMarker();
        Msg("  [TextPass] Execute complete (%.2f ms, no text)", m_textStats.cpuTimeMs);
        return;
    }

    BuildTextBuffers(cmdList);

    RenderText(ctx, outputTexture, depthTexture);

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_textStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    u32 totalVertices = 0;
    for (const auto& batch : m_fontBatches) {
        totalVertices += static_cast<u32>(batch.vertices.size());
    }
    m_textStats.numCharacters = totalVertices / 4;

    if (!GEnv.isDedicatedServer) {
        for (auto fontPtrPtr : GEnv.UI->m_pFontManager->m_all_fonts) {
            CGameFont* font = *fontPtrPtr;
            if (font) {
                font->strings.clear();
            }
        }
    }

    cmdList->endMarker();
    Msg("  [TextPass] Execute complete (%.2f ms, %u strings, %u chars)",
        m_textStats.cpuTimeMs, m_textStats.numStrings, m_textStats.numCharacters);
}

void TextPass::CollectTextGeometry() {
    m_fontBatches.clear();

    extern ENGINE_API Fvector2 g_current_font_scale;

    if (GEnv.isDedicatedServer || !GEnv.UI) {
        m_textStats.numStrings = 0;
        return;
    }

    CFontManager* fontMgr = GEnv.UI->m_pFontManager;
    if (!fontMgr || fontMgr->m_all_fonts.empty()) {
        m_textStats.numStrings = 0;
        return;
    }

    for (auto fontPtrPtr : fontMgr->m_all_fonts) {
        CGameFont* font = *fontPtrPtr;
        if (!font || font->strings.empty()) continue;

        FontBatch batch;
        batch.font = font;
        batch.numStrings = 0;
        u16 vertexIndex = 0;

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

            const u32 clr = str.c;
            u32 clr2 = str.c;

            if (font->uFlags & CGameFont::fsGradient) {
                const u32 r = color_get_R(clr) / 2;
                const u32 g = color_get_G(clr) / 2;
                const u32 b = color_get_B(clr) / 2;
                const u32 a = color_get_A(clr);
                clr2 = color_rgba(r, g, b, a);
            }

#ifndef USE_DX9
            X -= 0.5f;
            Y -= 0.5f;
            Y2 -= 0.5f;
#endif

            for (u16 j = 0; j < len; j++) {
                const Fvector& charTC = font->GetCharTC(
                    font->IsMultibyte() ? wsStr[1 + j] : (u16)(u8)str.string[j]
                );

                float scw = charTC.z * g_current_font_scale.x;
                float fTCWidth = charTC.z / font->vTS.x;

                if (!fis_zero(charTC.z)) {
                    float tu = (charTC.x / font->vTS.x);
                    float tv = (charTC.y / font->vTS.y);

                    float x0 = X;
                    float y0 = Y2;
                    float x1 = X;
                    float y1 = Y;
                    float x2 = X + scw;
                    float y2 = Y2;
                    float x3 = X + scw;
                    float y3 = Y;

                    TextVertex v0 = { x0, y0, 0.0f, 1.0f, clr2, tu,            tv + font->fTCHeight };
                    TextVertex v1 = { x1, y1, 0.0f, 1.0f, clr,  tu,            tv };
                    TextVertex v2 = { x2, y2, 0.0f, 1.0f, clr2, tu + fTCWidth, tv + font->fTCHeight };
                    TextVertex v3 = { x3, y3, 0.0f, 1.0f, clr,  tu + fTCWidth, tv };

                    batch.vertices.push_back(v0);
                    batch.vertices.push_back(v1);
                    batch.vertices.push_back(v2);
                    batch.vertices.push_back(v3);

                    batch.indices.push_back(vertexIndex + 0);
                    batch.indices.push_back(vertexIndex + 1);
                    batch.indices.push_back(vertexIndex + 2);

                    batch.indices.push_back(vertexIndex + 1);
                    batch.indices.push_back(vertexIndex + 3);
                    batch.indices.push_back(vertexIndex + 2);

                    vertexIndex += 4;
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
            m_fontBatches.push_back(std::move(batch));
        }
    }

    u32 totalStrings = 0;
    u32 totalVertices = 0;
    u32 totalIndices = 0;
    for (const auto& batch : m_fontBatches) {
        totalStrings += batch.numStrings;
        totalVertices += static_cast<u32>(batch.vertices.size());
        totalIndices += static_cast<u32>(batch.indices.size());
    }

    m_textStats.numStrings = totalStrings;

    if (totalVertices > 0) {
        Msg("  [TextPass::CollectTextGeometry] Collected %u batches: %u vertices, %u indices for %u strings",
            (u32)m_fontBatches.size(), totalVertices, totalIndices, totalStrings);
    }
}


void TextPass::BuildTextBuffers(nvrhi::ICommandList* cmdList) {

}

void TextPass::RenderText(ng::RenderContext& ctx, nvrhi::ITexture* outputTexture, nvrhi::ITexture* depthTexture) {
    nvrhi::ICommandList* cmdList = ctx.GetCommandList();

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [TextPass::RenderText] Failed to create framebuffer");
        return;
    }

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = outputTexture;
    passDesc.numRenderTargets = 1;
    passDesc.depthStencil = depthTexture;
    passDesc.clearColor = false;
    passDesc.clearDepth = false;

    ctx.BeginRenderPass(passDesc);

    struct ScreenResCB {
        float width;
        float height;
        float invWidth;
        float invHeight;
    };

    ScreenResCB cbData;
    cbData.width = static_cast<float>(m_config.width);
    cbData.height = static_cast<float>(m_config.height);
    cbData.invWidth = 1.0f / cbData.width;
    cbData.invHeight = 1.0f / cbData.height;

    ctx.WriteBuffer(m_constantBuffer.Get(), &cbData, sizeof(ScreenResCB));

    ctx.SetViewport(0, 0, (float)m_config.width, (float)m_config.height);

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    u32 totalQuadsDrawn = 0;

    for (const auto& batch : m_fontBatches) {
        if (batch.vertices.empty()) continue;

        auto* fontRender = static_cast<render_r4::dxFontRender*>(batch.font->pFontRender);
        if (!fontRender || !fontRender->pShader) {
            Msg("! [TextPass] Font batch has no shader, skipping");
            continue;
        }

        Shader* shader = fontRender->pShader._get();
        if (!shader || !shader->E[0]) {
            Msg("! [TextPass] Shader has no elements, skipping");
            continue;
        }

        MaterialPSO* pso = m_materialCache->GetOrCreateUIPSO(shader, 0, framebuffer);
        if (!pso) {
            Msg("! [TextPass] Failed to get PSO for font batch, skipping");
            continue;
        }

        m_materialCache->GetOrCreateBindingSet(pso, m_constantBuffer, pso->pass);

        if (!pso->vsBindingSet || !pso->psBindingSet) {
            Msg("! [TextPass] Failed to create binding sets for font batch, skipping");
            continue;
        }

        const size_t dataSize = batch.vertices.size() * sizeof(TextVertex);
        cmdList->writeBuffer(m_vertexBuffer, batch.vertices.data(), dataSize);

        ctx.SetPipeline(pso->pso->GetNativePipeline());

        ctx.SetVertexBuffer(0, m_vertexBuffer.Get(), 0);
        ctx.SetIndexBuffer(m_indexBuffer.Get(), nvrhi::Format::R16_UINT, 0);

        ctx.SetBindingSet(0, pso->vsBindingSet.Get());
        ctx.SetBindingSet(1, pso->psBindingSet.Get());

        const u32 numQuads = static_cast<u32>(batch.vertices.size() / 4);
        const u32 numIndices = numQuads * 6;

        ctx.DrawIndexed(numIndices, 0, 0);

        totalQuadsDrawn += numQuads;
    }

    ctx.EndRenderPass();

    Msg("  [TextPass::RenderText] Drew %u quads across %u font batches",
        totalQuadsDrawn, (u32)m_fontBatches.size());
}

}
