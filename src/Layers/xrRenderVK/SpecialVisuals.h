#pragma once

#include "LevelModels.h"

#include <array>

namespace xray::render::vulkan
{
// OGF_LODDEF2: eight baked views, four vertices per view. A facet is selected
// from the camera direction at submission time; nearby draws use its children.
struct LodFacets
{
    std::array<LevelModel, 8> meshes;
    std::array<std::array<float, 3>, 8> normals{};
};
bool decode_lod_facets(LevelBytes bytes, uint16_t material, LodFacets& result,
    std::string& error);
uint8_t select_lod_facet(const std::array<std::array<float, 3>, 8>& normals,
    const std::array<float, 3>& direction);

// OGF_TREEDEF2 stores an object transform and two five-channel color terms.
// Static geometry uses the same quantization as the legacy tree vertex shader.
bool transform_tree_vertices(LevelBytes definition, LevelModel& model, std::string& error);
bool decode_tree_windows(LevelBytes table, uint32_t item,
    const LevelModel& model, std::vector<SlideWindow>& result, std::string& error);
} // namespace xray::render::vulkan
