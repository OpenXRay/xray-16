#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{
enum class TextureColorSpace : u8
{
    Linear,
    Srgb
};

nvrhi::Format FormatForColorSpace(nvrhi::Format format, TextureColorSpace colorSpace);

float SrgbToLinear(float value);
Fvector SrgbToLinear(const Fvector& color);
}
