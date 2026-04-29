#include "stdafx.h"
#include "VariantTextureBuffer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::fg::bindless {

VariantTextureBuffer& VariantTextureBuffer::Instance()
{
    static VariantTextureBuffer instance;
    return instance;
}

void VariantTextureBuffer::Initialize(fg::RenderDevice* device)
{
    if (IsInitialized())
        return;

    m_data.resize(MAX_MATERIALS);
    for (auto& d : m_data)
        for (u32 i = 0; i < MAX_VARIANT_TEXTURE_SLOTS; i++)
            d.tex[i] = INVALID_TEXTURE_INDEX;

    if (!GPUStructuredBuffer::Initialize(device, "Bindless_VariantTextureBuffer", MAX_MATERIALS))
    {
        Msg("! [VariantTexture] Failed to create variant texture buffer");
        return;
    }

    m_uploadCount = MAX_MATERIALS;

    if (GEnv.Backend)
        GEnv.Backend->UploadBufferData(m_buffer, m_data.data(), MAX_MATERIALS * sizeof(VariantTextureData));

    Msg("* [VariantTexture] Buffer initialized (max: %u materials, %u KB)",
        MAX_MATERIALS, (MAX_MATERIALS * sizeof(VariantTextureData)) / 1024);
}

} // namespace xray::render::fg::bindless
