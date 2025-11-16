// xrRender/FrameGraphPasses/TextPass.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

// Forward declarations
class CGameFont;

namespace xray::render {
    class MaterialCache;
    namespace framegraph {
        class VolatileConstantBufferPool;
    }
}

namespace xray::render::resources {
    class FGResourceManager;
    struct TextureHandle;
}

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  TEXT PASS CONFIGURATION
// ══════════════════════════════════════════════════════════

struct TextPassConfig {
    u32 width = 0;   // Output RT width (Device.dwWidth)
    u32 height = 0;  // Output RT height (Device.dwHeight)
};

// ══════════════════════════════════════════════════════════
//  TEXT PASS (Render fonts/text via NVRHI)
// ══════════════════════════════════════════════════════════
// Renders all text/fonts on top of UI layer using pure NVRHI
// This is STEP 2 of the 4-step UI rendering pipeline:
//   1. UIPass:           Render UI sprites/widgets to rt_UIMain
//   2. TextPass:         Render text/fonts on top (THIS PASS)
//   3. UIDistortPass:    Render distortion mask to rt_UIDistort
//   4. UICompositePass:  Composite all layers to final output
//
// This pass:
// - Replaces legacy dxFontRender (which used RCache + raw D3D11)
// - Uses RenderContext for proper NVRHI state management
// - Loads font textures via FGResourceManager
// - Renders text as textured quads using NVRHI pipeline state
//
// NEVER uses RCache or raw D3D11 calls!

class TextPass : public framegraph::IPass {
public:
    TextPass(const TextPassConfig& config = TextPassConfig());
    ~TextPass() override;

    // IPass interface
    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;  // Text rendering is a custom phase
    }

    // Set output render targets (called by FrameGraphRenderer)
    // NOTE: TextPass renders ON TOP of UIPass output (same RT)
    void SetOutputs(framegraph::VirtualResourceHandle uiMain, framegraph::VirtualResourceHandle depth);

    // Text rendering statistics
    struct TextStats {
        u32 numStrings = 0;
        u32 numCharacters = 0;
        float cpuTimeMs = 0.0f;
    };

    const TextStats& GetTextStats() const { return m_textStats; }

private:
    // ═══════════════════════════════════════════════════════
    //  HELPER FUNCTIONS
    // ═══════════════════════════════════════════════════════

    // Collect all text from active CGameFont instances
    void CollectTextGeometry();

    // Build vertex/index buffers for text quads
    void BuildTextBuffers(nvrhi::ICommandList* cmdList);

    // Render text using RenderContext
    void RenderText(ng::RenderContext& ctx, nvrhi::ITexture* outputTexture, nvrhi::ITexture* depthTexture);

    // ═══════════════════════════════════════════════════════
    //  MEMBER VARIABLES
    // ═══════════════════════════════════════════════════════

    
    TextPassConfig m_config;
    TextStats m_textStats;

    // Output render targets (same as UIPass - we composite on top)
    framegraph::VirtualResourceHandle m_outputRT;      // rt_UIMain
    framegraph::VirtualResourceHandle m_depthStencil;  // rt_Depth

    // NVRHI resources for text rendering
    nvrhi::BufferHandle m_vertexBuffer;   // Dynamic vertex buffer for text quads
    nvrhi::BufferHandle m_indexBuffer;    // Static index buffer for quads

    // Material cache system (handles PSO/binding creation automatically)
    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_vcbPool;
    xr_unique_ptr<MaterialCache> m_materialCache;
    nvrhi::BufferHandle m_constantBuffer;  // Constant buffer for screen_res

    bool m_initialized{false};         // Buffers/vertex layout initialized

    // Vertex format for text rendering
    // Matches shader input: v_TL { float4 P : POSITION; float2 Tex0 : TEXCOORD0; float4 Color : COLOR; }
    struct TextVertex {
        float x, y, z, w;    // Position (w=1.0 for screen-space quads)
        u32 color;           // RGBA color (packed BGRA for shader)
        float u, v;          // Texture coordinates
    };

    // Per-font batch (groups geometry by font for MaterialCache)
    struct FontBatch {
        CGameFont* font;              // Font this batch belongs to
        xr_vector<TextVertex> vertices;
        xr_vector<u16> indices;
        u32 numStrings;              // Number of strings in this batch
    };

    // Collected text geometry (built each frame, grouped by font)
    xr_vector<FontBatch> m_fontBatches;
};

} // namespace xray::render::passes
