#pragma once

#include "LevelModels.h"

#include <array>

namespace xray::render::vulkan
{
// A lossless OGF catalogue for the level visual table and standalone model
// files. It preserves unsupported vertex encodings and animation chunks for
// their dedicated runtime factories; parsing a record does not make it drawable.
struct VisualRecord
{
    uint8_t type{};
    uint16_t shader_id{};
    std::array<float, 10> bounds{};
    std::string shader;
    std::string texture;
    std::vector<uint32_t> linked_children;
    std::vector<VisualRecord> embedded_children;
    std::vector<uint8_t> source;
};

bool parse_ogf_visual(LevelBytes bytes, VisualRecord& result, std::string& error);
bool parse_level_visuals(LevelBytes bytes, std::vector<VisualRecord>& result, std::string& error);
}
