#pragma once

#include "GPUStructuredBuffer.h"
#include "BindlessTypes.h"

namespace xray::render { class ShaderVariantRegistry; }

namespace xray::render::fg::bindless {

constexpr u32 MAX_SHADER_VARIANTS = 256;

enum VariantFlags : u32 {
    VARIANT_FLAG_FOG = (1 << 0),
    VARIANT_FLAG_DISTORT = (1 << 1),
    VARIANT_FLAG_TRANSPARENT = (1 << 2),
    VARIANT_FLAG_BACK_TO_FRONT = (1 << 3),
    VARIANT_FLAG_EMISSIVE = (1 << 4),
    VARIANT_FLAG_NO_SHADOW = (1 << 5),
};

struct alignas(16) VariantData {
    float emissive;
    u32 flags;
    u32 packed;
    float fadeScale;
};
static_assert(sizeof(VariantData) == 16, "VariantData must be 16 bytes");

class VariantBuffer : public GPUStructuredBuffer<VariantData> {
public:
    static VariantBuffer& Instance();

    void Initialize(fg::RenderDevice* device);
    void Rebuild(const ShaderVariantRegistry& registry);

private:
    VariantBuffer() = default;
};

} // namespace xray::render::fg::bindless
