#pragma once

#include "../RenderContext/ResourceHandle.h"

namespace xray::render::resources {

using xray::render::fg::ResourceHandle;
using xray::render::fg::TextureHandle;
using xray::render::fg::BufferHandle;
using xray::render::fg::SamplerHandle;

constexpr u32 INVALID_RESOURCE_INDEX = 0xFFFFFFFF;

}
