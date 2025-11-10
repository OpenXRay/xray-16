#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

class CGameFont;

namespace xray::render::resources {
    class FGResourceManager;
    struct TextureHandle;
}

namespace xray::render::passes {

struct TextPassConfig {
    u32 width = 0;
    u32 height = 0;
};

class TextPass : public framegraph::IPass {
public:
    TextPass(ng::RenderDevice* device, const TextPassConfig& config = TextPassConfig());
    ~TextPass() override;

    void Setup(framegraph::FrameGraph& fg) override;
    void Execute(ng::RenderContext& ctx, const framegraph::FrameGraph& fg) override;

    framegraph::RenderPhase GetPhase() const override {
        return framegraph::RenderPhase::Custom;
    }

    void SetOutputs(framegraph::VirtualResourceHandle uiMain, framegraph::VirtualResourceHandle depth);

    struct TextStats {
        u32 numStrings = 0;
        u32 numCharacters = 0;
        float cpuTimeMs = 0.0f;
    };

    const TextStats& GetTextStats() const { return m_textStats; }

private:

    void CollectTextGeometry();

    void BuildTextBuffers();

    void RenderText(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer);

    ng::RenderDevice* m_device;
    TextPassConfig m_config;
    TextStats m_textStats;

    framegraph::VirtualResourceHandle m_outputRT;
    framegraph::VirtualResourceHandle m_depthStencil;

    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_indexBuffer;
    nvrhi::ShaderHandle m_vertexShader;
    nvrhi::ShaderHandle m_pixelShader;
    nvrhi::BindingLayoutHandle m_bindingLayout;
    nvrhi::GraphicsPipelineHandle m_pipeline;

    bool m_initialized{false};

    xr_map<shared_str, resources::TextureHandle> m_fontTextures;

    struct TextVertex {
        float x, y, z;
        u32 color;
        float u, v;
    };

    xr_vector<TextVertex> m_vertices;
    xr_vector<u16> m_indices;
};

}
