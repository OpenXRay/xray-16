#pragma once

#include "LevelModels.h"
#include "ModelGeometry.h"
#include "SkeletonMotions.h"
#include "VisualCatalog.h"
#include "DetailAssets.h"

class IReader;

namespace xray::render::vulkan
{
// The engine opens/decompresses level chunks, including archive-backed files.
// Returns false without changing result when geometry cannot be represented.
bool load_engine_level_models(IReader& level, LevelModelData& result, std::string& error);
bool load_engine_level_visibility(IReader &level, LevelModelData &result, std::string &error);
bool load_engine_detail_assets(DetailAssets& result, std::string& error);
bool load_engine_visual_catalog(IReader &level, std::vector<VisualRecord> &result, std::string &error);
bool load_engine_model_visual(const char *name, VisualRecord &result, std::string &error);
bool load_engine_model_reader(IReader &reader, const char *name, VisualRecord &result, std::string &error);
bool normalize_engine_visual_chunks(IReader &reader, bool table, std::vector<uint8_t> &result, std::string &error);
bool load_engine_model_geometry(const char *name, IReader *source, ModelGeometry &result, std::string &error);
bool decode_engine_model_geometry(const VisualRecord &visual, ModelGeometry &result, std::string &error);
bool load_engine_motion_files(const std::string& pattern, std::vector<MotionFile>& files, std::string& error);
}
