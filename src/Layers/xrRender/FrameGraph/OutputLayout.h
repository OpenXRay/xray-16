#pragma once

#include "FrameGraph.h"

namespace xray::render::framegraph {

inline constexpr nvrhi::Format kSceneColorFormat = nvrhi::Format::RGBA16_FLOAT;
inline constexpr nvrhi::Format kSceneNormalFormat = nvrhi::Format::RGBA16_FLOAT;
inline constexpr nvrhi::Format kSceneBaseColorFormat = nvrhi::Format::RGBA16_FLOAT;
inline constexpr nvrhi::Format kSceneMaterialFormat = nvrhi::Format::RG8_UNORM;
inline constexpr nvrhi::Format kSceneDepthFormat = nvrhi::Format::D32;
inline constexpr nvrhi::Format kSceneDistortionFormat = nvrhi::Format::RGBA16_FLOAT;

struct DefaultOutputLayout {
    VirtualResourceHandle albedo;
    VirtualResourceHandle normal;
    VirtualResourceHandle baseColor;
    VirtualResourceHandle material;
    VirtualResourceHandle depth;
    VirtualResourceHandle distortion;
};

}
