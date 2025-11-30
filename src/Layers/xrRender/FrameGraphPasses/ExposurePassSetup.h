// xrRender/FrameGraphPasses/ExposurePassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

// Forward declarations
namespace xray::render {
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// ═══════════════════════════════════════════════════════
//  EXPOSURE PASS (Auto-Exposure / Eye Adaptation)
// ═══════════════════════════════════════════════════════
//
// Computes scene exposure for HDR rendering using histogram-based
// auto-exposure with temporal eye adaptation.
//
// PIPELINE:
// 1. Luminance Histogram - Compute shader generates 64-bin histogram
//    from HDR scene in log2 luminance space
// 2. Exposure Adaptation - Compute shader analyzes histogram,
//    skips extreme values, computes target exposure, applies
//    temporal smoothing for eye adaptation effect
//
// OUTPUT:
// - 1x1 R32_FLOAT texture containing exposure value
// - Sky pass reads this via s_tonemap.Load(int3(0,0,0)).x
// - Tonemap pass uses same exposure for HDR->LDR conversion
//
// REFERENCES:
// - Krzysztof Narkowicz: "Automatic Exposure" (2016)
// - Epic Games: "Auto Exposure in UE 4.25" (2020)
// - Hillaire: "A Scalable and Production Ready Sky and Atmosphere" (2020)

// Exposure configuration
struct ExposureConfig {
    // Histogram parameters
    float minLogLuminance = -10.0f;  // Minimum log2 luminance (EV)
    float maxLogLuminance = 4.0f;    // Maximum log2 luminance (EV)

    // Percentile clamping (skip extreme values)
    float lowPercentile = 0.5f;      // Skip darkest 50% of pixels
    float highPercentile = 0.98f;    // Skip brightest 2% of pixels

    // Eye adaptation speed (f-stops per second)
    float adaptSpeedUp = 3.0f;       // Speed when brightening
    float adaptSpeedDown = 1.0f;     // Speed when darkening (slower)

    // Exposure limits
    float minExposure = 0.001f;      // Minimum exposure value
    float maxExposure = 64.0f;       // Maximum exposure value

    // Calibration
    float exposureCompensation = 0.0f;  // Manual EV adjustment
    float calibrationConstant = 12.5f;  // Reflected-light meter constant K
};

// Output handles from exposure pass
struct ExposureOutput {
    framegraph::VirtualResourceHandle exposureTexture;  // 1x1 R32_FLOAT
    framegraph::VirtualResourceHandle histogramBuffer;  // 64 u32 bins (for debug)
};

// Setup the exposure pass
// Returns handle to 1x1 exposure texture
ExposureOutput setupExposurePass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle hdrSceneColor,  // HDR scene to analyze
    const ExposureConfig& config,
    float deltaTime,  // Frame delta for temporal adaptation
    u32 width,
    u32 height
);

// Get default exposure config (Earth-like, game-friendly)
ExposureConfig GetDefaultExposureConfig();

// Get the physical NVRHI exposure texture directly (for tonemap pass)
// This bypasses the framegraph virtual resource system to avoid state transition issues
nvrhi::ITexture* GetExposureTexture();

} // namespace xray::render::RENDER_NAMESPACE::passes
