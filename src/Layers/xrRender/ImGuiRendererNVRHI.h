#pragma once

#include "Include/xrRender/ImGuiRender.h"
#include "RenderContext/RenderDevice.h"
#include <nvrhi/nvrhi.h>
#include <imgui.h>
#include <map>

namespace xray::render::fg {

class ImGuiRendererNVRHI : public IImGuiRender {
public:
    ImGuiRendererNVRHI(RenderDevice* renderDevice);
    ~ImGuiRendererNVRHI() override;

    // IImGuiRender interface
    void Copy(IImGuiRender& _in) override;
    void Frame() override;
    void Render(ImDrawData* data) override;
    void OnDeviceCreate(ImGuiContext* context) override;
    void OnDeviceDestroy() override;
    void OnDeviceResetBegin() override;
    void OnDeviceResetEnd() override;

    void InvalidateShadersAndPipeline();

    // Render with explicit framebuffer and command list (called inline after FrameGraph)
    void Render(ImDrawData* data, nvrhi::IFramebuffer* framebuffer, nvrhi::ICommandList* cmdList);

private:
    // Core device handles
    RenderDevice* m_renderDevice = nullptr;
    nvrhi::DeviceHandle m_device;

    // Resources
    nvrhi::TextureHandle m_fontTexture;
    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_indexBuffer;
    nvrhi::BufferHandle m_constantBuffer;

    // Pipeline state objects
    nvrhi::GraphicsPipelineHandle m_pipeline;
    nvrhi::BindingLayoutHandle m_bindingLayout;
    nvrhi::BindingSetHandle m_resourceBindings;
    nvrhi::InputLayoutHandle m_inputLayout;
    nvrhi::ShaderHandle m_vertexShader;
    nvrhi::ShaderHandle m_pixelShader;
    nvrhi::SamplerHandle m_fontSampler;

    // Render state (contains blend, raster, depth-stencil)
    nvrhi::RenderState m_renderState;

    // Buffer management
    size_t m_vertexBufferSize = 0;
    size_t m_indexBufferSize = 0;
    u32 m_frameIndex = 0;

    // ImGui context
    ImGuiContext* m_imguiContext = nullptr;

    // Texture management (for modern ImGui 1.92+ API)
    std::map<ImTextureID, nvrhi::TextureHandle> m_textures;

    // Current framebuffer (set during Render() call)
    nvrhi::IFramebuffer* m_currentFramebuffer = nullptr;

    // Viewport (set during SetupRenderState)
    nvrhi::Viewport m_viewport;

    // Constants structure for projection matrix
    struct ImGuiConstants {
        float mvpMatrix[4][4];
    };

protected:
    // Rendering helpers (protected so derived classes can use them)
    void SetupRenderState(ImDrawData* drawData, nvrhi::ICommandList* cmdList);
    void UploadDrawData(ImDrawData* drawData, nvrhi::ICommandList* cmdList);
    void RenderDrawData(ImDrawData* drawData, nvrhi::ICommandList* cmdList);

private:
    // Internal helper methods
    bool CreateDeviceObjects();
    void DestroyDeviceObjects();
    void ProcessTextureRequests(ImDrawData* drawData);  // Modern ImGui 1.92+ texture handling
    bool CreateShaders();
    bool CreatePipelineState();
    bool CreateResourceBindings(); // Create bindings after font texture is available
    bool CreateBuffers(size_t vtxSize, size_t idxSize);
    bool ResizeBuffers(size_t vtxSize, size_t idxSize);

    // Utility
    nvrhi::ITexture* GetTextureFromImTextureID(ImTextureID id);
    void UpdateTextureBinding(ImTextureID textureId);
};

// Factory for creating the appropriate ImGui renderer
class ImGuiRendererFactory {
public:
    static xr_unique_ptr<IImGuiRender> Create(RenderDevice* device);
};

} // namespace xray::render::fg