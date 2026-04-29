// xrRender/FrameGraphPasses/ShaderConstants.h
#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_matrix.h"

// Forward declarations of X-Ray engine globals
extern ECORE_API float ps_r2_sun_lumscale_hemi;
extern ENGINE_API int ps_fg_pbr_diffuse_mode;
extern ENGINE_API Fvector4 ps_dev_param_1;
extern ENGINE_API Fvector4 ps_dev_param_2;
extern ENGINE_API Fvector4 ps_dev_param_3;
extern ENGINE_API Fvector4 ps_dev_param_4;
extern ENGINE_API float psHUD_FOV;
namespace xray::render {
    namespace fg {
        extern float r__dtex_range;  // Detail texture range (defined in TextureDescrManager.cpp)
    }
}

namespace xray::render::fg::passes {

// ══════════════════════════════════════════════════════════
//  PBR TEXTURE SLOT ASSIGNMENTS (Forward+ Rendering)
// ══════════════════════════════════════════════════════════
// These slots are reserved for PBR textures in Forward+ rendering.
// Must match shader expectations in res/gamedata/shaders/r5/forward/

constexpr u32 TEX_SLOT_DIFFUSE   = 0;  // Base color / diffuse (legacy)
constexpr u32 TEX_SLOT_BUMP      = 1;  // Normal map (bump)
constexpr u32 TEX_SLOT_BUMP_X    = 2;  // Normal map secondary channel
constexpr u32 TEX_SLOT_DETAIL    = 3;  // Detail texture

constexpr u32 TEX_SLOT_PBR       = 4;  // PBR: Packed texture (R=metallic, G=roughness, B=ao, A=parallax)

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

// Slot 1: Dynamic Transforms
// UPDATED PER-DRAW! Contains world/view/projection for current object.
struct alignas(16) DynamicTransforms {
    Fmatrix m_WVP;
    Fmatrix m_WV;
    Fmatrix m_W;
    Fvector4 L_material;
    Fvector4 hemi_cube_pos_faces;
    Fvector4 hemi_cube_neg_faces;
};

// Shader Params (Material-frequency constants, register b1)
// UPDATED PER-MATERIAL! Contains alpha ref and detail texture params.
struct alignas(16) ShaderParams {
    float m_AlphaRef;          // 0-4:    Alpha reference value for alpha testing
    float padding[3];          // 4-16:   Padding to align dt_params
    Fvector4 dt_params;        // 16-32:  Detail texture params (xy=scale, w=1/range)
};

// Skinned Material CB (per-draw constant, register b4)
// Used by pixel shader to index into bindless material buffer.
struct alignas(16) SkinnedMaterialCB {
    u32 materialID;
    u32 skeletonBoneOffset;
    u32 splatOffset;
    u32 splatCount;
};

// Slot 2: Static Globals (EXTENDED for Forward+)
// UPDATED ONCE PER FRAME! Contains view/projection matrices, lighting, fog, etc.
struct alignas(16) StaticGlobals {
    Fmatrix m_V;
    Fmatrix m_P;
    Fmatrix m_VP;

    Fvector4 timers;
    Fvector4 fog_plane;
    Fvector4 fog_params;
    Fvector4 fog_color;

    Fvector4 L_ambient;
    Fvector3 L_sun_color;
    float pbr_diffuse_mode;
    Fvector3 L_sun_dir_w;
    float hud_fov;
    Fvector4 L_hemi_color;

    Fvector3 eye_position;
    float padding3;

    Fvector4 pos_decompression_params;
    Fvector4 pos_decompression_params2;

    Fvector4 parallax;
    Fvector4 screen_res;

    Fmatrix m_InvVP;

    Fmatrix shadow_matrices[4];
    Fvector4 cascade_splits;

    Fvector4 cluster_params;
    Fvector4 cluster_scales;

    Fvector4 camera_direction;

    Fvector4 dev_param_1;
    Fvector4 dev_param_2;
    Fvector4 dev_param_3;
    Fvector4 dev_param_4;
};
static_assert(sizeof(StaticGlobals) == 848, "StaticGlobals must be 848 bytes");

// Legacy alias for compatibility
using GlobalConstants = StaticGlobals;

inline void FillGlobalConstants(GlobalConstants& cb) {
    cb.m_V = Device.mView;
    cb.m_P = Device.mProject;
    cb.m_VP.mul(Device.mProject, Device.mView);

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

    // Lighting - defaults, will be overridden by FillSunConstants if sun is available
    cb.L_ambient.set(0.2f, 0.2f, 0.2f, 1.0f);  // Ambient (placeholder)
    cb.L_sun_color.set(1.0f, 0.95f, 0.9f);     // Warm sunlight (placeholder)
    cb.pbr_diffuse_mode = (float)ps_fg_pbr_diffuse_mode;
    cb.L_sun_dir_w.set(0.577f, -0.577f, 0.577f);  // Diagonal down (placeholder)
    cb.L_hemi_color.set(0.3f, 0.4f, 0.5f, ps_r2_sun_lumscale_hemi);

    // Camera position
    cb.eye_position = Device.vCameraPosition;

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
    cb.hud_fov = psHUD_FOV;
    cb.padding3 = 0.0f;

    // ═══════════════════════════════════════════════════════
    //  FORWARD+ EXTENSIONS (Phase 1.3)
    // ═══════════════════════════════════════════════════════

    cb.m_InvVP.invert(cb.m_VP);

    for (int i = 0; i < 4; i++)
        cb.shadow_matrices[i].identity();
    cb.cascade_splits.set(10.0f, 50.0f, 150.0f, 500.0f);

    // Cluster grid parameters (PLACEHOLDER - Phase 5: will be populated from light culling pass)
    cb.cluster_params.set(16.0f, 16.0f, 24.0f, 0.0f);  // 16×16×24 grid, 0 lights for now
    cb.cluster_scales.set(0.1f, 500.0f, 1.0f, 1.0f);   // z_near, z_far, scale_x, scale_y

    // Camera direction vector (for lighting calculations)
    cb.camera_direction.set(Device.vCameraDirection.x, Device.vCameraDirection.y,
                            Device.vCameraDirection.z, 0.0f);

    cb.dev_param_1 = ps_dev_param_1;
    cb.dev_param_2 = ps_dev_param_2;
    cb.dev_param_3 = ps_dev_param_3;
    cb.dev_param_4 = ps_dev_param_4;
}

inline void FillDynamicTransforms(DynamicTransforms& cb, Fmatrix m_W = Fidentity) {
    cb.m_W = m_W;
    cb.m_WV.mul_43(Device.mView, m_W);
    cb.m_WVP.mul(Device.mProject, cb.m_WV);

    cb.L_material.set(0.01903f, 0.74998f, 0.0f, 0.25f);
    cb.hemi_cube_pos_faces.set(0.08034f, 0.42066f, 0.13277f, 0.0f);
    cb.hemi_cube_neg_faces.set(0.19919f, 0.00392f, 0.09922f, 0.0f);
}

struct SunLightData {
    Fvector color;      // Sun color (RGB)
    Fvector direction;  // Sun direction (world space, pointing toward light)
    float intensity;    // HDR intensity multiplier (1.0 = SDR, 2.0+ = HDR)
};

inline void FillSunConstants(StaticGlobals& cb, const SunLightData& sun) {
    const auto& desc = g_pGamePersistent->Environment().CurrentEnv;

    cb.L_sun_color.set(
        sun.color.x * sun.intensity,
        sun.color.y * sun.intensity,
        sun.color.z * sun.intensity
    );

    cb.L_sun_dir_w.set(
        sun.direction.x,
        sun.direction.y,
        sun.direction.z
    );

    cb.L_ambient.set(
        desc.ambient.x,
        desc.ambient.y,
        desc.ambient.z
    );

    cb.L_hemi_color.set(
        desc.hemi_color.x,
        desc.hemi_color.y,
        desc.hemi_color.z
    );
}

void GetSunLightData(SunLightData& outSun, float hdrIntensity = 2.0f);

inline StaticGlobals BuildStaticGlobals(float hdrIntensity = 2.0f) {
    StaticGlobals sg = {};
    FillGlobalConstants(sg);
    SunLightData sunData;
    GetSunLightData(sunData, hdrIntensity);
    FillSunConstants(sg, sunData);
    return sg;
}

} // namespace xray::render::fg::passes
