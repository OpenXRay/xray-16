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
    TextPass(ng::RenderDevice* device, const TextPassConfig& config = TextPassConfig());
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

    // Extract shader information from CGameFont and initialize pipeline (lazy init)
    bool InitializeFromFont(CGameFont* font, nvrhi::ICommandList* cmdList);

    // Build vertex/index buffers for text quads
    void BuildTextBuffers(nvrhi::ICommandList* cmdList);

    // Render text using NVRHI
    void RenderText(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer);

    // ═══════════════════════════════════════════════════════
    //  MEMBER VARIABLES
    // ═══════════════════════════════════════════════════════

    ng::RenderDevice* m_device;
    TextPassConfig m_config;
    TextStats m_textStats;

    // Output render targets (same as UIPass - we composite on top)
    framegraph::VirtualResourceHandle m_outputRT;      // rt_UIMain
    framegraph::VirtualResourceHandle m_depthStencil;  // rt_Depth

    // NVRHI resources for text rendering
    nvrhi::BufferHandle m_vertexBuffer;   // Dynamic vertex buffer for text quads
    nvrhi::BufferHandle m_indexBuffer;    // Static index buffer for quads
    nvrhi::ShaderHandle m_vertexShader;   // Text vertex shader
    nvrhi::ShaderHandle m_pixelShader;    // Text pixel shader
    nvrhi::BindingLayoutHandle m_bindingLayout;  // Layout for texture + constants
    nvrhi::GraphicsPipelineHandle m_pipeline;     // Cached pipeline state

    bool m_initialized{false};         // Buffers/vertex layout initialized
    bool m_pipelineReady{false};       // Shaders/pipeline initialized (lazy)

    // Font texture (loaded via FGResourceManager)
    ng::TextureHandle m_fontTextureHandle;

    // Shader reflection results (discovered textures/samplers/CBs)
    framegraph::ShaderRTBindings m_shaderReflection;
    framegraph::ShaderConstantBuffers m_vsConstantBuffers;  // Vertex shader CBs

    // Cached samplers (created once, reused every frame)
    xr_vector<nvrhi::SamplerHandle> m_samplers;

    // Constant buffer for vertex shader (if needed)
    nvrhi::BufferHandle m_vsConstantBuffer;

    // Default fallback texture (1x1 white) for when font textures aren't loaded
    nvrhi::TextureHandle m_defaultTexture;

    // Vertex format for text rendering
    // Matches shader input: v_TL { float4 P : POSITION; float2 Tex0 : TEXCOORD0; float4 Color : COLOR; }
    struct TextVertex {
        float x, y, z, w;    // Position (w=1.0 for screen-space quads)
        u32 color;           // RGBA color (packed BGRA for shader)
        float u, v;          // Texture coordinates
    };

    // Collected text geometry (built each frame)
    xr_vector<TextVertex> m_vertices;
    xr_vector<u16> m_indices;
};

} // namespace xray::render::passes
