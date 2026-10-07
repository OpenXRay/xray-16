#pragma once

#include "xrCore/stdafx.h"
#include "LevelModels.h"
#include "xrCore/_matrix.h"

namespace xray::render::vulkan
{
// Portal traversal is a conservative optimization: false means the caller
// must submit every level root to avoid losing visible geometry.
bool select_visible_sector_roots(const std::vector<LevelSector>& sectors,
    const std::vector<LevelPortal>& portals, size_t visual_count, size_t camera_sector,
    const Fmatrix& view_projection, const Fvector& camera_position,
    std::vector<uint32_t>& roots);

// Sector traversal only accounts for sector-owned roots. Keep independent
// level roots (for example outdoor visual hierarchies) in every valid view.
void append_unsectored_level_roots(const std::vector<uint32_t>& level_roots,
    const std::vector<LevelSector>& sectors, size_t visual_count,
    std::vector<uint32_t>& visible_roots);
}
