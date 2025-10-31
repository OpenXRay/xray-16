#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraphPasses/GBufferPass.h"

namespace xray::render::debug {

enum class GBufferVisMode : u32 {
    Off = 0,
    Albedo,
    Normal,
    Depth,
    Metallic,
    Roughness,
    MaterialID,
};

class GBufferVisualizer {
public:
    GBufferVisualizer();
    ~GBufferVisualizer();

    void Setup(
        framegraph::FrameGraph& fg,
        const passes::GBufferOutputs& gbuffer,
        framegraph::VirtualResourceHandle backbuffer
    );

    void SetMode(GBufferVisMode mode) { m_mode = mode; }
    GBufferVisMode GetMode() const { return m_mode; }

    void CycleMode();

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

}
