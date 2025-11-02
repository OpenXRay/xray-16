// xrRender/FrameGraphPasses/ShaderConstants.h
#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_matrix.h"

namespace xray::render::passes {

// ══════════════════════════════════════════════════════════
//  SHADER CONSTANT BUFFER LAYOUTS
// ══════════════════════════════════════════════════════════
// These structs match the HLSL constant buffer layouts from X-Ray shaders.
// Layout extracted from shader reflection and debugger inspection.

// Slot 0: Per-Object Constants (256 bytes)
// Updated per-draw using Volatile Constant Buffer (VCB)
struct alignas(16) PerObjectConstants {
    Fmatrix m_xform;           // 0-48:   Object world transform (3x4 matrix)
    Fmatrix m_xform_v;         // 48-96:  Object view-space transform
    Fvector4 consts;           // 96-112: Generic constants
    Fvector4 c_scale;          // 112-128: Scale factors
    Fvector4 c_bias;           // 128-144: Bias values
    Fvector4 wind;             // 144-160: Wind parameters
    Fvector4 wave;             // 160-176: Wave parameters
    Fvector2 c_sun;            // 176-184: Sun parameters
    float padding[2];          // 184-192: Padding to 16-byte alignment
    float padding2[16];        // 192-256: Remaining padding to 256 bytes
};
//static_assert(sizeof(PerObjectConstants) == 256, "PerObjectConstants must be 256 bytes");

// Slot 1: Global/Static Constants (368 bytes minimum, often 512 bytes allocated)
// Contains view/projection matrices, lighting, fog, etc.
struct alignas(16) GlobalConstants {
    // View and projection matrices
    Fmatrix m_V;               // 0-48:   View matrix (3x4)
    Fmatrix m_P;               // 48-112: Projection matrix (4x4)
    Fmatrix m_VP;              // 112-176: View-Projection matrix (4x4)

    // Timing
    Fvector4 timers;           // 176-192: x=game time, y=frame time, z=sin(time), w=cos(time)

    // Fog
    Fvector4 fog_plane;        // 192-208: Fog plane equation (ax + by + cz + d)
    Fvector4 fog_params;       // 208-224: x=fog near, y=fog far, z=density, w=?
    Fvector4 fog_color;        // 224-240: RGB fog color + alpha

    // Lighting
    Fvector4 L_ambient;        // 240-256: Ambient light color
    Fvector3 L_sun_color;      // 256-268: Sun light color
    float padding1;            // 268-272: Padding
    Fvector3 L_sun_dir_w;      // 272-284: Sun direction (world space)
    float padding2;            // 284-288: Padding
    Fvector4 L_hemi_color;     // 288-304: Hemisphere light color

    // Camera
    Fvector3 eye_position;     // 304-316: Camera position (world space)
    float padding3;            // 316-320: Padding

    // Vertex decompression (for compressed position attributes)
    Fvector4 pos_decompression_params;  // 320-336
    Fvector4 pos_decompression_params2; // 336-352

    // Misc
    Fvector4 parallax;         // 352-368: Parallax mapping parameters
};
//static_assert(sizeof(GlobalConstants) == 368, "GlobalConstants must be 368 bytes");

// Helper function to fill GlobalConstants from Device state
inline void FillGlobalConstants(GlobalConstants& cb) {
    // View/Projection matrices
    // CRITICAL: HLSL expects row-major float3x4/float4x4, but X-Ray stores column-major
    // We must TRANSPOSE the matrices when copying to CB!
    cb.m_V.transpose(Device.mView);
    cb.m_P.transpose(Device.mProject);

    // Compute view-projection (m_VP = m_V * m_P) AFTER transposing
    Fmatrix tempVP;
    tempVP.mul(Device.mProject, Device.mView);
    cb.m_VP.transpose(tempVP);

    // Timers
    cb.timers.set(
        Device.fTimeGlobal,           // Game time
        Device.fTimeDelta,            // Frame delta
        _sin(Device.fTimeGlobal),     // sin(time)
        _cos(Device.fTimeGlobal)      // cos(time)
    );

    // Fog (use X-Ray's global fog state if available, otherwise defaults)
    // TODO: Hook into X-Ray's CFogOfWar or environment system
    cb.fog_plane.set(0.0f, 1.0f, 0.0f, 0.0f);  // Plane equation
    cb.fog_params.set(0.0f, 1000.0f, 0.001f, 0.0f);  // near, far, density
    cb.fog_color.set(0.5f, 0.5f, 0.6f, 1.0f);  // Grayish-blue fog

    // Lighting (TODO: Hook into X-Ray's light manager)
    cb.L_ambient.set(0.2f, 0.2f, 0.2f, 1.0f);  // Ambient
    cb.L_sun_color.set(1.0f, 0.95f, 0.9f);     // Warm sunlight
    cb.L_sun_dir_w.set(0.577f, -0.577f, 0.577f);  // Diagonal down
    cb.L_hemi_color.set(0.3f, 0.4f, 0.5f, 1.0f);  // Sky color

    // Camera position
    cb.eye_position = Device.vCameraPosition;

    // Vertex decompression (used for quantized positions)
    cb.pos_decompression_params.set(1.0f, 1.0f, 1.0f, 1.0f);
    cb.pos_decompression_params2.set(0.0f, 0.0f, 0.0f, 0.0f);

    // Parallax mapping
    cb.parallax.set(0.02f, -0.01f, 0.0f, 0.0f);  // height scale, min samples, max samples, unused

    // Clear padding to avoid uninitialized memory warnings
    cb.padding1 = 0.0f;
    cb.padding2 = 0.0f;
    cb.padding3 = 0.0f;
}

} // namespace xray::render::passes
