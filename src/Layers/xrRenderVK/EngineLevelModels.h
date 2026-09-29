#pragma once

#include "LevelModels.h"

class IReader;

namespace xray::render::vulkan
{
// The engine opens/decompresses level chunks, including archive-backed files.
// Returns false without changing result when geometry cannot be represented.
bool load_engine_level_models(IReader& level, LevelModelData& result, std::string& error);
}
