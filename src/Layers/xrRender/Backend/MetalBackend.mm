#define BOOL XRayWindowsBOOL
#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "MetalBackend.h"
#undef BOOL

#include <nvrhi/metal3.h>
#include <nvrhi/validation.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>
#include <atomic>
#include <mutex>

extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void* pool);

struct MetalBackend::Impl final : nvrhi::IMessageCallback {
    struct Frame {
        nvrhi::CommandListHandle commands;
        nvrhi::TextureHandle texture;
        id<CAMetalDrawable> drawable = nil;
        id<MTLCommandBuffer> completion = nil;
    };

    std::atomic<DeviceState> state{DeviceState::Lost};
    SDL_Window* window = nullptr;
    SDL_MetalView view = nullptr;
    CAMetalLayer* layer = nil;
    id<MTLDevice> nativeDevice = nil;
    id<MTLCommandQueue> queue = nil;
    nvrhi::DeviceHandle nativeNvrhiDevice;
    nvrhi::DeviceHandle device;
    nvrhi::CommandListHandle uploads;
    id<MTLCommandBuffer> uploadCompletion = nil;
    Frame frames[3];
    static constexpr u32 MaxBindlessTextures = 65536;
    nvrhi::BindingLayoutHandle bindlessLayout;
    nvrhi::DescriptorTableHandle bindlessTable;
    xr_vector<u32> freeBindlessIndices;
    xr_map<nvrhi::ITexture*, u32> bindlessTextures;
    u32 nextBindlessIndex = 0;
    Capabilities capabilities;
    std::mutex queueMutex;
    u32 currentFrame = 0;
    u32 width = 0;
    u32 height = 0;
    u32 debugDepth = 0;
    void* framePool = nullptr;
    bool initialized = false;
    bool inFrame = false;
    bool readyToPresent = false;

    void message(nvrhi::MessageSeverity severity, const char* text) override {
        if (severity == nvrhi::MessageSeverity::Fatal)
            state.store(DeviceState::Lost);
        if (severity >= nvrhi::MessageSeverity::Error) {
            Msg("! [MetalBackend] ERROR: %s", text);
        } else if (severity == nvrhi::MessageSeverity::Warning) {
            Msg("! [MetalBackend] WARNING: %s", text);
        } else {
            Msg("* [MetalBackend] %s", text);
        }
    }

    void fail(const char* text) {
        state.store(DeviceState::Lost);
        message(nvrhi::MessageSeverity::Error, text);
    }

    bool checkCompletion(id<MTLCommandBuffer> commands, bool wait) {
        if (!commands)
            return true;
        if (wait)
            [commands waitUntilCompleted];
        if (commands.status == MTLCommandBufferStatusError) {
            const char* error = commands.error.localizedDescription.UTF8String;
            fail(error ? error : "Native Metal command buffer failed without an error description.");
            return false;
        }
        if (wait && commands.status != MTLCommandBufferStatusCompleted) {
            fail("Native Metal completion wait returned before command buffer completion.");
            return false;
        }
        return true;
    }

    void collect() {
        for (const Frame& frame : frames)
            checkCompletion(frame.completion, false);
        checkCompletion(uploadCompletion, false);
        if (device)
            device->runGarbageCollection();
    }

    bool updateDrawableSize() {
        int pixelWidth = 0;
        int pixelHeight = 0;
        if (!SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight)) {
            Msg("! [MetalBackend] SDL_GetWindowSizeInPixels: %s", SDL_GetError());
            fail("Unable to query the Metal window pixel dimensions.");
            return false;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
            return false;
        width = pixelWidth > 0 ? static_cast<u32>(pixelWidth) : 0;
        height = pixelHeight > 0 ? static_cast<u32>(pixelHeight) : 0;
        if (!width || !height) {
            DeviceState expected = DeviceState::Normal;
            state.compare_exchange_strong(expected, DeviceState::NeedReset);
            return false;
        }
        const CGSize size = CGSizeMake(width, height);
        if (!CGSizeEqualToSize(layer.drawableSize, size))
            layer.drawableSize = size;
        const float scale = SDL_GetWindowPixelDensity(window);
        if (scale > 0.f)
            layer.contentsScale = scale;
        DeviceState expected = DeviceState::NeedReset;
        state.compare_exchange_strong(expected, DeviceState::Normal);
        return true;
    }

    id<MTLCommandBuffer> nativeCommands(nvrhi::ICommandList* commands) {
        auto* native = static_cast<nvrhi::metal3::ICommandList*>(
            commands->getNativeObject(nvrhi::ObjectTypes::Nvrhi_Metal3_CommandList).pointer);
        if (!native) {
            fail("Unable to access the native Metal command list.");
            return nil;
        }
        return native->getNativeCommandBuffer();
    }

    void drainFramePool() {
        if (framePool) {
            void* pool = framePool;
            framePool = nullptr;
            objc_autoreleasePoolPop(pool);
        }
    }
};

MetalBackend::MetalBackend() : m_impl(xr_new<Impl>()) {}

MetalBackend::~MetalBackend() {
    Shutdown();
    xr_delete(m_impl);
}

bool MetalBackend::Initialize(SDL_Window* window, u32 width, u32 height, bool enableValidation) {
    @autoreleasepool {
        Impl& impl = *m_impl;
        if (impl.initialized || impl.view || impl.device) {
            Msg("! [MetalBackend] Already initialized");
            return false;
        }
        if (!window || !SDL_IsMainThread()) {
            impl.fail("Initialization requires an SDL window on the main thread.");
            return false;
        }
        impl.state.store(DeviceState::Normal);
        impl.window = window;
        impl.nativeDevice = MTLCreateSystemDefaultDevice();
        if (!impl.nativeDevice || ![impl.nativeDevice supportsFamily:MTLGPUFamilyMetal3] ||
            impl.nativeDevice.argumentBuffersSupport < MTLArgumentBuffersTier2) {
            impl.fail("Native Metal requires a Metal 3 GPU with argument buffers tier 2.");
            Shutdown();
            return false;
        }
        impl.queue = [impl.nativeDevice newCommandQueue];
        if (!impl.queue || impl.queue.device != impl.nativeDevice) {
            impl.fail("Unable to create a matching native Metal command queue.");
            Shutdown();
            return false;
        }
        impl.queue.label = @"OpenXRay Metal graphics and presentation";
        impl.view = SDL_Metal_CreateView(window);
        if (!impl.view) {
            Msg("! [MetalBackend] SDL_Metal_CreateView: %s", SDL_GetError());
            impl.fail("Unable to create the SDL Metal view.");
            Shutdown();
            return false;
        }
        impl.layer = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(impl.view);
        if (!impl.layer) {
            impl.fail("SDL Metal view has no CAMetalLayer.");
            Shutdown();
            return false;
        }
        impl.layer.device = impl.nativeDevice;
        impl.layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        impl.layer.framebufferOnly = YES;
        impl.layer.opaque = YES;
        impl.layer.maximumDrawableCount = GetBackBufferCount();
        impl.layer.allowsNextDrawableTimeout = YES;
        impl.layer.displaySyncEnabled = YES;

        nvrhi::metal3::DeviceDesc desc;
        desc.pDevice = impl.nativeDevice;
        desc.commonQueue = impl.queue;
        desc.errorCB = &impl;
        impl.nativeNvrhiDevice = nvrhi::metal3::createDevice(desc);
        if (!impl.nativeNvrhiDevice) {
            impl.fail("Unable to create the NVRHI Metal device.");
            Shutdown();
            return false;
        }
        impl.device = enableValidation ? nvrhi::validation::createValidationLayer(impl.nativeNvrhiDevice) : impl.nativeNvrhiDevice;
        if (!impl.device || !impl.device->queryFeatureSupport(nvrhi::Feature::DeferredCommandLists)) {
            impl.fail("NVRHI Metal device requires deferred command list support.");
            Shutdown();
            return false;
        }
        nvrhi::CommandListParameters params;
        params.enableImmediateExecution = false;
        for (Impl::Frame& frame : impl.frames) {
            frame.commands = impl.device->createCommandList(params);
            if (!frame.commands) {
                impl.fail("Unable to create a Metal frame command list.");
                Shutdown();
                return false;
            }
        }
        impl.uploads = impl.device->createCommandList(params);
        if (!impl.uploads) {
            impl.fail("Unable to create the Metal upload command list.");
            Shutdown();
            return false;
        }
        nvrhi::BindlessLayoutDesc bindlessDesc;
        bindlessDesc.visibility = nvrhi::ShaderType::All;
        bindlessDesc.firstSlot = 0;
        bindlessDesc.maxCapacity = Impl::MaxBindlessTextures;
        bindlessDesc.registerSpaces = {nvrhi::BindingLayoutItem::Texture_SRV(1)};
        impl.bindlessLayout = impl.device->createBindlessLayout(bindlessDesc);
        if (impl.bindlessLayout)
            impl.bindlessTable = impl.device->createDescriptorTable(impl.bindlessLayout);
        if (!impl.bindlessTable) {
            impl.fail("Unable to create the native bindless texture table.");
            Shutdown();
            return false;
        }
        impl.device->resizeDescriptorTable(impl.bindlessTable, Impl::MaxBindlessTextures, false);
        impl.initialized = true;
        UpdateCapabilities();
        ResizeSwapChain(width, height);
        if (impl.state.load() == DeviceState::Lost) {
            Shutdown();
            return false;
        }
        Msg("* [MetalBackend] Native Metal initialized on %s (%ux%u pixels, validation %s)",
            impl.nativeDevice.name.UTF8String, impl.width, impl.height, enableValidation ? "enabled" : "disabled");
        return true;
    }
}

void MetalBackend::Shutdown() {
    @autoreleasepool {
        Impl& impl = *m_impl;
        if (impl.inFrame) {
            while (impl.debugDepth)
                EndDebugEvent();
            impl.frames[impl.currentFrame].commands->close();
            impl.inFrame = false;
        }
        WaitForIdle();
        for (Impl::Frame& frame : impl.frames) {
            frame.commands = nullptr;
            frame.texture = nullptr;
            frame.drawable = nil;
            frame.completion = nil;
        }
        impl.uploads = nullptr;
        impl.uploadCompletion = nil;
        impl.bindlessTable = nullptr;
        impl.bindlessLayout = nullptr;
        impl.bindlessTextures.clear();
        impl.freeBindlessIndices.clear();
        impl.nextBindlessIndex = 0;
        impl.device = nullptr;
        impl.nativeNvrhiDevice = nullptr;
        impl.layer.device = nil;
        impl.layer = nil;
        if (impl.view) {
            SDL_Metal_DestroyView(impl.view);
            impl.view = nullptr;
        }
        impl.queue = nil;
        impl.nativeDevice = nil;
        impl.window = nullptr;
        impl.currentFrame = 0;
        impl.width = 0;
        impl.height = 0;
        impl.debugDepth = 0;
        impl.readyToPresent = false;
        impl.initialized = false;
        impl.state.store(DeviceState::Lost);
    }
    m_impl->drainFramePool();
}

bool MetalBackend::IsInitialized() const { return m_impl->initialized; }
bool MetalBackend::IsInFrame() const { return m_impl->inFrame; }
nvrhi::IDevice* MetalBackend::GetDevice() const { return m_impl->device.Get(); }
nvrhi::ICommandList* MetalBackend::GetCommandList() const {
    return m_impl->initialized ? m_impl->frames[m_impl->currentFrame].commands.Get() : nullptr;
}

DeviceState MetalBackend::GetDeviceState() const {
    @autoreleasepool {
        std::lock_guard<std::mutex> lock(m_impl->queueMutex);
        m_impl->collect();
        return m_impl->initialized ? m_impl->state.load() : DeviceState::Lost;
    }
}

void MetalBackend::WaitForIdle() {
    @autoreleasepool {
        Impl& impl = *m_impl;
        std::lock_guard<std::mutex> lock(impl.queueMutex);
        if (impl.device && !impl.device->waitForIdle())
            impl.fail("Waiting for the Metal device detected failed GPU work.");
        for (Impl::Frame& frame : impl.frames)
            impl.checkCompletion(frame.completion, true);
        impl.checkCompletion(impl.uploadCompletion, true);
        impl.collect();
    }
}

void MetalBackend::BeginFrame() {
    ZoneScopedN("Metal::BeginFrame");
    @autoreleasepool {
        Impl& impl = *m_impl;
        if (!impl.initialized || impl.state.load() == DeviceState::Lost)
            return;
        if (impl.inFrame || impl.readyToPresent) {
            impl.fail("BeginFrame requires the previous frame to be ended and presented.");
            return;
        }
        if (SDL_GetWindowFlags(impl.window) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN))
            return;
        std::unique_lock<std::mutex> lock(impl.queueMutex, std::defer_lock);
        {
            ZoneScopedN("Metal::QueueLock");
            lock.lock();
        }
        {
            ZoneScopedN("Metal::Collect");
            impl.collect();
        }
        if (impl.state.load() == DeviceState::Lost || !impl.updateDrawableSize())
            return;
        Impl::Frame& frame = impl.frames[impl.currentFrame];
        {
            ZoneScopedN("Metal::FrameSlotWait");
            if (!impl.checkCompletion(frame.completion, true))
                return;
        }
        {
            ZoneScopedN("Metal::GC");
            frame.completion = nil;
            frame.texture = nullptr;
            frame.drawable = nil;
            impl.device->runGarbageCollection();
        }
        if (impl.state.load() == DeviceState::Lost)
            return;
        {
            ZoneScopedN("Metal::NextDrawable");
            frame.drawable = [impl.layer nextDrawable];
        }
        if (!frame.drawable) {
            DeviceState expected = DeviceState::Normal;
            impl.state.compare_exchange_strong(expected, DeviceState::NeedReset);
            return;
        }
        id<MTLTexture> texture = frame.drawable.texture;
        if (!texture || texture.pixelFormat != MTLPixelFormatBGRA8Unorm || !texture.width || !texture.height) {
            impl.fail("The Metal drawable has no valid BGRA8 pixel texture.");
            frame.drawable = nil;
            return;
        }
        impl.width = static_cast<u32>(texture.width);
        impl.height = static_cast<u32>(texture.height);
        nvrhi::TextureDesc desc;
        desc.width = impl.width;
        desc.height = impl.height;
        desc.format = nvrhi::Format::BGRA8_UNORM;
        desc.isRenderTarget = true;
        desc.isShaderResource = false;
        desc.initialState = nvrhi::ResourceStates::Present;
        desc.keepInitialState = true;
        desc.debugName = "Metal presentation drawable";
        frame.texture = impl.device->createHandleForNativeTexture(nvrhi::ObjectTypes::MTL3_Texture,
            nvrhi::Object((__bridge void*)texture), desc);
        if (!frame.texture) {
            impl.fail("Unable to wrap the native Metal drawable texture.");
            frame.drawable = nil;
            return;
        }
        {
            ZoneScopedN("Metal::CommandListOpen");
            frame.commands->open();
        }
        if (impl.state.load() == DeviceState::Lost) {
            frame.texture = nullptr;
            frame.drawable = nil;
            return;
        }
        impl.inFrame = true;
    }
    m_impl->framePool = objc_autoreleasePoolPush();
}

void MetalBackend::EndFrame() {
    @autoreleasepool {
        Impl& impl = *m_impl;
        if (!impl.inFrame)
            return;
        while (impl.debugDepth)
            EndDebugEvent();
        Impl::Frame& frame = impl.frames[impl.currentFrame];
        frame.commands->close();
        impl.inFrame = false;
        if (impl.state.load() != DeviceState::Lost) {
            std::lock_guard<std::mutex> lock(impl.queueMutex);
            if (!impl.device->executeCommandList(frame.commands))
                impl.fail("NVRHI rejected the Metal frame submission.");
            else
                impl.readyToPresent = true;
        }
    }
    m_impl->drainFramePool();
}

void MetalBackend::Present(bool vsync) {
    @autoreleasepool {
        Impl& impl = *m_impl;
        if (!impl.initialized || !impl.readyToPresent || impl.state.load() == DeviceState::Lost)
            return;
        std::lock_guard<std::mutex> lock(impl.queueMutex);
        Impl::Frame& frame = impl.frames[impl.currentFrame];
        impl.layer.displaySyncEnabled = vsync;
        frame.completion = [impl.queue commandBuffer];
        if (!frame.completion) {
            impl.fail("Unable to allocate the native Metal presentation command buffer.");
            return;
        }
        frame.completion.label = @"OpenXRay Metal present";
        [frame.completion presentDrawable:frame.drawable];
        [frame.completion commit];
        impl.readyToPresent = false;
        impl.currentFrame = (impl.currentFrame + 1) % GetBackBufferCount();
        impl.collect();
    }
}

nvrhi::ITexture* MetalBackend::GetBackBuffer() {
    const Impl& impl = *m_impl;
    return (impl.inFrame || impl.readyToPresent) && impl.state.load() != DeviceState::Lost ?
        impl.frames[impl.currentFrame].texture.Get() : nullptr;
}

u32 MetalBackend::GetCurrentBackBufferIndex() const { return m_impl->currentFrame; }
std::pair<u32, u32> MetalBackend::GetBackBufferSize() const { return {m_impl->width, m_impl->height}; }

void MetalBackend::ResizeSwapChain(u32, u32) {
    @autoreleasepool {
        Impl& impl = *m_impl;
        if (!impl.initialized || impl.state.load() == DeviceState::Lost || impl.inFrame || impl.readyToPresent)
            return;
        impl.updateDrawableSize();
    }
}

nvrhi::ICommandList* MetalBackend::CreateCommandList() {
    @autoreleasepool {
        Impl& impl = *m_impl;
        if (!impl.initialized || impl.state.load() == DeviceState::Lost)
            return nullptr;
        nvrhi::CommandListParameters params;
        params.enableImmediateExecution = false;
        auto commands = impl.device->createCommandList(params);
        if (!commands)
            impl.fail("Unable to create an additional deferred Metal command list.");
        return commands.Detach();
    }
}

void MetalBackend::ExecuteCommandList(nvrhi::ICommandList* commandList) {
    ExecuteCommandLists(&commandList, 1);
}

void MetalBackend::ExecuteCommandLists(nvrhi::ICommandList* const* commandLists, u32 count) {
    @autoreleasepool {
        Impl& impl = *m_impl;
        if (!impl.initialized || impl.state.load() == DeviceState::Lost || !count)
            return;
        std::lock_guard<std::mutex> lock(impl.queueMutex);
        if (!impl.device->executeCommandLists(commandLists, count))
            impl.fail("NVRHI rejected the ordered Metal command list batch.");
    }
}

void MetalBackend::UploadBufferData(nvrhi::IBuffer* buffer, const void* data, size_t size) {
    @autoreleasepool {
        Impl& impl = *m_impl;
        if (!impl.initialized || impl.state.load() == DeviceState::Lost || !buffer || !data || !size)
            return;
        if (impl.inFrame) {
            impl.frames[impl.currentFrame].commands->writeBuffer(buffer, data, size);
            return;
        }
        std::lock_guard<std::mutex> lock(impl.queueMutex);
        if (!impl.checkCompletion(impl.uploadCompletion, true))
            return;
        impl.uploadCompletion = nil;
        impl.device->runGarbageCollection();
        if (impl.state.load() == DeviceState::Lost)
            return;
        impl.uploads->open();
        if (impl.state.load() == DeviceState::Lost)
            return;
        impl.uploads->writeBuffer(buffer, data, size);
        impl.uploads->close();
        if (impl.state.load() == DeviceState::Lost)
            return;
        id<MTLCommandBuffer> commands = impl.nativeCommands(impl.uploads);
        if (!commands) {
            impl.fail("The Metal upload command list has no native command buffer.");
            return;
        }
        if (!impl.device->executeCommandList(impl.uploads)) {
            impl.fail("NVRHI rejected the Metal buffer upload.");
            return;
        }
        impl.uploadCompletion = commands;
    }
}

const IRenderBackend::Capabilities& MetalBackend::GetCapabilities() const { return m_impl->capabilities; }
IRenderBackend::Capabilities& MetalBackend::GetMutableCapabilities() { return m_impl->capabilities; }

void MetalBackend::UpdateCapabilities() {
    Impl& impl = *m_impl;
    if (!impl.device)
        return;
    Capabilities& caps = impl.capabilities;
    caps = {};
    caps.meshShaders = impl.device->queryFeatureSupport(nvrhi::Feature::Meshlets);
    caps.rayTracing = impl.device->queryFeatureSupport(nvrhi::Feature::RayTracingPipeline);
    caps.variableRateShading = impl.device->queryFeatureSupport(nvrhi::Feature::VariableRateShading);
    caps.bindlessTextures = impl.bindlessTable != nullptr;
    caps.maxBindlessResources = caps.bindlessTextures ? Impl::MaxBindlessTextures : 0;
    caps.shaderModel = 66;
    caps.raster_major = 6;
    caps.raster_minor = 6;
    caps.raster_profile = "ps_6_6";
    caps.geometry_major = 6;
    caps.geometry_minor = 6;
    caps.geometry_profile = "vs_6_6";
    caps.useCombinedSamplers = false;
}

u32 MetalBackend::RegisterBindlessTexture(nvrhi::ITexture* texture) {
    @autoreleasepool {
        Impl& impl = *m_impl;
        std::lock_guard<std::mutex> lock(impl.queueMutex);
        if (!impl.bindlessTable || !texture)
            return UINT32_MAX;
        const auto found = impl.bindlessTextures.find(texture);
        if (found != impl.bindlessTextures.end())
            return found->second;
        u32 slot;
        if (!impl.freeBindlessIndices.empty()) {
            slot = impl.freeBindlessIndices.back();
            impl.freeBindlessIndices.pop_back();
        } else {
            if (impl.nextBindlessIndex == Impl::MaxBindlessTextures)
                return UINT32_MAX;
            slot = impl.nextBindlessIndex++;
        }
        if (!impl.device->writeDescriptorTable(impl.bindlessTable, nvrhi::BindingSetItem::Texture_SRV(slot, texture))) {
            impl.freeBindlessIndices.push_back(slot);
            return UINT32_MAX;
        }
        impl.bindlessTextures.emplace(texture, slot);
        return slot;
    }
}

void MetalBackend::UnregisterBindlessTexture(u32 index) {
    @autoreleasepool {
        Impl& impl = *m_impl;
        std::lock_guard<std::mutex> lock(impl.queueMutex);
        for (auto it = impl.bindlessTextures.begin(); it != impl.bindlessTextures.end(); ++it) {
            if (it->second != index)
                continue;
            if (!impl.device->writeDescriptorTable(impl.bindlessTable, nvrhi::BindingSetItem::Texture_SRV(index, nullptr)))
                return;
            impl.bindlessTextures.erase(it);
            impl.freeBindlessIndices.push_back(index);
            return;
        }
    }
}

nvrhi::IBindingLayout* MetalBackend::GetBindlessLayout() const { return m_impl->bindlessLayout.Get(); }
nvrhi::IDescriptorTable* MetalBackend::GetBindlessDescriptorTable() const { return m_impl->bindlessTable.Get(); }

void MetalBackend::BeginDebugEvent(pcstr name) {
    if (m_impl->inFrame && name) {
        GetCommandList()->beginMarker(name);
        ++m_impl->debugDepth;
    }
}

void MetalBackend::EndDebugEvent() {
    if (m_impl->inFrame && m_impl->debugDepth) {
        GetCommandList()->endMarker();
        --m_impl->debugDepth;
    }
}

void MetalBackend::SetMarker(pcstr name) {
    if (m_impl->inFrame && name) {
        GetCommandList()->beginMarker(name);
        GetCommandList()->endMarker();
    }
}

IRenderBackend* CreateMetalBackend(SDL_Window* window, u32 width, u32 height, bool enableValidation) {
    auto* backend = xr_new<MetalBackend>();
    if (backend->Initialize(window, width, height, enableValidation))
        return backend;
    xr_delete(backend);
    return nullptr;
}
