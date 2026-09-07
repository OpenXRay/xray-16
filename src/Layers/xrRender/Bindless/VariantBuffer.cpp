#include "stdafx.h"
#include "VariantBuffer.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::fg::bindless {

VariantBuffer& VariantBuffer::Instance()
{
    static VariantBuffer instance;
    return instance;
}

void VariantBuffer::Initialize(fg::RenderDevice* device)
{
    if (IsInitialized())
        return;
    if (!GPUStructuredBuffer::Initialize(device, "Bindless_VariantBuffer", MAX_SHADER_VARIANTS))
    {
        Msg("! [VariantBuffer] Failed to create shader variant buffer");
        return;
    }
    m_data.assign(MAX_SHADER_VARIANTS, VariantData{ 0.0f, VARIANT_FLAG_FOG, 0u, 0u });
    if (GEnv.Backend)
        GEnv.Backend->UploadBufferData(m_buffer, m_data.data(), MAX_SHADER_VARIANTS * sizeof(VariantData));
}

void VariantBuffer::Rebuild(const ShaderVariantRegistry& registry)
{
    if (!IsInitialized())
        return;
    const u32 count = registry.GetVariantCount();
    R_ASSERT2(count <= MAX_SHADER_VARIANTS, "Shader variant table overflow");
    for (u32 i = 0; i < MAX_SHADER_VARIANTS; ++i)
    {
        VariantData d = { 0.0f, VARIANT_FLAG_FOG, 0u, 0u };
        if (const ShaderVariantDesc* v = i < count ? registry.GetVariantByIndex(i) : nullptr)
        {
            d.emissive = v->emissive ? v->emissiveIntensity : 0.0f;
            d.flags = (v->fog ? VARIANT_FLAG_FOG : 0u) | (v->distort ? VARIANT_FLAG_DISTORT : 0u)
                | (v->transparent ? VARIANT_FLAG_TRANSPARENT : 0u) | (v->backToFront ? VARIANT_FLAG_BACK_TO_FRONT : 0u)
                | (v->emissive ? VARIANT_FLAG_EMISSIVE : 0u)
                | (v->castsShadow ? 0u : VARIANT_FLAG_NO_SHADOW);
        }
        m_data[i] = d;
    }
    if (GEnv.Backend)
        GEnv.Backend->UploadBufferData(m_buffer, m_data.data(), MAX_SHADER_VARIANTS * sizeof(VariantData));
    m_dirtyRangeStart = UINT32_MAX;
    m_dirtyRangeEnd = 0;
}

} // namespace xray::render::fg::bindless
