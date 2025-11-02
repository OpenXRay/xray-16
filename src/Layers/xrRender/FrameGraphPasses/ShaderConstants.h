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
// CRITICAL: HLSL float3x4 = 48 bytes (3 rows), float4x4 = 64 bytes (4 rows)
// We CANNOT use Fmatrix (64 bytes) for float3x4 - it would shift all offsets!
struct alignas(16) GlobalConstants {
    // View and projection matrices
    // m_V is float3x4 in HLSL = 48 bytes (3 rows of float4)
    float m_V[12];             // 0-48:   View matrix (3x4 row-major)

    // m_P is float4x4 in HLSL = 64 bytes (4 rows of float4)
    float m_P[16];             // 48-112: Projection matrix (4x4 row-major)

    // m_VP is float4x4 in HLSL = 64 bytes (4 rows of float4)
    float m_VP[16];            // 112-176: View-Projection matrix (4x4 row-major)

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

    // m_V is float3x4 (12 floats: 3 rows of 4 floats)
    // X-Ray's mView is column-major, HLSL expects row-major, so transpose
    Fmatrix viewT;
    viewT.transpose(Device.mView);
    cb.m_V[0]  = viewT._11; cb.m_V[1]  = viewT._12; cb.m_V[2]  = viewT._13; cb.m_V[3]  = viewT._14;
    cb.m_V[4]  = viewT._21; cb.m_V[5]  = viewT._22; cb.m_V[6]  = viewT._23; cb.m_V[7]  = viewT._24;
    cb.m_V[8]  = viewT._31; cb.m_V[9]  = viewT._32; cb.m_V[10] = viewT._33; cb.m_V[11] = viewT._34;

    // m_P is float4x4 (16 floats: 4 rows of 4 floats)
    Fmatrix projT;
    projT.transpose(Device.mProject);
    cb.m_P[0]  = projT._11; cb.m_P[1]  = projT._12; cb.m_P[2]  = projT._13; cb.m_P[3]  = projT._14;
    cb.m_P[4]  = projT._21; cb.m_P[5]  = projT._22; cb.m_P[6]  = projT._23; cb.m_P[7]  = projT._24;
    cb.m_P[8]  = projT._31; cb.m_P[9]  = projT._32; cb.m_P[10] = projT._33; cb.m_P[11] = projT._34;
    cb.m_P[12] = projT._41; cb.m_P[13] = projT._42; cb.m_P[14] = projT._43; cb.m_P[15] = projT._44;

    // m_VP is float4x4 (16 floats: 4 rows of 4 floats)
    Fmatrix tempVP;
    tempVP.mul(Device.mProject, Device.mView);
    Fmatrix vpT;
    vpT.transpose(tempVP);
    cb.m_VP[0]  = vpT._11; cb.m_VP[1]  = vpT._12; cb.m_VP[2]  = vpT._13; cb.m_VP[3]  = vpT._14;
    cb.m_VP[4]  = vpT._21; cb.m_VP[5]  = vpT._22; cb.m_VP[6]  = vpT._23; cb.m_VP[7]  = vpT._24;
    cb.m_VP[8]  = vpT._31; cb.m_VP[9]  = vpT._32; cb.m_VP[10] = vpT._33; cb.m_VP[11] = vpT._34;
    cb.m_VP[12] = vpT._41; cb.m_VP[13] = vpT._42; cb.m_VP[14] = vpT._43; cb.m_VP[15] = vpT._44;

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
