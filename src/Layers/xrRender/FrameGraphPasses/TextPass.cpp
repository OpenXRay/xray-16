// xrRender/FrameGraphPasses/TextPass.cpp
#include "stdafx.h"
#include "TextPass.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrUICore/ui_base.h"
#include "xrUICore/FontManager/FontManager.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "xrCore/Text/StringConversion.hpp"

namespace xray::render::passes {

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

        // TODO: Create shaders, buffers, pipeline state
        // - Load vertex/pixel shaders for text rendering
        // - Create dynamic vertex buffer
        // - Create index buffer for quads
        // - Create binding layout for textures + constants
        // - Create graphics pipeline state

        m_initialized = true;
        Msg("  [TextPass] NVRHI initialization complete");
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
    //  BUILD BUFFERS
    // ═══════════════════════════════════════════════════════

    BuildTextBuffers();

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

                    // Create 4 vertices for quad
                    TextVertex v0 = { X,       Y2, 0.0f, clr2, tu,             tv + font->fTCHeight };
                    TextVertex v1 = { X,       Y,  0.0f, clr,  tu,             tv };
                    TextVertex v2 = { X + scw, Y2, 0.0f, clr2, tu + fTCWidth,  tv + font->fTCHeight };
                    TextVertex v3 = { X + scw, Y,  0.0f, clr,  tu + fTCWidth,  tv };

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

void TextPass::BuildTextBuffers() {
    // TODO: Upload vertex data to GPU
    // - Update dynamic vertex buffer with m_vertices
    // - Upload indices if needed (can be static)

    Msg("  [TextPass::BuildTextBuffers] TODO: Implement buffer upload");
}

void TextPass::RenderText(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer) {
    // TODO: Render text using NVRHI
    // 1. Set graphics state
    // 2. Set pipeline
    // 3. Bind vertex/index buffers
    // 4. Bind font texture
    // 5. Draw indexed

    Msg("  [TextPass::RenderText] TODO: Implement NVRHI text rendering");
}

} // namespace xray::render::passes
