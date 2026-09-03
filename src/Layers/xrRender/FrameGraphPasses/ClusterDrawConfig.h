#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg::passes {

struct ClusterDrawConfig {
    nvrhi::IBuffer* entryBuffer = nullptr;
    nvrhi::IBuffer* visibleEntryBuffer = nullptr;
    nvrhi::IBuffer* fadeBuffer = nullptr;
    nvrhi::IBuffer* argsBuffer = nullptr;
    nvrhi::IBuffer* instanceBuffer = nullptr;
    nvrhi::IBuffer* dynamicInstanceBuffer = nullptr;
    nvrhi::IBuffer* dynamicPrevWorldBuffer = nullptr;
    u32 entryCount = 0;

    nvrhi::IBuffer* terrainVisibleEntryBuffer = nullptr;
    nvrhi::IBuffer* terrainFadeBuffer = nullptr;
    nvrhi::IBuffer* terrainArgsBuffer = nullptr;
    nvrhi::IBuffer* terrainInstanceBuffer = nullptr;
    u32 terrainEntryCount = 0;

    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;

    bool IsValid() const {
        return entryBuffer && visibleEntryBuffer && fadeBuffer && argsBuffer && instanceBuffer && entryCount > 0;
    }

    bool TerrainValid() const {
        return entryBuffer && terrainVisibleEntryBuffer && terrainFadeBuffer && terrainArgsBuffer && terrainInstanceBuffer && terrainEntryCount > 0;
    }

    bool UseMegaBuffers() const {
        return megaVertexBuffer && megaIndexBuffer;
    }
};

}
