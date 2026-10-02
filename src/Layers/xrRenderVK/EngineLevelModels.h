#pragma once

#include "LevelModels.h"
#include "ModelGeometry.h"
#include "SkeletonMotions.h"
#include "VisualCatalog.h"

class IReader;

namespace xray::render::vulkan
{
// The engine opens/decompresses level chunks, including archive-backed files.
// Returns false without changing result when geometry cannot be represented.
bool load_engine_level_models(IReader& level, LevelModelData& result, std::string& error);
bool load_engine_level_visibility(IReader& level, LevelModelData& result, std::string& error);
bool load_engine_visual_catalog(IReader& level, std::vector<VisualRecord>& result, std::string& error);
bool load_engine_model_visual(const char* name, VisualRecord& result, std::string& error);
bool load_engine_model_geometry(const char* name, IReader* source,
    ModelGeometry& result, std::string& error);
bool decode_engine_model_geometry(const VisualRecord& visual, ModelGeometry& result, std::string& error);
bool load_engine_motion_files(const std::string& pattern, std::vector<MotionFile>& files, std::string& error);
}
