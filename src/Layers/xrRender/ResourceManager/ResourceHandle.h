#pragma once

#include "../RenderContext/ResourceHandle.h"

namespace xray::render::resources {

using xray::render::ng::ResourceHandle;
using xray::render::ng::TextureHandle;
using xray::render::ng::BufferHandle;
using xray::render::ng::SamplerHandle;

constexpr u32 INVALID_RESOURCE_INDEX = 0xFFFFFFFF;

}
