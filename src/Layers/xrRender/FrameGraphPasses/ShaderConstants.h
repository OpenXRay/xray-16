// xrRender/FrameGraphPasses/ShaderConstants.h
#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_matrix.h"
#include "xrEngine/device.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "Include/xrAPI/xrAPI.h"
#include <algorithm>

// Forward declarations of X-Ray engine globals
extern ECORE_API float ps_r2_sun_lumscale_hemi;
extern ECORE_API float ps_r2_sun_lumscale_amb;
extern ECORE_API Flags32 ps_r2_ls_flags;
extern ECORE_API float ps_r2_df_parallax_h;
extern ENGINE_API int ps_fg_pbr_diffuse_mode;
extern ENGINE_API int ps_r_rt_gi;
extern ENGINE_API float ps_r_rt_gi_ambient_scale;
extern ENGINE_API int ps_r_path_tracer;
extern bool g_restirPipelinesReady;
extern bool g_restirReplaceForward;
extern ENGINE_API int ps_r_hdr10;
extern ENGINE_API float ps_r_hdr10_hud;
extern ENGINE_API float ps_r_hdr10_paper_white;
extern ENGINE_API int ps_r_vol_fog;
extern ENGINE_API int ps_r_atmosphere;
extern ENGINE_API float ps_r_atmosphere_strength;
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

inline u32& RenderResW() { static u32 w = 0; return w; }
inline u32& RenderResH() { static u32 h = 0; return h; }
inline void SetRenderResolution(u32 w, u32 h)
{
    RenderResW() = std::max(1u, w);
    RenderResH() = std::max(1u, h);
}
inline u32 GetRenderWidth()
{
    const u32 w = RenderResW();
    return w ? w : std::max(1u, (u32)Device.dwWidth);
}
inline u32 GetRenderHeight()
{
    const u32 h = RenderResH();
    return h ? h : std::max(1u, (u32)Device.dwHeight);
}

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
    u32 hudLit;
    u32 pad[3];
};
static_assert(sizeof(SkinnedMaterialCB) == 32, "SkinnedMaterialCB must be 32 bytes");

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
    cb.m_VP = Device.mFullTransform;

    const float t = Device.fTimeGlobal;
    cb.timers.set(t, t * 10.f, t / 10.f, _sin(t));

    if (g_pGamePersistent)
    {
        const auto& env = g_pGamePersistent->Environment().CurrentEnv;
        const float n = env.fog_near;
        const float f = std::max(env.fog_far, n + 1.0f);
        const float r = 1.0f / (f - n);
        if (ps_r_vol_fog)
            cb.fog_params.set(0.0f, 0.0f, 0.0f, 0.0f);
        else
            cb.fog_params.set(-n * r, r, r, r);
        cb.fog_color.set(env.fog_color.x, env.fog_color.y, env.fog_color.z,
            (ps_r_atmosphere != 0) ? std::clamp(ps_r_atmosphere_strength, 0.f, 4.f) : 0.0f);

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

    cb.L_ambient.set(0.2f, 0.2f, 0.2f, 1.0f);
    cb.L_sun_color.set(1.0f, 0.95f, 0.9f);
    cb.pbr_diffuse_mode = (float)ps_fg_pbr_diffuse_mode;
    cb.L_sun_dir_w.set(0.577f, -0.577f, 0.577f);
    cb.L_hemi_color.set(0.3f, 0.4f, 0.5f, ps_r2_sun_lumscale_hemi);

    cb.eye_position = Device.vCameraPosition;

    const u32 rw = GetRenderWidth();
    const u32 rh = GetRenderHeight();
    const float VertTan = -1.0f * tanf(deg2rad(Device.fFOV / 2.0f));
    const float HorzTan = -VertTan / Device.fASPECT;

    cb.pos_decompression_params.set(HorzTan, VertTan, (2.0f * HorzTan) / (float)rw, (2.0f * VertTan) / (float)rh);
    cb.pos_decompression_params2.set((float)rw, (float)rh, 1.0f / (float)rw, 1.0f / (float)rh);

    const float ph = ps_r2_df_parallax_h;
    cb.parallax.set(ph, -0.5f * ph, 0.0f, 0.0f);
    if (ps_r2_ls_flags.test(1u << 22))
        cb.parallax.z = 1.0f;
    if (((ps_r_rt_gi != 0) && g_restirReplaceForward) || (ps_r_path_tracer != 0))
        cb.parallax.w = -1.0f;

    cb.screen_res.set(
        (float)Device.dwWidth,
        (float)Device.dwHeight,
        1.0f / (float)Device.dwWidth,
        1.0f / (float)Device.dwHeight
    );

    cb.hud_fov = psHUD_FOV;
    cb.padding3 = 1.0f;
    if (GEnv.Backend && GEnv.Backend->IsHdr10())
        cb.padding3 = ps_r_hdr10_hud / std::max(ps_r_hdr10_paper_white, 1.f);

    cb.m_InvVP.invert(cb.m_VP);

    for (int i = 0; i < 4; i++)
        cb.shadow_matrices[i].identity();
    cb.cascade_splits.set(10.0f, 50.0f, 150.0f, 500.0f);

    cb.cluster_params.set(16.0f, 16.0f, 24.0f, 0.0f);
    cb.cluster_scales.set(0.1f, 500.0f, 1.0f, 1.0f);

    cb.camera_direction.set(Device.vCameraDirection.x, Device.vCameraDirection.y,
                            Device.vCameraDirection.z,
                            ps_r2_ls_flags.test(1u << 20) ? 1.0f : 0.0f);

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
    if (!g_pGamePersistent)
    {
        cb.L_sun_color.set(0.f, 0.f, 0.f);
        cb.L_sun_dir_w.set(0.f, -1.f, 0.f);
        cb.L_ambient.set(0.2f, 0.2f, 0.2f, 1.0f);
        cb.L_hemi_color.set(0.3f, 0.4f, 0.5f, 1.0f);
        return;
    }

    const auto& desc = g_pGamePersistent->Environment().CurrentEnv;

    float ambScale = 1.0f;
    const bool rtAmbientCut =
        (((ps_r_rt_gi != 0) && g_restirReplaceForward) || (ps_r_path_tracer != 0));
    if (rtAmbientCut)
        ambScale = std::clamp(ps_r_rt_gi_ambient_scale, 0.0f, 1.0f);

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

    const float minamb = 0.001f;
    cb.L_ambient.set(
        std::max(desc.ambient.x * 2.f, minamb) * ps_r2_sun_lumscale_amb * ambScale,
        std::max(desc.ambient.y * 2.f, minamb) * ps_r2_sun_lumscale_amb * ambScale,
        std::max(desc.ambient.z * 2.f, minamb) * ps_r2_sun_lumscale_amb * ambScale,
        desc.weight
    );

    const float hemiScale = 2.f * ps_r2_sun_lumscale_hemi * ambScale;
    cb.L_hemi_color.set(
        desc.env_color.x * hemiScale,
        desc.env_color.y * hemiScale,
        desc.env_color.z * hemiScale,
        1.0f
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
