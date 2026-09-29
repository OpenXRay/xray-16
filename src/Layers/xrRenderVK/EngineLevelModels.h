#pragma once

#include "LevelModels.h"
#include "VisualCatalog.h"

class IReader;

namespace xray::render::vulkan
{
// The engine opens/decompresses level chunks, including archive-backed files.
// Returns false without changing result when geometry cannot be represented.
bool load_engine_level_models(IReader& level, LevelModelData& result, std::string& error);
bool load_engine_visual_catalog(IReader& level, std::vector<VisualRecord>& result, std::string& error);
bool load_engine_model_visual(const char* name, VisualRecord& result, std::string& error);
}
