// xrRender/FrameGraphPasses/ShaderConstants.h
#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_matrix.h"

// Forward declarations of X-Ray engine globals
namespace xray::render {
    namespace RENDER_NAMESPACE {
        extern float r__dtex_range;  // Detail texture range (defined in TextureDescrManager.cpp)
    }
}

namespace xray::render::RENDER_NAMESPACE::passes {

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
    float padding2[8];        // 192-256: Remaining padding to 256 bytes
};
static_assert(sizeof(PerObjectConstants) == 256, "PerObjectConstants must be 256 bytes");

// Slot 1: Dynamic Transforms (224 bytes minimum)
// UPDATED PER-DRAW! Contains world/view/projection for current object.
// For static geometry: m_W = identity, for dynamic: m_W = object's world matrix
// CRITICAL: HLSL float3x4 = 48 bytes (3 rows), float4x4 = 64 bytes (4 rows)
struct alignas(16) DynamicTransforms {
    // Combined matrices (computed per-draw)
    float m_WVP[16];           // 0-64:   World-View-Projection (float4x4)
    float m_WV[12];            // 64-112: World-View (float3x4)
    float m_W[12];             // 112-160: World matrix (float3x4)
                               //         IDENTITY for static geometry!

    // Material/lighting per-draw params
    Fvector4 L_material;       // 160-176: Material params (0,0,0,mid)
    Fvector4 hemi_cube_pos_faces;  // 176-192: Hemisphere cube pos faces
    Fvector4 hemi_cube_neg_faces;  // 192-208: Hemisphere cube neg faces
    // dt_params MOVED to ShaderParams (b1) - it's material-frequency, not instance-frequency
};

// Shader Params (Material-frequency constants, register b1)
// UPDATED PER-MATERIAL! Contains alpha ref and detail texture params.
struct alignas(16) ShaderParams {
    float m_AlphaRef;          // 0-4:    Alpha reference value for alpha testing
    float padding[3];          // 4-16:   Padding to align dt_params
    Fvector4 dt_params;        // 16-32:  Detail texture params (xy=scale, w=1/range)
};

// Slot 2: Static Globals (EXTENDED for Forward+)
// UPDATED ONCE PER FRAME! Contains view/projection matrices, lighting, fog, etc.
// CRITICAL: HLSL float3x4 = 48 bytes (3 rows), float4x4 = 64 bytes (4 rows)
// We CANNOT use Fmatrix (64 bytes) for float3x4 - it would shift all offsets!
//
// PHASE 1.3: Extended with Forward+ data (inverse VP, shadow cascades, cluster grid)
struct alignas(16) StaticGlobals {
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
    Fvector4 screen_res;       // 368-384: Screen resolution (x=width, y=height, z=1/width, w=1/height)

    // ═══════════════════════════════════════════════════════
    //  FORWARD+ EXTENSIONS (Phase 1.3+)
    // ═══════════════════════════════════════════════════════

    // Inverse View-Projection for position reconstruction from depth
    float m_InvVP[16];         // 384-448: Inverse View-Projection matrix (float4x4)

    // Shadow cascade matrices (Phase 4: CSM shadows)
    float shadow_matrices[4][16]; // 448-704: Shadow view-projection matrices (4×float4x4)
    Fvector4 cascade_splits;   // 704-720: Cascade split distances (x, y, z, w)

    // Cluster grid parameters (Phase 5: Clustered light culling)
    Fvector4 cluster_params;   // 720-736: (grid_dim_x, grid_dim_y, grid_dim_z, num_lights)
    Fvector4 cluster_scales;   // 736-752: (z_near, z_far, scale_x, scale_y)

    // Camera direction vector (for lighting calculations)
    Fvector4 camera_direction; // 752-768: Camera forward vector (xyz) + padding (w)
};
static_assert(sizeof(StaticGlobals) == 768, "StaticGlobals must be 768 bytes");

// Legacy alias for compatibility
using GlobalConstants = StaticGlobals;

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

    /*
    static class cl_pos_decompress_params : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
#if defined(USE_DX11)
        const float VertTan = -1.0f * tanf(deg2rad(Device.fFOV / 2.0f));
        const float HorzTan = -VertTan / Device.fASPECT;
#elif defined(USE_OGL)
        const float VertTan = tanf(deg2rad(Device.fFOV / 2.0f));
        const float HorzTan = VertTan / Device.fASPECT;
#else
#   error No graphics API selected or enabled!
#endif
        cmd_list.set_c(
            C, HorzTan, VertTan, (2.0f * HorzTan) / (float)Device.dwWidth, (2.0f * VertTan) / (float)Device.dwHeight);
    }
} binder_pos_decompress_params;

static class cl_pos_decompress_params2 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, (float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth,
            1.0f / (float)Device.dwHeight);
    }
} binder_pos_decompress_params2;
    */

#if defined(USE_DX11)
    const float VertTan = -1.0f * tanf(deg2rad(Device.fFOV / 2.0f));
    const float HorzTan = -VertTan / Device.fASPECT;
#elif defined(USE_OGL)
    const float VertTan = tanf(deg2rad(Device.fFOV / 2.0f));
    const float HorzTan = VertTan / Device.fASPECT;
#else
#   error No graphics API selected or enabled!
#endif

    // Vertex decompression (used for quantized positions)
    cb.pos_decompression_params.set(HorzTan, VertTan, (2.0f * HorzTan) / (float)Device.dwWidth, (2.0f * VertTan) / (float)Device.dwHeight);
    cb.pos_decompression_params2.set((float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth, 1.0f / (float)Device.dwHeight);

    // Parallax mapping
    cb.parallax.set(0.02f, -0.01f, 0.0f, 0.0f);  // height scale, min samples, max samples, unused

    // Screen resolution (for UI shaders and other effects)
    cb.screen_res.set(
        (float)Device.dwWidth,              // x = width
        (float)Device.dwHeight,             // y = height
        1.0f / (float)Device.dwWidth,       // z = 1/width
        1.0f / (float)Device.dwHeight       // w = 1/height
    );

    // Clear padding to avoid uninitialized memory warnings
    cb.padding1 = 0.0f;
    cb.padding2 = 0.0f;
    cb.padding3 = 0.0f;

    // ═══════════════════════════════════════════════════════
    //  FORWARD+ EXTENSIONS (Phase 1.3)
    // ═══════════════════════════════════════════════════════

    // Inverse View-Projection matrix (for position reconstruction from depth)
    Fmatrix invVP;
    invVP.invert(tempVP);
    Fmatrix invVPT;
    invVPT.transpose(invVP);
    cb.m_InvVP[0]  = invVPT._11; cb.m_InvVP[1]  = invVPT._12; cb.m_InvVP[2]  = invVPT._13; cb.m_InvVP[3]  = invVPT._14;
    cb.m_InvVP[4]  = invVPT._21; cb.m_InvVP[5]  = invVPT._22; cb.m_InvVP[6]  = invVPT._23; cb.m_InvVP[7]  = invVPT._24;
    cb.m_InvVP[8]  = invVPT._31; cb.m_InvVP[9]  = invVPT._32; cb.m_InvVP[10] = invVPT._33; cb.m_InvVP[11] = invVPT._34;
    cb.m_InvVP[12] = invVPT._41; cb.m_InvVP[13] = invVPT._42; cb.m_InvVP[14] = invVPT._43; cb.m_InvVP[15] = invVPT._44;

    // Shadow cascade matrices (PLACEHOLDER - Phase 4: will be populated from shadow pass)
    for (int i = 0; i < 4; i++) {
        Fmatrix identity;
        identity.identity();
        Fmatrix identityT;
        identityT.transpose(identity);

        cb.shadow_matrices[i][0]  = identityT._11; cb.shadow_matrices[i][1]  = identityT._12;
        cb.shadow_matrices[i][2]  = identityT._13; cb.shadow_matrices[i][3]  = identityT._14;
        cb.shadow_matrices[i][4]  = identityT._21; cb.shadow_matrices[i][5]  = identityT._22;
        cb.shadow_matrices[i][6]  = identityT._23; cb.shadow_matrices[i][7]  = identityT._24;
        cb.shadow_matrices[i][8]  = identityT._31; cb.shadow_matrices[i][9]  = identityT._32;
        cb.shadow_matrices[i][10] = identityT._33; cb.shadow_matrices[i][11] = identityT._34;
        cb.shadow_matrices[i][12] = identityT._41; cb.shadow_matrices[i][13] = identityT._42;
        cb.shadow_matrices[i][14] = identityT._43; cb.shadow_matrices[i][15] = identityT._44;
    }
    cb.cascade_splits.set(10.0f, 50.0f, 150.0f, 500.0f);

    // Cluster grid parameters (PLACEHOLDER - Phase 5: will be populated from light culling pass)
    cb.cluster_params.set(16.0f, 16.0f, 24.0f, 0.0f);  // 16×16×24 grid, 0 lights for now
    cb.cluster_scales.set(0.1f, 500.0f, 1.0f, 1.0f);   // z_near, z_far, scale_x, scale_y

    // Camera direction vector (for lighting calculations)
    cb.camera_direction.set(Device.vCameraDirection.x, Device.vCameraDirection.y,
                            Device.vCameraDirection.z, 0.0f);
}

inline void FillDynamicTransforms(DynamicTransforms& cb, Fmatrix m_W = Fidentity) {
    Fmatrix wv, wvp;
    wv.mul_43(Device.mView, m_W);           // WV = View * World
    wvp.mul(Device.mProject, wv);        // WVP = Projection *

    // Transpose for HLSL (column-major -> row-major)
    Fmatrix wvpT, wvT, wT;
    wvpT.transpose(wvp);
    wvT.transpose(wv);

    wT.identity();
    wT.transpose(m_W);  // World = identity for static geometry

    // m_WVP (float4x4, 64 bytes)
    cb.m_WVP[0]  = wvpT._11; cb.m_WVP[1]  = wvpT._12; cb.m_WVP[2]  = wvpT._13; cb.m_WVP[3]  = wvpT._14;
    cb.m_WVP[4]  = wvpT._21; cb.m_WVP[5]  = wvpT._22; cb.m_WVP[6]  = wvpT._23; cb.m_WVP[7]  = wvpT._24;
    cb.m_WVP[8]  = wvpT._31; cb.m_WVP[9]  = wvpT._32; cb.m_WVP[10] = wvpT._33; cb.m_WVP[11] = wvpT._34;
    cb.m_WVP[12] = wvpT._41; cb.m_WVP[13] = wvpT._42; cb.m_WVP[14] = wvpT._43; cb.m_WVP[15] = wvpT._44;

    // m_WV (float3x4, 48 bytes)
    cb.m_WV[0]  = wvT._11; cb.m_WV[1]  = wvT._12; cb.m_WV[2]  = wvT._13; cb.m_WV[3]  = wvT._14;
    cb.m_WV[4]  = wvT._21; cb.m_WV[5]  = wvT._22; cb.m_WV[6]  = wvT._23; cb.m_WV[7]  = wvT._24;
    cb.m_WV[8]  = wvT._31; cb.m_WV[9]  = wvT._32; cb.m_WV[10] = wvT._33; cb.m_WV[11] = wvT._34;

    // m_W (float3x4, 48 bytes)
    cb.m_W[0]  = wT._11; cb.m_W[1]  = wT._12; cb.m_W[2]  = wT._13; cb.m_W[3]  = wT._14;
    cb.m_W[4]  = wT._21; cb.m_W[5]  = wT._22; cb.m_W[6]  = wT._23; cb.m_W[7]  = wT._24;
    cb.m_W[8]  = wT._31; cb.m_W[9]  = wT._32; cb.m_W[10] = wT._33; cb.m_W[11] = wT._34;

    // Material/lighting params (TODO: Get from X-Ray's material/environment system)
    cb.L_material.set(0.01903f, 0.74998f, 0.0f, 0.25f);  // Vanilla values from RenderDoc
    cb.hemi_cube_pos_faces.set(0.08034f, 0.42066f, 0.13277f, 0.0f);
    cb.hemi_cube_neg_faces.set(0.19919f, 0.00392f, 0.09922f, 0.0f);

    // dt_params moved to ShaderParams (b1) - material-frequency, not instance-frequency
}

} // namespace xray::render::passes
