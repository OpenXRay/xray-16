#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"
#include <backends/imgui_impl_dx11.h>

namespace xray::render::ng {

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

void ImGuiRendererNVRHI::Copy(IImGuiRender& _in)
{
    *this = *dynamic_cast<ImGuiRendererNVRHI*>(&_in);
}

void ImGuiRendererNVRHI::Frame()
{
}

void ImGuiRendererNVRHI::Render(ImDrawData* data)
{
    if (!data || data->TotalVtxCount == 0 || data->TotalIdxCount == 0)
        return;

    ProcessTextureRequests(data);

    if (!m_pipeline || !m_resourceBindings)
    {
        return;
    }

    static bool logged = false;
    if (!logged && m_resourceBindings)
    {
        Msg("* ImGui NVRHI renderer ready - waiting for framebuffer integration");
        logged = true;
    }
}

void ImGuiRendererNVRHI::OnDeviceCreate(ImGuiContext* context)
{
    ImGui::SetAllocatorFunctions(
        [](size_t size, void* )
        {
            return xr_malloc(size);
        },
        [](void* ptr, void* )
        {
            xr_free(ptr);
        }
    );

    m_imguiContext = context;
    ImGui::SetCurrentContext(context);

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "xrRender_NVRHI";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

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
    DestroyDeviceObjects();
}

void ImGuiRendererNVRHI::OnDeviceResetEnd()
{
    CreateDeviceObjects();
}

bool ImGuiRendererNVRHI::CreateDeviceObjects()
{
    if (!m_device)
        return false;

    if (!CreateShaders())
    {
        Msg("! Failed to create ImGui shaders");
        return false;
    }

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setMinFilter(true);
    samplerDesc.setMagFilter(true);
    samplerDesc.setMipFilter(true);
    samplerDesc.setAddressU(nvrhi::SamplerAddressMode::Wrap);
    samplerDesc.setAddressV(nvrhi::SamplerAddressMode::Wrap);
    samplerDesc.setAddressW(nvrhi::SamplerAddressMode::Wrap);
    m_fontSampler = m_device->createSampler(samplerDesc);

    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(ImGuiConstants);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.debugName = "ImGui Constants";
    m_constantBuffer = m_device->createBuffer(cbDesc);

    if (!CreatePipelineState())
    {
        Msg("! Failed to create ImGui pipeline state");
        return false;
    }

    if (!CreateBuffers(5000, 10000))
    {
        Msg("! Failed to create ImGui vertex/index buffers");
        return false;
    }

    return true;
}

void ImGuiRendererNVRHI::DestroyDeviceObjects()
{
    m_textures.clear();
    m_fontTexture = nullptr;

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

    m_vertexBufferSize = 0;
    m_indexBufferSize = 0;
}

bool ImGuiRendererNVRHI::CreateBuffers(size_t vtxSize, size_t idxSize)
{
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = vtxSize * sizeof(ImDrawVert);
    vbDesc.isVertexBuffer = true;
    vbDesc.isVolatile = true;
    vbDesc.debugName = "ImGui Vertex Buffer";

    m_vertexBuffer = m_device->createBuffer(vbDesc);
    if (!m_vertexBuffer)
        return false;

    m_vertexBufferSize = vtxSize;

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = idxSize * sizeof(ImDrawIdx);
    ibDesc.isIndexBuffer = true;
    ibDesc.isVolatile = true;
    ibDesc.debugName = "ImGui Index Buffer";

    m_indexBuffer = m_device->createBuffer(ibDesc);
    if (!m_indexBuffer)
        return false;

    m_indexBufferSize = idxSize;

    return true;
}

bool ImGuiRendererNVRHI::ResizeBuffers(size_t vtxSize, size_t idxSize)
{
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;

    return CreateBuffers(vtxSize, idxSize);
}

void ImGuiRendererNVRHI::ProcessTextureRequests(ImDrawData* drawData)
{
    if (!drawData || !drawData->Textures)
        return;

    ImVector<ImTextureData*>* textures = drawData->Textures;
    for (int i = 0; i < textures->Size; i++)
    {
        ImTextureData* texData = (*textures)[i];
        if (!texData)
            continue;

        switch (texData->Status)
        {
        case ImTextureStatus_WantCreate:
            {
                Msg("* ImGui requesting texture creation: %dx%d", texData->Width, texData->Height);

                nvrhi::TextureDesc texDesc;
                texDesc.setWidth(texData->Width);
                texDesc.setHeight(texData->Height);
                texDesc.setDepth(1);
                texDesc.setArraySize(1);
                texDesc.setMipLevels(1);
                texDesc.setDimension(nvrhi::TextureDimension::Texture2D);

                nvrhi::Format nvrhiFormat = nvrhi::Format::RGBA8_UNORM;
                if (texData->Format == ImTextureFormat_Alpha8)
                    nvrhiFormat = nvrhi::Format::R8_UNORM;

                texDesc.setFormat(nvrhiFormat);
                texDesc.setDebugName("ImGui Texture");

                nvrhi::TextureHandle texture = m_device->createTexture(texDesc);
                if (texture)
                {
                    if (texData->Pixels && m_renderDevice)
                    {
                        size_t dataSize = texData->Width * texData->Height * texData->BytesPerPixel;
                        m_renderDevice->UploadTextureDataToNVRHI(
                            texture.Get(),
                            0, 0,
                            texData->Pixels,
                            dataSize
                        );
                    }

                    m_textures[(ImTextureID)(intptr_t)texture.Get()] = texture;
                    texData->SetTexID((ImTextureID)(intptr_t)texture.Get());
                    texData->SetStatus(ImTextureStatus_OK);

                    if (!m_fontTexture)
                    {
                        m_fontTexture = texture;
                        Msg("* ImGui font texture created (%dx%d)", texData->Width, texData->Height);

                        if (!m_resourceBindings && !CreateResourceBindings())
                        {
                            Msg("! Failed to create ImGui resource bindings after font texture creation");
                        }
                    }
                }
                else
                {
                    Msg("! Failed to create ImGui texture");
                }
            }
            break;

        case ImTextureStatus_WantUpdates:
            {
                ImTextureID texId = texData->GetTexID();
                auto it = m_textures.find(texId);
                if (it != m_textures.end() && m_renderDevice)
                {
                    for (int j = 0; j < texData->Updates.Size; j++)
                    {
                        const ImTextureRect& update = texData->Updates[j];

                        const void* updateData = (const u8*)texData->Pixels +
                            (update.y * texData->Width + update.x) * texData->BytesPerPixel;

                        size_t dataSize = update.w * update.h * texData->BytesPerPixel;

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
                ImTextureID texId = texData->GetTexID();
                auto it = m_textures.find(texId);
                if (it != m_textures.end())
                {
                    if (it->second == m_fontTexture)
                        m_fontTexture = nullptr;

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

xr_unique_ptr<IImGuiRender> ImGuiRendererFactory::Create(RenderDevice* device)
{
    if (device && device->GetNVRHIDevice())
    {
        IImGuiRender* renderer = xr_new<ImGuiRendererNVRHI>(device);
        return xr_unique_ptr<IImGuiRender>(renderer);
    }

    return nullptr;
}

}
