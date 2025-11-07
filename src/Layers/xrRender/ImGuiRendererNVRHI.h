#pragma once

#include "Include/xrRender/ImGuiRender.h"
#include "RenderContext/RenderDevice.h"
#include <nvrhi/nvrhi.h>
#include <imgui.h>

namespace xray::render::ng {

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
    bool CreateFontsTexture();
    bool CreateShaders();
    bool CreatePipelineState();
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

} // namespace xray::render::ng