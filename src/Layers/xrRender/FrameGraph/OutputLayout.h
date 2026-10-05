#pragma once

#include "FrameGraph.h"

namespace xray::render::framegraph {

inline constexpr nvrhi::Format kSceneColorFormat = nvrhi::Format::RGBA16_FLOAT;
inline constexpr nvrhi::Format kSceneNormalFormat = nvrhi::Format::RGBA16_FLOAT;
inline constexpr nvrhi::Format kSceneBaseColorFormat = nvrhi::Format::RGBA16_FLOAT;
inline constexpr nvrhi::Format kSceneMaterialFormat = nvrhi::Format::RGBA8_UNORM;
inline constexpr nvrhi::Format kSceneDepthFormat = nvrhi::Format::D32;
inline constexpr nvrhi::Format kSceneDistortionFormat = nvrhi::Format::RGBA16_FLOAT;

struct DefaultOutputLayout {
    VirtualResourceHandle albedo;      // RT0: Lit HDR color (RGBA16_FLOAT)
    VirtualResourceHandle normal;      // RT1: World normal.xyz + Roughness.a (RGBA16_FLOAT)
    VirtualResourceHandle baseColor;   // RT2: Unlit diffuse albedo.rgb + Metallic.a (RGBA16_FLOAT)
    VirtualResourceHandle material;    // RT3: Shading class.r + Transmission.g + octahedral geometric normal.ba (RGBA8_UNORM)
    VirtualResourceHandle depth;       // Depth/Stencil (D32)
    VirtualResourceHandle distortion;  // Distortion buffer (RG = UV offset, A = intensity)
};

}
