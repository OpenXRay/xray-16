#pragma once

#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

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

    void BuildTextBuffers(nvrhi::ICommandList* cmdList);

    void RenderText(ng::RenderContext& ctx, nvrhi::ITexture* outputTexture, nvrhi::ITexture* depthTexture);

    ng::RenderDevice* m_device;
    TextPassConfig m_config;
    TextStats m_textStats;

    framegraph::VirtualResourceHandle m_outputRT;
    framegraph::VirtualResourceHandle m_depthStencil;

    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_indexBuffer;

    xr_unique_ptr<framegraph::VolatileConstantBufferPool> m_vcbPool;
    xr_unique_ptr<MaterialCache> m_materialCache;
    nvrhi::BufferHandle m_constantBuffer;

    bool m_initialized{false};

    struct TextVertex {
        float x, y, z, w;
        u32 color;
        float u, v;
    };

    struct FontBatch {
        CGameFont* font;
        xr_vector<TextVertex> vertices;
        xr_vector<u16> indices;
        u32 numStrings;
    };

    xr_vector<FontBatch> m_fontBatches;
};

}
