#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"
#include "RenderContext/RenderContext.h"
#include <backends/imgui_impl_dx11.h> // For ImDrawVert layout reference
#include <d3d11.h>

// Include CRender for accessing backbuffer
#include "Layers/xrRender_R2/r2.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"

namespace xray::render::fg {

//=============================================================================
// Constructor/Destructor
//=============================================================================

ImGuiRendererNVRHI::ImGuiRendererNVRHI(RenderDevice* renderDevice)
    : m_renderDevice(renderDevice)
{
    VERIFY(renderDevice);
    m_device = nvrhi::DeviceHandle(renderDevice->GetNVRHIDevice());
}

ImGuiRendererNVRHI::~ImGuiRendererNVRHI()
{
    DestroyDeviceObjects();
}

//=============================================================================
// IImGuiRender Interface Implementation
//=============================================================================

void ImGuiRendererNVRHI::Copy(IImGuiRender& _in)
{
    *this = *dynamic_cast<ImGuiRendererNVRHI*>(&_in);
}

void ImGuiRendererNVRHI::Frame()
{
    // Nothing special needed for NVRHI - ImGui tracks its own state
    // This would be called at the start of each frame
}

void ImGuiRendererNVRHI::Render(ImDrawData* data)
{
    // This is the legacy entry point called from device.cpp
    // For now, we'll just log that rendering is disabled here
    // The actual rendering happens inline in FrameGraphRenderer

    static bool logged = false;
    if (!logged)
    {
        Msg("* ImGui legacy Render() called - actual rendering happens inline in FrameGraphRenderer");
        logged = true;
    }
}

void ImGuiRendererNVRHI::Render(ImDrawData* data, nvrhi::IFramebuffer* framebuffer, nvrhi::ICommandList* cmdList)
{
    if (!data || data->TotalVtxCount == 0 || data->TotalIdxCount == 0)
        return;

    if (!framebuffer || !cmdList)
    {
        Msg("! ImGuiRendererNVRHI::Render - Invalid framebuffer or command list");
        return;
    }

    // Process texture requests
    ProcessTextureRequests(data);

    // Check if we have all required resources
    if (!m_pipeline || !m_resourceBindings)
    {
        Msg("! ImGuiRendererNVRHI::Render - Pipeline or bindings not ready");
        return;
    }

    // Store framebuffer so RenderDrawData can use it in graphics state
    m_currentFramebuffer = framebuffer;

    // Render ImGui draw data
    RenderDrawData(data, cmdList);

    m_currentFramebuffer = nullptr;
}

void ImGuiRendererNVRHI::OnDeviceCreate(ImGuiContext* context)
{
    // Set custom memory allocators
    ImGui::SetAllocatorFunctions(
        [](size_t size, void* /*user_data*/)
        {
            return xr_malloc(size);
        },
        [](void* ptr, void* /*user_data*/)
        {
            xr_free(ptr);
        }
    );

    m_imguiContext = context;
    ImGui::SetCurrentContext(context);

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "xrRender_NVRHI";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We support ImDrawCmd::VtxOffset
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // We handle texture management (ImGui 1.92+)

    // Create all device objects
    if (!CreateDeviceObjects())
    {
        Msg("! Failed to create ImGui NVRHI device objects");
    }
}

void ImGuiRendererNVRHI::OnDeviceDestroy()
{
    DestroyDeviceObjects();
    m_imguiContext = nullptr;
}

void ImGuiRendererNVRHI::OnDeviceResetBegin()
{
    // Release resources that depend on swapchain
    DestroyDeviceObjects();
}

void ImGuiRendererNVRHI::OnDeviceResetEnd()
{
    CreateDeviceObjects();
}

void ImGuiRendererNVRHI::InvalidateShadersAndPipeline()
{
    m_vertexShader = nullptr;
    m_pixelShader = nullptr;
    m_inputLayout = nullptr;
    m_bindingLayout = nullptr;
    m_resourceBindings = nullptr;
    m_pipeline = nullptr;

    if (!CreateShaders() || !CreatePipelineState())
    {
        Msg("! ImGui shader hot-reload failed");
        return;
    }

    if (m_fontTexture)
        CreateResourceBindings();
}

//=============================================================================
// Phase 3: Resource Management Implementation
//=============================================================================

bool ImGuiRendererNVRHI::CreateDeviceObjects()
{
    if (!m_device)
        return false;

    // Create shaders
    if (!CreateShaders())
    {
        Msg("! Failed to create ImGui shaders");
        return false;
    }

    // Font texture creation is now handled through ProcessTextureRequests
    // with the modern ImGui 1.92+ API (ImGuiBackendFlags_RendererHasTextures)

    // Create sampler for font texture (matches vanilla imgui_impl_dx11.cpp sampler state)
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setMinFilter(true);  // Enable linear filtering
    samplerDesc.setMagFilter(true);  // Enable linear filtering
    samplerDesc.setMipFilter(true);  // Enable linear filtering
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);  // ClampToEdge for UVW
    m_fontSampler = m_device->createSampler(samplerDesc);

    // Create constant buffer for projection matrix
    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(ImGuiConstants);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true; // We update it every frame
    cbDesc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    cbDesc.debugName = "ImGui Constants";
    m_constantBuffer = m_device->createBuffer(cbDesc);

    // NOTE: With ImGuiBackendFlags_RendererHasTextures, the font texture
    // will be automatically requested by ImGui through ProcessTextureRequests()
    // during the first frame. We defer pipeline and binding creation until then.

    // Create pipeline state (without bindings yet - those will be created
    // after the font texture is available)
    if (!CreatePipelineState())
    {
        Msg("! Failed to create ImGui pipeline state");
        return false;
    }

    // Create initial vertex and index buffers (will resize as needed)
    if (!CreateBuffers(5000, 10000))
    {
        Msg("! Failed to create ImGui vertex/index buffers");
        return false;
    }

    return true;
}

void ImGuiRendererNVRHI::DestroyDeviceObjects()
{
    // Clear all textures (including font texture)
    m_textures.clear();
    m_fontTexture = nullptr;

    // Clear other resources
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_constantBuffer = nullptr;
    m_fontSampler = nullptr;
    m_vertexShader = nullptr;
    m_pixelShader = nullptr;
    m_inputLayout = nullptr;
    m_bindingLayout = nullptr;
    m_resourceBindings = nullptr;
    m_pipeline = nullptr;
    // m_renderState is a struct, not a handle, so no need to null it

    m_vertexBufferSize = 0;
    m_indexBufferSize = 0;
}

bool ImGuiRendererNVRHI::CreateBuffers(size_t vtxSize, size_t idxSize)
{
    // Create vertex buffer
    // NOTE: Using non-volatile buffers for vertex/index data.
    // Volatile buffers in NVRHI D3D12 have issues with IA binding -
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = vtxSize * sizeof(ImDrawVert);
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "ImGui Vertex Buffer";
    vbDesc.keepInitialState = true;
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;

    m_vertexBuffer = m_device->createBuffer(vbDesc);
    if (!m_vertexBuffer)
        return false;

    m_vertexBufferSize = vtxSize;

    // Create index buffer
    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = idxSize * sizeof(ImDrawIdx);
    ibDesc.isIndexBuffer = true;
    ibDesc.debugName = "ImGui Index Buffer";
    ibDesc.keepInitialState = true;
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

    m_indexBuffer = m_device->createBuffer(ibDesc);
    if (!m_indexBuffer)
        return false;

    m_indexBufferSize = idxSize;

    return true;
}

bool ImGuiRendererNVRHI::ResizeBuffers(size_t vtxSize, size_t idxSize)
{
    // Release old buffers
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;

    // Create new ones with increased size
    return CreateBuffers(vtxSize, idxSize);
}

void ImGuiRendererNVRHI::ProcessTextureRequests(ImDrawData* drawData)
{
    if (!drawData || !drawData->Textures)
        return;

    // Process texture requests from ImGui (modern 1.92+ API)
    ImVector<ImTextureData*>* textures = drawData->Textures;
    for (int i = 0; i < textures->Size; i++)
    {
        ImTextureData* texData = (*textures)[i];
        if (!texData)
            continue;

        // Handle texture state requests
        switch (texData->Status)
        {
        case ImTextureStatus_WantCreate:
            {
                // ImGui is requesting us to create a texture
                Msg("* ImGui requesting texture creation: %dx%d", texData->Width, texData->Height);

                // Create NVRHI texture
                nvrhi::TextureDesc texDesc;
                texDesc.setWidth(texData->Width);
                texDesc.setHeight(texData->Height);
                texDesc.setDepth(1);
                texDesc.setArraySize(1);
                texDesc.setMipLevels(1);
                texDesc.setDimension(nvrhi::TextureDimension::Texture2D);

                // Determine format based on ImGui's texture format
                nvrhi::Format nvrhiFormat = nvrhi::Format::RGBA8_UNORM;
                if (texData->Format == ImTextureFormat_Alpha8)
                    nvrhiFormat = nvrhi::Format::R8_UNORM;

                texDesc.setFormat(nvrhiFormat);
                texDesc.setDebugName("ImGui Texture");

                // D3D12 requires explicit state tracking for textures
                texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
                texDesc.keepInitialState = true;

                nvrhi::TextureHandle texture = m_device->createTexture(texDesc);
                if (texture)
                {
                    // Upload initial pixel data if provided
                    if (texData->Pixels && m_renderDevice)
                    {
                        size_t dataSize = texData->Width * texData->Height * texData->BytesPerPixel;
                        m_renderDevice->UploadTextureDataToNVRHI(
                            texture.Get(),
                            0, 0,  // Array slice 0, Mip level 0
                            texData->Pixels,
                            dataSize
                        );
                    }

                    // Store texture in our map and set the ID for ImGui
                    m_textures[(ImTextureID)(intptr_t)texture.Get()] = texture;
                    texData->SetTexID((ImTextureID)(intptr_t)texture.Get());
                    texData->SetStatus(ImTextureStatus_OK);

                    // If this is the first texture (font texture), store it and create bindings
                    if (!m_fontTexture)
                    {
                        m_fontTexture = texture;
                        Msg("* ImGui font texture created (%dx%d)", texData->Width, texData->Height);

                        // Now that we have the font texture, create the resource bindings
                        if (!m_resourceBindings && !CreateResourceBindings())
                        {
                            Msg("! Failed to create ImGui resource bindings after font texture creation");
                        }
                    }
                }
                else
                {
                    Msg("! Failed to create ImGui texture");
                    // Leave status as WantCreate so it will be retried
                }
            }
            break;

        case ImTextureStatus_WantUpdates:
            {
                // ImGui wants to update parts of the texture
                ImTextureID texId = texData->GetTexID();
                auto it = m_textures.find(texId);
                if (it != m_textures.end() && m_renderDevice)
                {
                    // Process texture updates
                    for (int j = 0; j < texData->Updates.Size; j++)
                    {
                        const ImTextureRect& update = texData->Updates[j];

                        // Calculate data pointer and size for this update region
                        const void* updateData = (const u8*)texData->Pixels +
                            (update.y * texData->Width + update.x) * texData->BytesPerPixel;

                        // Note: This is a simplified update - proper implementation would handle
                        // partial texture updates more efficiently
                        size_t dataSize = update.w * update.h * texData->BytesPerPixel;

                        // For now, re-upload the entire texture
                        // TODO: Implement partial texture updates
                        size_t fullDataSize = texData->Width * texData->Height * texData->BytesPerPixel;
                        m_renderDevice->UploadTextureDataToNVRHI(
                            it->second.Get(),
                            0, 0,
                            texData->Pixels,
                            fullDataSize
                        );
                    }
                    texData->SetStatus(ImTextureStatus_OK);
                }
            }
            break;

        case ImTextureStatus_WantDestroy:
            {
                // ImGui wants us to destroy this texture
                ImTextureID texId = texData->GetTexID();
                auto it = m_textures.find(texId);
                if (it != m_textures.end())
                {
                    // Check if it's the font texture
                    if (it->second == m_fontTexture)
                        m_fontTexture = nullptr;

                    // Release the texture
                    it->second = nullptr;
                    m_textures.erase(it);

                    texData->SetStatus(ImTextureStatus_Destroyed);
                    Msg("* ImGui texture destroyed");
                }
            }
            break;
        }
    }
}

//=============================================================================
// Factory Implementation
//=============================================================================

xr_unique_ptr<IImGuiRender> ImGuiRendererFactory::Create(RenderDevice* device)
{
    if (device && device->GetNVRHIDevice())
    {
        // Cast to base class pointer
        IImGuiRender* renderer = xr_new<ImGuiRendererNVRHI>(device);
        return xr_unique_ptr<IImGuiRender>(renderer);
    }

    return nullptr;
}

} // namespace xray::render::fg
