// xrRender/Debug/GBufferVisualization.h
#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraphPasses/GBufferPass.h"

namespace xray::render::debug {

// ══════════════════════════════════════════════════════════
//  G-BUFFER VISUALIZATION MODE
// ══════════════════════════════════════════════════════════

enum class GBufferVisMode : u32 {
    Off = 0,
    Albedo,
    Normal,
    Depth,
    Metallic,
    Roughness,
    MaterialID,
};

// ══════════════════════════════════════════════════════════
//  G-BUFFER VISUALIZER
// ══════════════════════════════════════════════════════════

class GBufferVisualizer {
public:
    GBufferVisualizer();
    ~GBufferVisualizer();

    // Setup visualization pass
    void Setup(
        framegraph::FrameGraph& fg,
        const passes::GBufferOutputs& gbuffer,
        framegraph::VirtualResourceHandle backbuffer
    );

    // Set visualization mode
    void SetMode(GBufferVisMode mode) { m_mode = mode; }
    GBufferVisMode GetMode() const { return m_mode; }

    // Cycle through modes (for debugging)
    void CycleMode();

    // Get mode name
    static const char* GetModeName(GBufferVisMode mode);

private:
    GBufferVisMode m_mode = GBufferVisMode::Off;

    nvrhi::GraphicsPipelineHandle m_pipeline;
    nvrhi::IShader* m_vertexShader = nullptr;
    nvrhi::IShader* m_pixelShader = nullptr;

    void Execute(
        ng::RenderContext& ctx,
        const framegraph::FrameGraph& fg,
        const passes::GBufferOutputs& gbuffer,
        framegraph::VirtualResourceHandle backbuffer
    );
};

} // namespace xray::render::debug
