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
    if (!data || data->TotalVtxCount == 0)
        return;

    nvrhi::ICommandList* cmdList = nullptr;
    if (!cmdList)
    {
        Msg("! ImGuiRendererNVRHI::Render - No command list available");
        return;
    }

    RenderDrawData(data, cmdList);
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

    if (!CreateFontsTexture())
    {
        Msg("! Failed to create ImGui font texture");
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
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_constantBuffer = nullptr;
    m_fontTexture = nullptr;
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
    vbDesc.structStride = sizeof(ImDrawVert);
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

bool ImGuiRendererNVRHI::CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    if (!io.Fonts->TexData)
    {
        Msg("! Font atlas TexData is null after build");
        return false;
    }

    unsigned char* pixels = io.Fonts->TexData->Pixels;
    int width = io.Fonts->TexData->Width;
    int height = io.Fonts->TexData->Height;
    int bytesPerPixel = io.Fonts->TexData->BytesPerPixel;

    if (!pixels || width == 0 || height == 0)
    {
        Msg("! Font atlas has no pixel data");
        return false;
    }

    if (bytesPerPixel != 4)
    {
        Msg("! Font atlas is not in RGBA32 format (has %d bytes per pixel)", bytesPerPixel);
        return false;
    }

    nvrhi::TextureDesc texDesc;
    texDesc.setWidth(width);
    texDesc.setHeight(height);
    texDesc.setDepth(1);
    texDesc.setArraySize(1);
    texDesc.setMipLevels(1);
    texDesc.setDimension(nvrhi::TextureDimension::Texture2D);
    texDesc.setFormat(nvrhi::Format::RGBA8_UNORM);
    texDesc.setDebugName("ImGui Font Texture");

    size_t dataSize = width * height * 4;

    m_fontTexture = m_device->createTexture(texDesc);
    if (!m_fontTexture)
        return false;

    io.Fonts->TexData->SetTexID((ImTextureID)(intptr_t)m_fontTexture.Get());

    if (io.Fonts->TexData->Pixels)
    {
        io.Fonts->TexData->DestroyPixels();
    }

    return true;
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
