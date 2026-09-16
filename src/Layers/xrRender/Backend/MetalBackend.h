#pragma once

#include "xrEngine/IRenderBackend.h"

struct SDL_Window;

class MetalBackend : public IRenderBackend {
public:
    MetalBackend();
    ~MetalBackend() override;

    MetalBackend(const MetalBackend&) = delete;
    MetalBackend& operator=(const MetalBackend&) = delete;

    bool Initialize(SDL_Window* window, u32 width, u32 height, bool enableValidation = false);
    void Shutdown() override;

    API GetAPI() const override { return API::Metal; }
    pcstr GetAPIName() const override { return "Metal"; }
    bool IsInitialized() const override;
    void WaitForIdle() override;
    DeviceState GetDeviceState() const override;

    nvrhi::IDevice* GetDevice() const override;
    nvrhi::ICommandList* GetCommandList() const override;
    nvrhi::ICommandList* CreateCommandList() override;
    void ExecuteCommandList(nvrhi::ICommandList* commandList) override;
    void ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) override;
    void UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) override;

    nvrhi::ITexture* GetBackBuffer() override;
    u32 GetCurrentBackBufferIndex() const override;
    u32 GetBackBufferCount() const override { return 3; }
    std::pair<u32, u32> GetBackBufferSize() const override;
    void Present(bool vsync) override;
    void ResizeSwapChain(u32 width, u32 height) override;
    void BeginFrame() override;
    void EndFrame() override;
    bool IsInFrame() const override;

    const Capabilities& GetCapabilities() const override;
    Capabilities& GetMutableCapabilities() override;
    void UpdateCapabilities() override;

    u32 RegisterBindlessTexture(nvrhi::ITexture* texture) override;
    void UnregisterBindlessTexture(u32 index) override;
    nvrhi::IBindingLayout* GetBindlessLayout() const override;
    nvrhi::IDescriptorTable* GetBindlessDescriptorTable() const override;

    void BeginDebugEvent(pcstr name) override;
    void EndDebugEvent() override;
    void SetMarker(pcstr name) override;

private:
    struct Impl;
    Impl* m_impl;
};

IRenderBackend* CreateMetalBackend(SDL_Window* window, u32 width, u32 height, bool enableValidation);
