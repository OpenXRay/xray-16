// xrRender/FrameGraphPasses/TextPass.cpp
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
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"

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

    // Create VCB pool for dynamic constant buffer management
    m_vcbPool = xr_make_unique<framegraph::VolatileConstantBufferPool>(device);

    // Create material cache (TextPass owns its own MaterialCache with VCB pool)
    m_materialCache = xr_make_unique<MaterialCache>(
        device,
        device->GetFGResourceManager(),  // Pass FGResourceManager for native texture loading
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
    // TextPass uses the same render targets as UIPass (renders on top)
    // No need to create or declare them - they're already registered
    Msg("  [TextPass::Setup] Registered pass with FrameGraph");
}

void TextPass::Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) {
    auto executeStart = std::chrono::high_resolution_clock::now();

    // Get command list for PIX marker
    nvrhi::ICommandList* cmdList = ctx.GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->beginMarker("TextPass");

    // ═══════════════════════════════════════════════════════
    //  GET PHYSICAL RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::ITexture* outputTexture = fg.GetPhysicalTexture(m_outputRT);
    nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(m_depthStencil);

    if (!outputTexture || !depthTexture) {
        Msg("! [TextPass::Execute] Failed to get physical textures");
        cmdList->endMarker();
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  INITIALIZE NVRHI RESOURCES (FIRST RUN)
    // ═══════════════════════════════════════════════════════

    if (!m_initialized) {
        Msg("  [TextPass] Initializing NVRHI resources...");

        nvrhi::IDevice* nvrhiDevice = m_device->GetNVRHIDevice();
        VERIFY(nvrhiDevice != nullptr);

        // ───────────────────────────────────────────────────────
        //  1. CREATE DYNAMIC VERTEX BUFFER
        // ───────────────────────────────────────────────────────
        // Max 16384 vertices = 4096 quads = ~4096 characters
        constexpr u32 MAX_TEXT_VERTICES = 16384;

        nvrhi::BufferDesc vbDesc;
        vbDesc.byteSize = MAX_TEXT_VERTICES * sizeof(TextVertex);
        vbDesc.structStride = 0;  // 0 for regular vertex buffers (stride is per-binding)
        vbDesc.debugName = "TextPass Vertex Buffer";
        vbDesc.canHaveUAVs = false;
        vbDesc.isVertexBuffer = true;
        vbDesc.isVolatile = true;  // Dynamic buffer (updated each frame via writeBuffer)
        // cpuAccess is implicit for volatile buffers - don't set it!

        m_vertexBuffer = nvrhiDevice->createBuffer(vbDesc);
        if (!m_vertexBuffer) {
            Msg("! [TextPass] Failed to create vertex buffer");
            cmdList->endMarker();
            return;
        }
        Msg("  [TextPass] Created dynamic vertex buffer (%u vertices, %u bytes)",
            MAX_TEXT_VERTICES, vbDesc.byteSize);

        // ───────────────────────────────────────────────────────
        //  2. CREATE STATIC INDEX BUFFER (QUAD PATTERN)
        // ───────────────────────────────────────────────────────
        // Generate repeating quad indices: 0,1,2, 1,3,2, 4,5,6, 5,7,6, ...
        constexpr u32 MAX_TEXT_INDICES = MAX_TEXT_VERTICES / 4 * 6;  // 6 indices per quad
        xr_vector<u16> quadIndices;
        quadIndices.reserve(MAX_TEXT_INDICES);

        for (u16 quadIdx = 0; quadIdx < MAX_TEXT_VERTICES / 4; quadIdx++) {
            u16 baseVertex = quadIdx * 4;
            // Triangle 1: v0, v1, v2
            quadIndices.push_back(baseVertex + 0);
            quadIndices.push_back(baseVertex + 1);
            quadIndices.push_back(baseVertex + 2);
            // Triangle 2: v1, v3, v2
            quadIndices.push_back(baseVertex + 1);
            quadIndices.push_back(baseVertex + 3);
            quadIndices.push_back(baseVertex + 2);
        }

        nvrhi::BufferDesc ibDesc;
        ibDesc.byteSize = MAX_TEXT_INDICES * sizeof(u16);
        ibDesc.debugName = "TextPass Index Buffer";
        ibDesc.canHaveUAVs = false;
        ibDesc.isIndexBuffer = true;
        ibDesc.isVolatile = false;  // Static buffer
        ibDesc.cpuAccess = nvrhi::CpuAccessMode::None;
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

        m_indexBuffer = nvrhiDevice->createBuffer(ibDesc);
        if (!m_indexBuffer) {
            Msg("! [TextPass] Failed to create index buffer");
            cmdList->endMarker();
            return;
        }

        // Upload quad indices to GPU
        cmdList->writeBuffer(m_indexBuffer, quadIndices.data(), quadIndices.size() * sizeof(u16));
        Msg("  [TextPass] Created static index buffer (%u indices, %u bytes)",
            MAX_TEXT_INDICES, ibDesc.byteSize);

        // ───────────────────────────────────────────────────────
        //  3. CREATE INPUT LAYOUT (VERTEX FORMAT)
        // ───────────────────────────────────────────────────────
        // Describe TextVertex layout for GPU:
        // struct TextVertex { float x,y,z; u32 color; float u,v; }
        //
        // This will be used when creating the graphics pipeline.
        // NVRHI doesn't have a separate "input layout" object like D3D11 -
        // instead, we pass vertex attributes to GraphicsPipelineDesc.

        // Use std::vector for NVRHI compatibility (allocator mismatch with xr_vector)
        std::vector<nvrhi::VertexAttributeDesc> vertexAttributes;
        vertexAttributes.reserve(3);

        // Attribute 0: Position (xyzw) at offset 0
        nvrhi::VertexAttributeDesc positionAttr;
        positionAttr.name = "POSITION";
        positionAttr.format = nvrhi::Format::RGBA32_FLOAT;  // float4 (xyzw)
        positionAttr.offset = 0;
        positionAttr.bufferIndex = 0;
        positionAttr.elementStride = sizeof(TextVertex);
        vertexAttributes.push_back(positionAttr);

        // Attribute 1: Color (RGBA as packed u32) at offset 12
        nvrhi::VertexAttributeDesc colorAttr;
        colorAttr.name = "COLOR";
        colorAttr.format = nvrhi::Format::RGBA8_UNORM;  // u32 RGBA -> float4 [0,1]
        colorAttr.offset = offsetof(TextVertex, color);
        colorAttr.bufferIndex = 0;
        colorAttr.elementStride = sizeof(TextVertex);
        vertexAttributes.push_back(colorAttr);

        // Attribute 2: TexCoord (uv) at offset 16
        nvrhi::VertexAttributeDesc texcoordAttr;
        texcoordAttr.name = "TEXCOORD";
        texcoordAttr.format = nvrhi::Format::RG32_FLOAT;
        texcoordAttr.offset = offsetof(TextVertex, u);
        texcoordAttr.bufferIndex = 0;
        texcoordAttr.elementStride = sizeof(TextVertex);
        vertexAttributes.push_back(texcoordAttr);

        Msg("  [TextPass] Input layout defined (%zu attributes, stride %u bytes)",
            vertexAttributes.size(), sizeof(TextVertex));

        // ───────────────────────────────────────────────────────
        //  4. CREATE CONSTANT BUFFER FOR screen_res
        // ───────────────────────────────────────────────────────
        // Create constant buffer for vertex shader (screen_res: width, height, 1/width, 1/height)

        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = 16;  // float4 (16 bytes)
        cbDesc.isConstantBuffer = true;
        cbDesc.debugName = "TextPass screen_res CB";
        cbDesc.isVolatile = true;  // Updated each frame
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

    // ═══════════════════════════════════════════════════════
    //  COLLECT TEXT GEOMETRY FROM FONTS
    // ═══════════════════════════════════════════════════════

    CollectTextGeometry();

    if (m_fontBatches.empty()) {
        // No text to render - fast path
        m_textStats.numStrings = 0;
        m_textStats.numCharacters = 0;

        auto executeEnd = std::chrono::high_resolution_clock::now();
        m_textStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

        cmdList->endMarker();
        Msg("  [TextPass] Execute complete (%.2f ms, no text)", m_textStats.cpuTimeMs);
        return;
    }

    // MaterialCache will handle PSO/binding creation on-demand per font
    // (No lazy initialization needed - MaterialCache creates PSOs as fonts are encountered)

    // ═══════════════════════════════════════════════════════
    //  BUILD BUFFERS (UPLOAD TO GPU)
    // ═══════════════════════════════════════════════════════

    BuildTextBuffers(cmdList);

    // ═══════════════════════════════════════════════════════
    //  RENDER TEXT VIA RENDERCONTEXT
    // ═══════════════════════════════════════════════════════

    RenderText(ctx, outputTexture, depthTexture);

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_textStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

    // Count total characters across all batches
    u32 totalVertices = 0;
    for (const auto& batch : m_fontBatches) {
        totalVertices += static_cast<u32>(batch.vertices.size());
    }
    m_textStats.numCharacters = totalVertices / 4;  // 4 verts per quad

    // ═══════════════════════════════════════════════════════
    //  CLEAR FONT STRINGS AFTER RENDERING
    // ═══════════════════════════════════════════════════════
    // We've collected and rendered the strings, now clear them
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

// ═══════════════════════════════════════════════════════
//  HELPER FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════

void TextPass::CollectTextGeometry() {
    m_fontBatches.clear();

    extern ENGINE_API Fvector2 g_current_font_scale;

    // Check if UI system is available
    if (GEnv.isDedicatedServer || !GEnv.UI) {
        m_textStats.numStrings = 0;
        return;
    }

    CFontManager* fontMgr = GEnv.UI->m_pFontManager;
    if (!fontMgr || fontMgr->m_all_fonts.empty()) {
        m_textStats.numStrings = 0;
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  COLLECT GEOMETRY BATCHED BY FONT
    // ═══════════════════════════════════════════════════════
    // Each font gets its own batch so MaterialCache can use the correct
    // shader/texture combination per font

    for (auto fontPtrPtr : fontMgr->m_all_fonts) {
        CGameFont* font = *fontPtrPtr;
        if (!font || font->strings.empty()) continue;

        // ═══════════════════════════════════════════════════════
        //  VALIDATE FONT (Set texture size if not already set)
        // ═══════════════════════════════════════════════════════
        // This replicates what dxFontRender::OnRender does in vanilla
        if (!(font->uFlags & CGameFont::fsValid)) {
            auto* fontRender = static_cast<render_r4::dxFontRender*>(font->pFontRender);
            if (fontRender && fontRender->strTextureName.size() > 0) {
                // Load texture to get dimensions
                resources::FGResourceManager* resMgr = m_device->GetFGResourceManager();
                resources::TextureManager* texMgr = resMgr->GetTextureManager();
                ng::TextureHandle texHandle = texMgr->LoadTexture(fontRender->strTextureName.c_str());

                if (texHandle.IsValid()) {
                    const auto* texMetadata = texMgr->GetMetadata(texHandle);
                    if (texMetadata) {
                        Msg("! [TextPass DEBUG] Texture metadata: width=%u, height=%u, state=%s",
                            texMetadata->desc.width, texMetadata->desc.height,
                            TextureStateToString(texMetadata->state));

                        // Try to get actual texture dimensions from NVRHI texture
                        nvrhi::ITexture* nvrhiTex = texMgr->GetNVRHITexture(texHandle);
                        if (nvrhiTex) {
                            const nvrhi::TextureDesc& desc = nvrhiTex->getDesc();
                            font->vTS.set((int)desc.width, (int)desc.height);
                            font->fTCHeight = font->fHeight / float(font->vTS.y);
                            font->uFlags |= CGameFont::fsValid;
                            Msg("! [TextPass] Validated font: texture=%s, size=%dx%d",
                                fontRender->strTextureName.c_str(), font->vTS.x, font->vTS.y);
                        }
                        else {
                            Msg("! [TextPass] Failed to get NVRHI texture for font");
                        }
                    }
                }
            }
        }

        // Create new batch for this font
        FontBatch batch;
        batch.font = font;
        batch.numStrings = 0;
        u16 vertexIndex = 0;

        // ═══════════════════════════════════════════════════════
        //  PROCESS EACH STRING IN THIS FONT
        // ═══════════════════════════════════════════════════════
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

            // Handle alignment
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

            // Gradient support
            if (font->uFlags & CGameFont::fsGradient) {
                const u32 r = color_get_R(clr) / 2;
                const u32 g = color_get_G(clr) / 2;
                const u32 b = color_get_B(clr) / 2;
                const u32 a = color_get_A(clr);
                clr2 = color_rgba(r, g, b, a);
            }

#ifndef USE_DX9
            // Vertex shader will cancel DX9 correction
            X -= 0.5f;
            Y -= 0.5f;
            Y2 -= 0.5f;
#endif

            // ═══════════════════════════════════════════════════════
            //  BUILD QUADS FOR EACH CHARACTER
            // ═══════════════════════════════════════════════════════
            for (u16 j = 0; j < len; j++) {
                const Fvector& charTC = font->GetCharTC(
                    font->IsMultibyte() ? wsStr[1 + j] : (u16)(u8)str.string[j]
                );

                float scw = charTC.z * g_current_font_scale.x;
                float fTCWidth = charTC.z / font->vTS.x;

                if (!fis_zero(charTC.z)) {
                    float tu = (charTC.x / font->vTS.x);
                    float tv = (charTC.y / font->vTS.y);

                    // Pass pixel coordinates - vertex shader will transform using screen_res
                    // Shader expects: O.HPos.x = I.P.x * screen_res.z * 2 - 1
                    //                 O.HPos.y = (I.P.y * screen_res.w * 2 - 1) * -1

                    float x0 = X;
                    float y0 = Y2;          // Bottom-left
                    float x1 = X;
                    float y1 = Y;           // Top-left
                    float x2 = X + scw;
                    float y2 = Y2;          // Bottom-right
                    float x3 = X + scw;
                    float y3 = Y;           // Top-right

                    // Create 4 vertices for quad (pixel coords, color, UV)
                    TextVertex v0 = { x0, y0, 0.0f, 1.0f, clr2, tu,            tv + font->fTCHeight };
                    TextVertex v1 = { x1, y1, 0.0f, 1.0f, clr,  tu,            tv };
                    TextVertex v2 = { x2, y2, 0.0f, 1.0f, clr2, tu + fTCWidth, tv + font->fTCHeight };
                    TextVertex v3 = { x3, y3, 0.0f, 1.0f, clr,  tu + fTCWidth, tv };

                    batch.vertices.push_back(v0);
                    batch.vertices.push_back(v1);
                    batch.vertices.push_back(v2);
                    batch.vertices.push_back(v3);

                    // Create 6 indices for 2 triangles (quad)
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

        // Add this font's batch if it has geometry
        if (!batch.vertices.empty()) {
            m_fontBatches.push_back(std::move(batch));
        }
    }

    // Update stats
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
    // NOTE: We now render per-batch, so we'll upload vertices per-batch in RenderText()
    // (Each batch uploads its own vertices before drawing)

    // Index buffer is static and already uploaded during initialization
}

void TextPass::RenderText(ng::RenderContext& ctx, nvrhi::ITexture* outputTexture, nvrhi::ITexture* depthTexture) {
    nvrhi::ICommandList* cmdList = ctx.GetCommandList();

    // ═══════════════════════════════════════════════════════
    //  0. CREATE FRAMEBUFFER FOR MATERIALCACHE
    // ═══════════════════════════════════════════════════════
    // MaterialCache's GetOrCreateUIPSO needs a framebuffer for cache keying

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [TextPass::RenderText] Failed to create framebuffer");
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  1. BEGIN RENDER PASS
    // ═══════════════════════════════════════════════════════

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = outputTexture;
    passDesc.numRenderTargets = 1;
    passDesc.depthStencil = depthTexture;
    passDesc.clearColor = false;  // Don't clear - we're compositing on top of UIPass
    passDesc.clearDepth = false;

    ctx.BeginRenderPass(passDesc);

    // ═══════════════════════════════════════════════════════
    //  2. UPDATE CONSTANT BUFFER (screen_res)
    // ═══════════════════════════════════════════════════════

    // Prepare screen_res: (width, height, 1/width, 1/height)
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

    // ═══════════════════════════════════════════════════════
    //  3. SET VIEWPORT & SCISSOR (ONCE FOR ALL BATCHES)
    // ═══════════════════════════════════════════════════════

    ctx.SetViewport(0, 0, (float)m_config.width, (float)m_config.height);

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    // ═══════════════════════════════════════════════════════
    //  4. UPDATE GLOBAL CONSTANT BUFFERS
    // ═══════════════════════════════════════════════════════
    // Font shaders need static_globals for screen_res and other parameters

    StaticGlobals staticGlobalsCB = {};
    FillGlobalConstants(staticGlobalsCB);

    // Write static_globals to all font PSOs before rendering
    for (const auto& batch : m_fontBatches) {
        if (batch.vertices.empty()) continue;

        auto* fontRender = static_cast<render_r4::dxFontRender*>(batch.font->pFontRender);
        if (!fontRender || !fontRender->pShader) continue;

        Shader* shader = fontRender->pShader._get();
        if (!shader || !shader->E[0]) continue;

        MaterialPSO* pso = m_materialCache->GetOrCreateUIPSO(shader, 0, framebuffer);
        if (pso) {
            for (const auto& cbInfo : pso->constantBuffers) {
                if (cbInfo.name == "static_globals") {
                    u32 sizeToWrite = std::min<u32>(sizeof(StaticGlobals), cbInfo.size);
                    ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &staticGlobalsCB, sizeToWrite);
                    break;
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  5. RENDER EACH FONT BATCH
    // ═══════════════════════════════════════════════════════
    // Each font has its own shader/texture, so we need separate PSO/bindings

    u32 totalQuadsDrawn = 0;

    for (const auto& batch : m_fontBatches) {
        if (batch.vertices.empty()) continue;

        // ───────────────────────────────────────────────────────
        //  4.1 GET SHADER FROM FONT
        // ───────────────────────────────────────────────────────

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

        // ───────────────────────────────────────────────────────
        //  4.2 GET PSO FROM MATERIALCACHE
        // ───────────────────────────────────────────────────────
        // MaterialCache will create PSO for this shader+framebuffer combo

        MaterialPSO* pso = m_materialCache->GetOrCreateUIPSO(shader, 0, framebuffer);
        if (!pso) {
            Msg("! [TextPass] Failed to get PSO for font batch, skipping");
            continue;
        }

        // ───────────────────────────────────────────────────────
        //  4.3 GET BINDING SETS FROM MATERIALCACHE
        // ───────────────────────────────────────────────────────
        // MaterialCache extracts textures/samplers and creates binding sets

        m_materialCache->GetOrCreateBindingSet(pso, m_constantBuffer, pso->pass);

        if (!pso->vsBindingSet || !pso->psBindingSet) {
            Msg("! [TextPass] Failed to create binding sets for font batch, skipping");
            continue;
        }

        // ───────────────────────────────────────────────────────
        //  4.4 UPLOAD VERTICES FOR THIS BATCH
        // ───────────────────────────────────────────────────────

        const size_t dataSize = batch.vertices.size() * sizeof(TextVertex);
        cmdList->writeBuffer(m_vertexBuffer, batch.vertices.data(), dataSize);

        // ───────────────────────────────────────────────────────
        //  4.5 SET PIPELINE STATE
        // ───────────────────────────────────────────────────────

        ctx.SetPipeline(pso->pso->GetNativePipeline());

        // ───────────────────────────────────────────────────────
        //  4.6 BIND VERTEX/INDEX BUFFERS
        // ───────────────────────────────────────────────────────

        ctx.SetVertexBuffer(0, m_vertexBuffer.Get(), 0);
        ctx.SetIndexBuffer(m_indexBuffer.Get(), nvrhi::Format::R16_UINT, 0);

        // ───────────────────────────────────────────────────────
        //  4.7 BIND RESOURCES (CRITICAL FIX!)
        // ───────────────────────────────────────────────────────
        // Bind BOTH per-stage binding sets:
        // Slot 0: VS binding set (VS constant buffers)
        // Slot 1: PS binding set (PS constant buffers + textures + samplers)
        // This is the fix for the texture/sampler binding regression!

        ctx.SetBindingSet(0, pso->vsBindingSet.Get());
        ctx.SetBindingSet(1, pso->psBindingSet.Get());

        // ───────────────────────────────────────────────────────
        //  4.8 DRAW
        // ───────────────────────────────────────────────────────

        const u32 numQuads = static_cast<u32>(batch.vertices.size() / 4);
        const u32 numIndices = numQuads * 6;

        ctx.DrawIndexed(numIndices, 0, 0);

        totalQuadsDrawn += numQuads;
    }

    // ═══════════════════════════════════════════════════════
    //  5. END RENDER PASS
    // ═══════════════════════════════════════════════════════

    ctx.EndRenderPass();

    Msg("  [TextPass::RenderText] Drew %u quads across %u font batches",
        totalQuadsDrawn, (u32)m_fontBatches.size());
}

} // namespace xray::render::passes
