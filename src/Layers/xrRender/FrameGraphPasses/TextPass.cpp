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
        //  4. DEFER SHADER/PIPELINE CREATION
        // ───────────────────────────────────────────────────────
        // We will extract shaders from CGameFont during first CollectTextGeometry()
        // This allows us to use the ACTUAL shader the font uses, not a hardcoded one

        Msg("  [TextPass] Deferring shader/pipeline creation until first font is encountered");
        Msg("  [TextPass] Will extract shader from CGameFont->pFontRender->pShader");

        m_initialized = true;
        Msg("  [TextPass] NVRHI buffer/layout initialization complete");
    }

    // ═══════════════════════════════════════════════════════
    //  COLLECT TEXT GEOMETRY FROM FONTS
    // ═══════════════════════════════════════════════════════

    CollectTextGeometry();

    if (m_vertices.empty()) {
        // No text to render - fast path
        m_textStats.numStrings = 0;
        m_textStats.numCharacters = 0;

        auto executeEnd = std::chrono::high_resolution_clock::now();
        m_textStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();

        cmdList->endMarker();
        Msg("  [TextPass] Execute complete (%.2f ms, no text)", m_textStats.cpuTimeMs);
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  LAZY INITIALIZATION: Extract shader from first font
    // ═══════════════════════════════════════════════════════
    if (!m_pipelineReady && !GEnv.isDedicatedServer && GEnv.UI) {
        CFontManager* fontMgr = GEnv.UI->m_pFontManager;
        if (fontMgr && !fontMgr->m_all_fonts.empty()) {
            CGameFont* firstFont = *fontMgr->m_all_fonts[0];
            if (firstFont && !InitializeFromFont(firstFont, cmdList)) {
                Msg("! [TextPass] Failed to initialize from font shader");
                cmdList->endMarker();
                return;
            }
        }
    }

    // Pipeline must be ready to render
    if (!m_pipelineReady) {
        Msg("! [TextPass] Pipeline not ready - skipping render");
        cmdList->endMarker();
        return;
    }

    // ═══════════════════════════════════════════════════════
    //  BUILD BUFFERS (UPLOAD TO GPU)
    // ═══════════════════════════════════════════════════════

    BuildTextBuffers(cmdList);

    // ═══════════════════════════════════════════════════════
    //  RENDER TEXT VIA NVRHI
    // ═══════════════════════════════════════════════════════

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTexture);
    fbDesc.setDepthAttachment(depthTexture);

    nvrhi::FramebufferHandle framebuffer = m_device->GetNVRHIDevice()->createFramebuffer(fbDesc);
    if (!framebuffer) {
        Msg("! [TextPass::Execute] Failed to create framebuffer");
        cmdList->endMarker();
        return;
    }

    RenderText(cmdList, framebuffer);

    // ═══════════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════════

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_textStats.cpuTimeMs = std::chrono::duration<float, std::milli>(executeEnd - executeStart).count();
    m_textStats.numCharacters = static_cast<u32>(m_vertices.size() / 4);  // 4 verts per quad

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
    m_vertices.clear();
    m_indices.clear();

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

    u32 totalStrings = 0;
    u16 vertexIndex = 0;

    // ═══════════════════════════════════════════════════════
    //  ITERATE ALL REGISTERED FONTS
    // ═══════════════════════════════════════════════════════
    for (auto fontPtrPtr : fontMgr->m_all_fonts) {
        CGameFont* font = *fontPtrPtr;
        if (!font || font->strings.empty()) continue;

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

                    m_vertices.push_back(v0);
                    m_vertices.push_back(v1);
                    m_vertices.push_back(v2);
                    m_vertices.push_back(v3);

                    // Create 6 indices for 2 triangles (quad)
                    m_indices.push_back(vertexIndex + 0);
                    m_indices.push_back(vertexIndex + 1);
                    m_indices.push_back(vertexIndex + 2);

                    m_indices.push_back(vertexIndex + 1);
                    m_indices.push_back(vertexIndex + 3);
                    m_indices.push_back(vertexIndex + 2);

                    vertexIndex += 4;
                }

                X += scw * font->vInterval.x;
                if (font->IsMultibyte()) {
                    X -= 2;
                    if (IsNeedSpaceCharacter(wsStr[1 + j]))
                        X += font->fXStep;
                }
            }

            totalStrings++;
        }
    }

    m_textStats.numStrings = totalStrings;

    if (m_vertices.size() > 0) {
        Msg("  [TextPass::CollectTextGeometry] Collected %zu vertices, %zu indices for %u strings",
            m_vertices.size(), m_indices.size(), totalStrings);
    }
}

bool TextPass::InitializeFromFont(CGameFont* font, nvrhi::ICommandList* cmdList) {
    Msg("  [TextPass::InitializeFromFont] Extracting shader from CGameFont...");

    // ═══════════════════════════════════════════════════════
    //  STEP 1: Extract shader from font's renderer
    // ═══════════════════════════════════════════════════════

    auto* fontRender = static_cast<render_r4::dxFontRender*>(font->pFontRender);
    if (!fontRender || !fontRender->pShader) {
        Msg("! [TextPass] Font has no shader");
        return false;
    }

    Shader* shader = fontRender->pShader._get();
    if (!shader || !shader->E[0]) {
        Msg("! [TextPass] Shader has no elements");
        return false;
    }

    // Get first shader element (pass 0)
    ShaderElement* element = shader->E[0]._get();
    if (!element || !element->passes[0] || !element->passes[0]->ps) {
        Msg("! [TextPass] Shader has no pixel shader in pass 0");
        return false;
    }

    SPS* pixelShader = element->passes[0]->ps._get();
    if (!pixelShader || !pixelShader->bytecode) {
        Msg("! [TextPass] Pixel shader has no bytecode");
        return false;
    }

    Msg("    ✓ Extracted pixel shader: %s", pixelShader->cName.c_str());

    // ═══════════════════════════════════════════════════════
    //  STEP 2: Reflect pixel shader bytecode
    // ═══════════════════════════════════════════════════════

    m_shaderReflection = framegraph::ShaderReflector::AnalyzePixelShader(
        pixelShader->sh,      // ID3D11PixelShader*
        pixelShader->bytecode // ID3DBlob*
    );

    Msg("    ✓ Reflected shader: %u textures, %u samplers",
        (u32)m_shaderReflection.inputTextures.size(),
        (u32)m_shaderReflection.samplers.size());

    // ═══════════════════════════════════════════════════════
    //  STEP 3: Load font texture via FGResourceManager
    // ═══════════════════════════════════════════════════════

    // Get texture name directly from dxFontRender (stored during Initialize)
    if (fontRender->strTextureName.size() > 0) {
        const char* textureName = fontRender->strTextureName.c_str();
        Msg("    Font texture: %s", textureName);

        // Load texture via FGResourceManager
        resources::FGResourceManager* resMgr = m_device->GetFGResourceManager();
        if (!resMgr) {
            Msg("! [TextPass] FGResourceManager not available");
            return false;
        }

        resources::TextureManager* texMgr = resMgr->GetTextureManager();
        if (!texMgr) {
            Msg("! [TextPass] TextureManager not available");
            return false;
        }

        // Load texture by name
        m_fontTextureHandle = texMgr->LoadTexture(textureName);
        if (!m_fontTextureHandle.IsValid()) {
            Msg("! [TextPass] TextureManager failed to load texture: %s", textureName);
            Msg("! Handle index: %u, generation: %u", m_fontTextureHandle.index, m_fontTextureHandle.generation);
            return false;
        }

        Msg("      ✓ Loaded font texture via FGResourceManager: %s", textureName);
        Msg("        Handle: index=%u, gen=%u", m_fontTextureHandle.index, m_fontTextureHandle.generation);
    } else {
        Msg("! [TextPass] Font renderer has no texture name");
        return false;
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 4: Create binding layout from reflection
    //  (Deferred until after constant buffer analysis)
    // ═══════════════════════════════════════════════════════

    // ═══════════════════════════════════════════════════════
    //  STEP 4.5: Create samplers (once, cached for reuse)
    // ═══════════════════════════════════════════════════════

    m_samplers.clear();
    m_samplers.reserve(m_shaderReflection.samplers.size());

    for (const auto& samplerInfo : m_shaderReflection.samplers) {
        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllFilters(true);  // Linear filtering
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
        nvrhi::SamplerHandle sampler = m_device->GetNVRHIDevice()->createSampler(samplerDesc);
        m_samplers.push_back(sampler);
        Msg("    ✓ Created cached sampler for '%s' at slot s%u", samplerInfo.name.c_str(), samplerInfo.slot);
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 5: Create NVRHI shader handles from bytecode
    // ═══════════════════════════════════════════════════════

    nvrhi::IDevice* nvrhiDevice = m_device->GetNVRHIDevice();

    // Create pixel shader
    nvrhi::ShaderDesc psDesc;
    psDesc.shaderType = nvrhi::ShaderType::Pixel;
    psDesc.debugName = pixelShader->cName.c_str();

    m_pixelShader = nvrhiDevice->createShader(
        psDesc,
        pixelShader->bytecode->GetBufferPointer(),
        pixelShader->bytecode->GetBufferSize()
    );

    if (!m_pixelShader) {
        Msg("! [TextPass] Failed to create NVRHI pixel shader");
        return false;
    }

    // Create vertex shader
    SVS* vertexShader = element->passes[0]->vs._get();
    if (!vertexShader || !vertexShader->bytecode) {
        Msg("! [TextPass] Vertex shader has no bytecode");
        return false;
    }

    nvrhi::ShaderDesc vsDesc;
    vsDesc.shaderType = nvrhi::ShaderType::Vertex;
    vsDesc.debugName = vertexShader->cName.c_str();

    m_vertexShader = nvrhiDevice->createShader(
        vsDesc,
        vertexShader->bytecode->GetBufferPointer(),
        vertexShader->bytecode->GetBufferSize()
    );

    if (!m_vertexShader) {
        Msg("! [TextPass] Failed to create NVRHI vertex shader");
        return false;
    }

    Msg("    ✓ Created NVRHI shader handles (VS: %s, PS: %s)",
        vertexShader->cName.c_str(), pixelShader->cName.c_str());

    // ═══════════════════════════════════════════════════════
    //  STEP 6: Reflect vertex shader to get input signature
    // ═══════════════════════════════════════════════════════

    framegraph::VertexInputSignature vsSignature = framegraph::ShaderReflector::AnalyzeVertexShader(
        nullptr,  // Don't need ID3D11VertexShader*
        vertexShader->bytecode
    );

    if (vsSignature.elements.empty()) {
        Msg("! [TextPass] Vertex shader has no input elements");
        return false;
    }

    Msg("    ✓ VS expects %u input elements:", (u32)vsSignature.elements.size());
    for (const auto& elem : vsSignature.elements) {
        Msg("      - %s%u (format from shader)", elem.semanticName.c_str(), elem.semanticIndex);
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 6.5: Analyze vertex shader constant buffers
    // ═══════════════════════════════════════════════════════

    m_vsConstantBuffers = framegraph::ShaderReflector::AnalyzeConstantBuffers(vertexShader->bytecode);

    Msg("    ✓ VS has %u constant buffers:", (u32)m_vsConstantBuffers.buffers.size());
    for (const auto& cb : m_vsConstantBuffers.buffers) {
        Msg("      - %s: slot b%u, %u bytes", cb.name.c_str(), cb.slot, cb.size);

        // Create NVRHI constant buffer for this CB
        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = cb.size;
        cbDesc.isConstantBuffer = true;
        cbDesc.debugName = cb.name.c_str();
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        m_vsConstantBuffer = nvrhiDevice->createBuffer(cbDesc);
        if (!m_vsConstantBuffer) {
            Msg("! [TextPass] Failed to create constant buffer '%s'", cb.name.c_str());
        } else {
            Msg("        ✓ Created constant buffer");
        }
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 6.6: Create unified binding layout (PS + VS resources)
    // ═══════════════════════════════════════════════════════

    std::vector<nvrhi::BindingLayoutItem> bindingLayoutItems;

    // Add pixel shader textures
    for (const auto& tex : m_shaderReflection.inputTextures) {
        nvrhi::BindingLayoutItem item;
        item.slot = tex.slot;
        item.type = nvrhi::ResourceType::Texture_SRV;
        bindingLayoutItems.push_back(item);
    }

    // Add pixel shader samplers
    for (const auto& sampler : m_shaderReflection.samplers) {
        nvrhi::BindingLayoutItem item;
        item.slot = sampler.slot;
        item.type = nvrhi::ResourceType::Sampler;
        bindingLayoutItems.push_back(item);
    }

    // Add vertex shader constant buffers
    for (const auto& cb : m_vsConstantBuffers.buffers) {
        nvrhi::BindingLayoutItem item;
        item.slot = cb.slot;
        item.type = nvrhi::ResourceType::ConstantBuffer;
        bindingLayoutItems.push_back(item);
    }

    nvrhi::BindingLayoutDesc bindingLayoutDesc;
    bindingLayoutDesc.bindings = bindingLayoutItems;
    bindingLayoutDesc.visibility = nvrhi::ShaderType::All;  // Both VS and PS

    m_bindingLayout = nvrhiDevice->createBindingLayout(bindingLayoutDesc);
    if (!m_bindingLayout) {
        Msg("! [TextPass] Failed to create binding layout");
        return false;
    }

    Msg("    ✓ Created unified binding layout (%u bindings total)", (u32)bindingLayoutItems.size());

    // Convert to NVRHI vertex attributes with our offsets
    // NVRHI uses arraySize to handle multiple semantic indices!
    // If we have TEXCOORD0 and TEXCOORD1, we pass name="TEXCOORD" with arraySize=2
    // NVRHI will create D3D11 elements with SemanticIndex 0 and 1

    // Group attributes by semantic name
    xr_map<xr_string, xr_vector<const framegraph::VertexInputSignature::InputElement*>> semanticGroups;
    for (const auto& elem : vsSignature.elements) {
        semanticGroups[elem.semanticName.c_str()].push_back(&elem);
    }

    // For each semantic name, create one NVRHI attribute with arraySize
    std::vector<nvrhi::VertexAttributeDesc> vertexAttributes;
    vertexAttributes.reserve(semanticGroups.size());

    for (const auto& [semName, elems] : semanticGroups) {
        const auto* firstElem = elems[0];

        nvrhi::VertexAttributeDesc attr;
        attr.name = semName.c_str();
        attr.format = firstElem->format;
        attr.bufferIndex = 0;
        attr.elementStride = sizeof(TextVertex);
        attr.arraySize = (uint32_t)elems.size();  // Number of indices
        attr.isInstanced = false;

        // Map semantic to our TextVertex structure
        if (xr_strcmp(semName.c_str(), "POSITION") == 0 ||
            xr_strcmp(semName.c_str(), "POSITIONT") == 0) {
            attr.offset = 0;  // float4 xyzw
            attr.format = nvrhi::Format::RGBA32_FLOAT;
        } else if (xr_strcmp(semName.c_str(), "COLOR") == 0) {
            attr.offset = offsetof(TextVertex, color);  // u32
            attr.format = nvrhi::Format::RGBA8_UNORM;
        } else if (xr_strcmp(semName.c_str(), "TEXCOORD") == 0) {
            attr.offset = offsetof(TextVertex, u);  // float2 uv
            attr.format = nvrhi::Format::RG32_FLOAT;
        } else {
            Msg("! [TextPass] Unknown semantic: %s", semName.c_str());
            continue;
        }

        vertexAttributes.push_back(attr);
        Msg("      → Mapped %s (arraySize=%u) to offset %u",
            semName.c_str(), attr.arraySize, attr.offset);
    }

    nvrhi::GraphicsPipelineDesc pipelineDesc;

    // Shaders
    pipelineDesc.VS = m_vertexShader;
    pipelineDesc.PS = m_pixelShader;

    // Primitive topology
    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

    // Binding layout
    pipelineDesc.bindingLayouts = { m_bindingLayout };

    // Input layout
    pipelineDesc.inputLayout = m_device->GetNVRHIDevice()->createInputLayout(
        vertexAttributes.data(), (uint32_t)vertexAttributes.size(), m_vertexShader);

    if (!pipelineDesc.inputLayout) {
        Msg("! [TextPass] Failed to create input layout");
        return false;
    }

    // Rasterizer state
    pipelineDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    pipelineDesc.renderState.rasterState.fillMode = nvrhi::RasterFillMode::Fill;
    pipelineDesc.renderState.rasterState.frontCounterClockwise = false;
    pipelineDesc.renderState.rasterState.depthClipEnable = true;

    // Blend state (alpha blending)
    pipelineDesc.renderState.blendState.targets[0].blendEnable = true;
    pipelineDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
    pipelineDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::InvSrcAlpha;
    pipelineDesc.renderState.blendState.targets[0].blendOp = nvrhi::BlendOp::Add;
    pipelineDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
    pipelineDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;
    pipelineDesc.renderState.blendState.targets[0].blendOpAlpha = nvrhi::BlendOp::Add;
    pipelineDesc.renderState.blendState.targets[0].colorWriteMask = nvrhi::ColorMask::All;

    // Depth/stencil state
    pipelineDesc.renderState.depthStencilState.depthTestEnable = true;
    pipelineDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipelineDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Always;
    pipelineDesc.renderState.depthStencilState.stencilEnable = false;

    // Framebuffer info
    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.width = m_config.width;
    fbInfo.height = m_config.height;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
    fbInfo.depthFormat = nvrhi::Format::D24S8;
    fbInfo.sampleCount = 1;
    fbInfo.sampleQuality = 0;

    // Create pipeline
    m_pipeline = m_device->GetNVRHIDevice()->createGraphicsPipeline(pipelineDesc, fbInfo);
    if (!m_pipeline) {
        Msg("! [TextPass] Failed to create graphics pipeline");
        return false;
    }

    Msg("    ✓ Created graphics pipeline");

    m_pipelineReady = true;
    Msg("  [TextPass::InitializeFromFont] Lazy initialization complete!");
    return true;
}

void TextPass::BuildTextBuffers(nvrhi::ICommandList* cmdList) {
    // Upload vertex data to GPU (m_vertexBuffer is dynamic/volatile)
    if (!m_vertices.empty()) {
        const size_t dataSize = m_vertices.size() * sizeof(TextVertex);
        cmdList->writeBuffer(m_vertexBuffer, m_vertices.data(), dataSize);
        Msg("  [TextPass::BuildTextBuffers] Uploaded %u vertices (%zu bytes)",
            (u32)m_vertices.size(), dataSize);
    }

    // Index buffer was already uploaded during initialization (static)
}

void TextPass::RenderText(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer) {
    // ═══════════════════════════════════════════════════════
    //  0. UPDATE CONSTANT BUFFER (screen_res)
    // ═══════════════════════════════════════════════════════

    if (m_vsConstantBuffer) {
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

        cmdList->writeBuffer(m_vsConstantBuffer, &cbData, sizeof(ScreenResCB));
        Msg("  [TextPass::RenderText] Updated screen_res CB: (%g, %g, %g, %g)",
            cbData.width, cbData.height, cbData.invWidth, cbData.invHeight);
    }

    // ═══════════════════════════════════════════════════════
    //  1. SET GRAPHICS STATE
    // ═══════════════════════════════════════════════════════

    nvrhi::GraphicsState graphicsState;

    // Framebuffer (render target + depth)
    graphicsState.framebuffer = framebuffer;

    // Pipeline (shaders, blend, depth, rasterizer)
    graphicsState.pipeline = m_pipeline;

    // Viewport (full screen)
    nvrhi::Viewport viewport;
    viewport.minX = 0.0f;
    viewport.minY = 0.0f;
    viewport.maxX = static_cast<float>(m_config.width);
    viewport.maxY = static_cast<float>(m_config.height);
    viewport.minZ = 0.0f;
    viewport.maxZ = 1.0f;
    graphicsState.viewport.addViewport(viewport);

    // Scissor rect (full screen, no clipping)
    nvrhi::Rect scissor;
    scissor.minX = 0;
    scissor.minY = 0;
    scissor.maxX = m_config.width;
    scissor.maxY = m_config.height;
    graphicsState.viewport.addScissorRect(scissor);

    // Vertex buffer binding
    nvrhi::VertexBufferBinding vbBinding;
    vbBinding.buffer = m_vertexBuffer;
    vbBinding.slot = 0;
    vbBinding.offset = 0;
    graphicsState.vertexBuffers = { vbBinding };

    // Index buffer binding
    graphicsState.indexBuffer = { m_indexBuffer, nvrhi::Format::R16_UINT, 0 };

    // ═══════════════════════════════════════════════════════
    //  3. CREATE BINDING SET (FROM SHADER REFLECTION)
    // ═══════════════════════════════════════════════════════
    // Bind textures and samplers discovered via shader reflection

    std::vector<nvrhi::BindingSetItem> bindings;

    // Bind textures (use extracted font texture)
    for (const auto& tex : m_shaderReflection.inputTextures) {
        resources::FGResourceManager* resMgr = m_device->GetFGResourceManager();
        resources::TextureManager* texMgr = resMgr->GetTextureManager();
        nvrhi::ITexture* nvrhiTex = texMgr->GetNVRHITexture(m_fontTextureHandle);

        if (nvrhiTex) {
            bindings.push_back(nvrhi::BindingSetItem::Texture_SRV(tex.slot, nvrhiTex));
            Msg("    Bound font texture '%s' at slot t%u", tex.name.c_str(), tex.slot);
        } else {
            Msg("! [TextPass::RenderText] Font texture not available for slot t%u", tex.slot);
            Msg("! Handle: index=%u, gen=%u", m_fontTextureHandle.index, m_fontTextureHandle.generation);
        }
    }

    // Bind cached samplers (created during init, reused every frame)
    for (size_t i = 0; i < m_shaderReflection.samplers.size() && i < m_samplers.size(); i++) {
        const auto& samplerInfo = m_shaderReflection.samplers[i];
        bindings.push_back(nvrhi::BindingSetItem::Sampler(samplerInfo.slot, m_samplers[i]));
        Msg("    Bound cached sampler '%s' at slot s%u", samplerInfo.name.c_str(), samplerInfo.slot);
    }

    // Bind vertex shader constant buffers
    if (m_vsConstantBuffer) {
        for (const auto& cb : m_vsConstantBuffers.buffers) {
            bindings.push_back(nvrhi::BindingSetItem::ConstantBuffer(cb.slot, m_vsConstantBuffer));
            Msg("    Bound VS constant buffer '%s' at slot b%u (%u bytes)",
                cb.name.c_str(), cb.slot, cb.size);
        }
    }

    // Create binding set
    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = bindings;
    bindingSetDesc.trackLiveness = true;

    nvrhi::BindingSetHandle bindingSet = m_device->GetNVRHIDevice()->createBindingSet(
        bindingSetDesc, m_bindingLayout);

    if (bindingSet) {
        graphicsState.bindings = { bindingSet };
    } else {
        Msg("! [TextPass::RenderText] Failed to create binding set");
    }

    // ═══════════════════════════════════════════════════════
    //  2. ISSUE DRAW CALL
    // ═══════════════════════════════════════════════════════

    cmdList->setGraphicsState(graphicsState);

    // Draw indexed triangles (6 indices per quad, 2 triangles per quad)
    const u32 numQuads = static_cast<u32>(m_vertices.size() / 4);
    const u32 indexCount = numQuads * 6;

    nvrhi::DrawArguments drawArgs;
    drawArgs.instanceCount = 1;
    drawArgs.startIndexLocation = 0;
    drawArgs.startVertexLocation = 0;
    drawArgs.vertexCount = indexCount;

    cmdList->drawIndexed(drawArgs);

    Msg("  [TextPass::RenderText] Drew %u quads (%u indices)", numQuads, indexCount);
}

} // namespace xray::render::passes
