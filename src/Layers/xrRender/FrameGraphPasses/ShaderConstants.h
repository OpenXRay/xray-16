// xrRender/FrameGraphPasses/ShaderConstants.h
#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_matrix.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "xrEngine/device.h"

// Forward declarations of X-Ray engine globals
extern ECORE_API float ps_r2_sun_lumscale_hemi;
extern ECORE_API float ps_r2_sun_lumscale_amb;
extern ECORE_API float ps_r2_sun_normal_bias;
extern ECORE_API float ps_r2_sun_soft;
extern ECORE_API float ps_r2_sun_blocker;
extern ECORE_API float ps_r2_sun_contact;
extern ENGINE_API int ps_fg_pbr_diffuse_mode;
extern ENGINE_API Fvector4 ps_dev_param_1;
extern ENGINE_API Fvector4 ps_dev_param_2;
extern ENGINE_API Fvector4 ps_dev_param_3;
extern ENGINE_API Fvector4 ps_dev_param_4;
extern ENGINE_API float psHUD_FOV;
extern ENGINE_API int ps_r_contact_shadows;
extern ENGINE_API float ps_r_contact_shadows_length;
extern ENGINE_API int ps_r_sky_ibl;
extern ENGINE_API float ps_r_sky_ibl_intensity;
extern ENGINE_API int ps_r_foliage_sss;
extern ENGINE_API float ps_r_foliage_sss_intensity;
extern ECORE_API u32 ps_r_sun_quality;
namespace xray::render {
    namespace fg {
        extern float r__dtex_range;  // Detail texture range (defined in TextureDescrManager.cpp)
    }
}

namespace xray::render::fg::passes {

// Filled by CascadedShadows pass; consumed by FillGlobalConstants
struct ShadowCascadeGPUData
{
    Fmatrix matrices[4];
    Fvector4 splits;
    Fvector2 viewShadowProj; // far-cascade UV direction for accum_sun_far fade
    float mapSize = 2048.f;
    bool valid = false;
};
extern ShadowCascadeGPUData g_ShadowCascadeGPUData;

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

// Keep cluster_params / cluster_scales in sync with ClusteredLightManager.
// Every write to Frame/StaticGlobals must call this (or BuildStaticGlobals).
inline void FillClusterParams(StaticGlobals& cb)
{
    auto& clm = ::xray::render::fg::ClusteredLightManager::Instance();
    const u32 w = std::max(1u, (u32)Device.dwWidth);
    const u32 h = std::max(1u, (u32)Device.dwHeight);
    const float zNear = VIEWPORT_NEAR;
    const float zFar = g_pGamePersistent
        ? g_pGamePersistent->Environment().CurrentEnv.far_plane
        : 500.f;

    if (clm.IsReady())
    {
        const ::xray::render::fg::ClusterCB cluster = clm.BuildClusterCB(w, h, zNear, zFar);
        cb.cluster_params = cluster.gridDims; // xyz = tilesX/Y/slices, w = numLights
        cb.cluster_scales.set(zNear, zFar, cluster.depthParams.z, cluster.depthParams.w);
    }
    else
    {
        const u32 tilesX = (w + ::xray::render::fg::CLUSTER_TILE_SIZE - 1) / ::xray::render::fg::CLUSTER_TILE_SIZE;
        const u32 tilesY = (h + ::xray::render::fg::CLUSTER_TILE_SIZE - 1) / ::xray::render::fg::CLUSTER_TILE_SIZE;
        cb.cluster_params.set(
            static_cast<float>(tilesX),
            static_cast<float>(tilesY),
            static_cast<float>(::xray::render::fg::CLUSTER_NUM_SLICES),
            0.0f);
        cb.cluster_scales.set(zNear, zFar, 1.0f, static_cast<float>(::xray::render::fg::CLUSTER_TILE_SIZE));
    }
}

inline void FillGlobalConstants(GlobalConstants& cb) {
    cb.m_V = Device.mView;
    cb.m_P = Device.mProject;
    // MUST match geometry + GPU cull (Device.mFullTransform), not a second mul.
    // Remul(mProject,mView) can drift from mFullTransform and break depth↔UV SSR.
    cb.m_VP = Device.mFullTransform;

    // Classic R2/R3 binder (Blender_Recorder_StandartBinding cl_times):
    //   (t, t*10, t/10, sin(t)) — watermove_tc / clouds use timers.z = t/10
    const float t = Device.fTimeGlobal;
    cb.timers.set(t, t * 10.f, t / 10.f, _sin(t));

    // Classic R2/R3 fog packing (Blender_Recorder_StandartBinding):
    // fog_params: x=-n/(f-n), y=z=w=1/(f-n) → fog = saturate(d*w + x)
    if (g_pGamePersistent)
    {
        const auto& env = g_pGamePersistent->Environment().CurrentEnv;
        const float n = env.fog_near;
        const float f = std::max(env.fog_far, n + 1.0f);
        const float r = 1.0f / (f - n);
        cb.fog_params.set(-n * r, r, r, r);
        cb.fog_color.set(env.fog_color.x, env.fog_color.y, env.fog_color.z,
            env.rain_density);

        Fvector4 plane;
        const Fmatrix& M = Device.mFullTransform;
        plane.x = -(M._14 + M._13);
        plane.y = -(M._24 + M._23);
        plane.z = -(M._34 + M._33);
        plane.w = -(M._44 + M._43);
        const float denom = -1.0f / _sqrt(_sqr(plane.x) + _sqr(plane.y) + _sqr(plane.z));
        plane.mul(denom);
        const float B = r;
        cb.fog_plane.set(-plane.x * B, -plane.y * B, -plane.z * B, 1.0f - (plane.w - n) * B);
    }
    else
    {
        cb.fog_plane.set(0.0f, 1.0f, 0.0f, 0.0f);
        cb.fog_params.set(0.0f, 0.001f, 0.001f, 0.001f);
        cb.fog_color.set(0.5f, 0.5f, 0.6f, 0.0f);
    }

    // Lighting - defaults, will be overridden by FillSunConstants if sun is available
    cb.L_ambient.set(0.2f, 0.2f, 0.2f, 1.0f);  // Ambient (placeholder)
    cb.L_sun_color.set(1.0f, 0.95f, 0.9f);     // Warm sunlight (placeholder)
    cb.pbr_diffuse_mode = (float)ps_fg_pbr_diffuse_mode;
    cb.L_sun_dir_w.set(0.577f, -0.577f, 0.577f);  // Diagonal down (placeholder)
    cb.L_hemi_color.set(0.3f, 0.4f, 0.5f, ps_r2_sun_lumscale_hemi);

    // Camera position
    cb.eye_position = Device.vCameraPosition;

    const float VertTan = -1.0f * tanf(deg2rad(Device.fFOV / 2.0f));
    const float HorzTan = -VertTan / Device.fASPECT;

    // Vertex decompression (used for quantized positions)
    cb.pos_decompression_params.set(HorzTan, VertTan, (2.0f * HorzTan) / (float)Device.dwWidth, (2.0f * VertTan) / (float)Device.dwHeight);
    cb.pos_decompression_params2.set((float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth, 1.0f / (float)Device.dwHeight);

    // Parallax mapping: .x = height scale (r2_parallax_h; 0 disables object parallax),
    // .y = bias, .z = foliage SSS intensity, .w = sky IBL intensity. Object steep
    // parallax in bindless_forward.ps keys off .x>0 and requires r2_steep_parallax.
    const float parallaxH = ps_r2_ls_flags.test(R2FLAG_STEEP_PARALLAX) ? ps_r2_df_parallax_h : 0.0f;
    cb.parallax.set(parallaxH, -0.01f,
        (ps_r_foliage_sss != 0) ? ps_r_foliage_sss_intensity : 0.0f,
        (ps_r_sky_ibl != 0) ? ps_r_sky_ibl_intensity : 0.0f);

    // Screen resolution (for UI shaders and other effects)
    cb.screen_res.set(
        (float)Device.dwWidth,              // x = width
        (float)Device.dwHeight,             // y = height
        1.0f / (float)Device.dwWidth,       // z = 1/width
        1.0f / (float)Device.dwHeight       // w = 1/height
    );

    cb.hud_fov = psHUD_FOV;
    // Screen-space contact shadow length (0 = disabled); packed as StaticGlobals.padding3
    cb.padding3 = (ps_r_contact_shadows != 0) ? ps_r_contact_shadows_length : 0.0f;

    // ═══════════════════════════════════════════════════════
    //  FORWARD+ EXTENSIONS (Phase 1.3)
    // ═══════════════════════════════════════════════════════

    // Same inverse the rest of the frame / TAA / wet use — never remul+invert alone.
    cb.m_InvVP = Device.mInvFullTransform;

    static bool s_loggedVp = false;
    if (!s_loggedVp)
    {
        Fmatrix remul;
        remul.mul(Device.mProject, Device.mView);
        const float d =
            _abs(remul._11 - Device.mFullTransform._11) +
            _abs(remul._22 - Device.mFullTransform._22) +
            _abs(remul._33 - Device.mFullTransform._33) +
            _abs(remul._43 - Device.mFullTransform._43);
        Msg("* [SSR/VP] StaticGlobals m_VP := mFullTransform (remul delta=%.6f)", d);
        s_loggedVp = true;
    }

    if (g_ShadowCascadeGPUData.valid)
    {
        for (int i = 0; i < 4; i++)
            cb.shadow_matrices[i] = g_ShadowCascadeGPUData.matrices[i];
        cb.cascade_splits = g_ShadowCascadeGPUData.splits;
        // .w = UV rim blend width; keep if zero (older path)
        if (cb.cascade_splits.w <= 0.f)
            cb.cascade_splits.w = 0.12f;
    }
    else
    {
        for (int i = 0; i < 4; i++)
            cb.shadow_matrices[i].identity();
        cb.cascade_splits.set(10.0f, 50.0f, 150.0f, 500.0f);
    }

    // Cluster grid: filled by FillClusterParams (must run on every StaticGlobals write)
    FillClusterParams(cb);

    // Camera direction; .w = soft particles enable (r2_soft_particles)
    cb.camera_direction.set(
        Device.vCameraDirection.x, Device.vCameraDirection.y, Device.vCameraDirection.z,
        ps_r2_ls_flags.test(R2FLAG_SOFT_PARTICLES) ? 1.0f : 0.0f);

    cb.dev_param_1 = ps_dev_param_1;
    cb.dev_param_2 = ps_dev_param_2;
    cb.dev_param_3 = ps_dev_param_3;
    cb.dev_param_4 = ps_dev_param_4;

    // r_shadow_debug overlay mode -> shadow_sampling.h ShadowDebugColor
    cb.dev_param_3.z = float(ps_r_shadow_debug);
    // r2_sun_normal_bias (meters) -> shadow_sampling.h NormalOffsetWorld
    cb.dev_param_3.w = ps_r2_sun_normal_bias;

    // PCSS filtering controls -> shadow_sampling.h SampleCascadePCSS.
    // (dev_param_4 was previously view_shadow_proj, which no shader reads.)
    cb.dev_param_4.x = ps_r2_sun_soft;     // max penumbra (texels) — softness
    cb.dev_param_4.y = ps_r2_sun_blocker;  // blocker-search spacing (texels)
    cb.dev_param_4.z = ps_r2_sun_contact;  // min penumbra (texels) — contact sharpness

    if (g_ShadowCascadeGPUData.valid)
    {
        cb.dev_param_3.x = g_ShadowCascadeGPUData.mapSize;
        // r2_sun_quality → soft PCSS tier in shadow_sampling.h (dev_param_3.y)
        cb.dev_param_3.y = float(ps_r_sun_quality);
    }
}

inline void FillDynamicTransforms(DynamicTransforms& cb, Fmatrix m_W = Fidentity) {
    cb.m_W = m_W;
    cb.m_WV.mul_43(Device.mView, m_W);
    cb.m_WVP.mul(Device.mProject, cb.m_WV);

    cb.L_material.set(0.01903f, 0.74998f, 0.0f, 0.25f);
    cb.hemi_cube_pos_faces.set(0.08034f, 0.42066f, 0.13277f, 0.0f);
    cb.hemi_cube_neg_faces.set(0.19919f, 0.00392f, 0.09922f, 0.0f);
}

// Soft rain / thunderbolt FX (effects_world_soft.*)
struct alignas(16) SoftFXConstants {
    Fmatrix m_WVP;
    Fvector4 EyePos;
};

inline void FillSoftFXConstants(SoftFXConstants& cb)
{
    cb.m_WVP.mul(Device.mProject, Device.mView);
    cb.EyePos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 1.f);
}

struct SunLightData {
    Fvector color;      // Sun color (RGB)
    Fvector direction;  // Sun travel direction (world space, sun → surface; env.sun_dir)
    float intensity;    // HDR intensity multiplier (1.0 = SDR, 2.0+ = HDR)
};

inline void FillSunConstants(StaticGlobals& cb, const SunLightData& sun) {
    if (!g_pGamePersistent)
    {
        cb.L_sun_color.set(0.f, 0.f, 0.f);
        cb.L_sun_dir_w.set(0.f, -1.f, 0.f);
        return;
    }

    const auto& desc = g_pGamePersistent->Environment().CurrentEnv;

    cb.L_sun_color.set(
        sun.color.x * sun.intensity,
        sun.color.y * sun.intensity,
        sun.color.z * sun.intensity
    );

    Fvector dir = sun.direction;
    if (dir.magnitude() < 1e-4f)
        dir.set(0.f, -1.f, 0.f);
    dir.normalize_safe();
    cb.L_sun_dir_w.set(dir.x, dir.y, dir.z);

    cb.L_ambient.set(
        desc.ambient.x * ps_r2_sun_lumscale_amb,
        desc.ambient.y * ps_r2_sun_lumscale_amb,
        desc.ambient.z * ps_r2_sun_lumscale_amb,
        desc.weight
    );

    // 3-arg set keeps L_hemi_color.w (ps_r2_sun_lumscale_hemi from FillGlobalConstants)
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
    FillClusterParams(sg);
    return sg;
}

} // namespace xray::render::fg::passes
