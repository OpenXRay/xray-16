#pragma once

#include "GPUStructuredBuffer.h"
#include "BindlessTypes.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

constexpr u32 MAX_VARIANT_TEXTURE_SLOTS = 8;

struct alignas(16) VariantTextureData {
    u32 tex[MAX_VARIANT_TEXTURE_SLOTS];
};
static_assert(sizeof(VariantTextureData) == 32, "VariantTextureData must be 32 bytes");

class VariantTextureBuffer : public GPUStructuredBuffer<VariantTextureData> {
public:
    static VariantTextureBuffer& Instance();

    void Initialize(fg::RenderDevice* device);

    void SetVariantTextures(u32 materialID, const VariantTextureData& data) { Set(materialID, data); }
    const VariantTextureData* GetVariantTextures(u32 materialID) const { return Get(materialID); }

private:
    VariantTextureBuffer() = default;
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
