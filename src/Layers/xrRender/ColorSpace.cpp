#include "stdafx.h"
#include "ColorSpace.h"

#include <cmath>

namespace xray::render::fg
{
nvrhi::Format FormatForColorSpace(nvrhi::Format format, TextureColorSpace colorSpace)
{
    const bool srgb = colorSpace == TextureColorSpace::Srgb;
    switch (format)
    {
    case nvrhi::Format::RGBA8_UNORM:
    case nvrhi::Format::SRGBA8_UNORM:
        return srgb ? nvrhi::Format::SRGBA8_UNORM : nvrhi::Format::RGBA8_UNORM;
    case nvrhi::Format::BGRA8_UNORM:
    case nvrhi::Format::SBGRA8_UNORM:
        return srgb ? nvrhi::Format::SBGRA8_UNORM : nvrhi::Format::BGRA8_UNORM;
    case nvrhi::Format::BC1_UNORM:
    case nvrhi::Format::BC1_UNORM_SRGB:
        return srgb ? nvrhi::Format::BC1_UNORM_SRGB : nvrhi::Format::BC1_UNORM;
    case nvrhi::Format::BC2_UNORM:
    case nvrhi::Format::BC2_UNORM_SRGB:
        return srgb ? nvrhi::Format::BC2_UNORM_SRGB : nvrhi::Format::BC2_UNORM;
    case nvrhi::Format::BC3_UNORM:
    case nvrhi::Format::BC3_UNORM_SRGB:
        return srgb ? nvrhi::Format::BC3_UNORM_SRGB : nvrhi::Format::BC3_UNORM;
    case nvrhi::Format::BC7_UNORM:
    case nvrhi::Format::BC7_UNORM_SRGB:
        return srgb ? nvrhi::Format::BC7_UNORM_SRGB : nvrhi::Format::BC7_UNORM;
    default:
        return format;
    }
}

float SrgbToLinear(float value)
{
    if (value < 0.0f)
        return -SrgbToLinear(-value);
    if (value <= 0.04045f)
        return value / 12.92f;
    return std::pow((value + 0.055f) / 1.055f, 2.4f);
}

Fvector SrgbToLinear(const Fvector& color)
{
    return Fvector().set(SrgbToLinear(color.x), SrgbToLinear(color.y), SrgbToLinear(color.z));
}
}
