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

    void Copy(IImGuiRender& _in) override;
    void Frame() override;
    void Render(ImDrawData* data) override;
    void OnDeviceCreate(ImGuiContext* context) override;
    void OnDeviceDestroy() override;
    void OnDeviceResetBegin() override;
    void OnDeviceResetEnd() override;

    void InvalidateShadersAndPipeline();

    void Render(ImDrawData* data, nvrhi::IFramebuffer* framebuffer, nvrhi::ICommandList* cmdList);

private:
    RenderDevice* m_renderDevice = nullptr;
    nvrhi::DeviceHandle m_device;

    nvrhi::TextureHandle m_fontTexture;
    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_indexBuffer;
    nvrhi::BufferHandle m_constantBuffer;

    nvrhi::GraphicsPipelineHandle m_pipeline;
    nvrhi::GraphicsPipelineDesc m_pipelineDesc;
    nvrhi::BindingLayoutHandle m_bindingLayout;
    nvrhi::BindingSetHandle m_resourceBindings;
    nvrhi::InputLayoutHandle m_inputLayout;
    nvrhi::ShaderHandle m_vertexShader;
    nvrhi::ShaderHandle m_pixelShader;
    nvrhi::SamplerHandle m_fontSampler;

    nvrhi::RenderState m_renderState;

    size_t m_vertexBufferSize = 0;
    size_t m_indexBufferSize = 0;
    u32 m_frameIndex = 0;

    ImGuiContext* m_imguiContext = nullptr;

    std::map<ImTextureID, nvrhi::TextureHandle> m_textures;

    nvrhi::IFramebuffer* m_currentFramebuffer = nullptr;

    nvrhi::Viewport m_viewport;

    struct ImGuiConstants {
        float mvpMatrix[4][4];
    };

protected:
    void SetupRenderState(ImDrawData* drawData, nvrhi::ICommandList* cmdList);
    void UploadDrawData(ImDrawData* drawData, nvrhi::ICommandList* cmdList);
    void RenderDrawData(ImDrawData* drawData, nvrhi::ICommandList* cmdList);

private:
    bool CreateDeviceObjects();
    void DestroyDeviceObjects();
    void ProcessTextureRequests(ImDrawData* drawData);
    bool CreateShaders();
    bool CreatePipelineState();
    bool CreateResourceBindings();
    bool CreateBuffers(size_t vtxSize, size_t idxSize);

    nvrhi::ITexture* GetTextureFromImTextureID(ImTextureID id);
    void UpdateTextureBinding(ImTextureID textureId);
    nvrhi::IGraphicsPipeline* GetOrCreatePipelineForFramebuffer(nvrhi::IFramebuffer* fb);
};

class ImGuiRendererFactory {
public:
    static xr_unique_ptr<IImGuiRender> Create(RenderDevice* device);
};

}
