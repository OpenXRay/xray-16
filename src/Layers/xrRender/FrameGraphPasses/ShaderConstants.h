#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_matrix.h"

namespace xray::render {
    namespace RENDER_NAMESPACE {
        extern float r__dtex_range;
    }
}

namespace xray::render::passes {

struct alignas(16) PerObjectConstants {
    Fmatrix m_xform;
    Fmatrix m_xform_v;
    Fvector4 consts;
    Fvector4 c_scale;
    Fvector4 c_bias;
    Fvector4 wind;
    Fvector4 wave;
    Fvector2 c_sun;
    float padding[2];
    float padding2[8];
};
static_assert(sizeof(PerObjectConstants) == 256, "PerObjectConstants must be 256 bytes");

struct alignas(16) DynamicTransforms {
    float m_WVP[16];
    float m_WV[12];
    float m_W[12];

    Fvector4 L_material;
    Fvector4 hemi_cube_pos_faces;
    Fvector4 hemi_cube_neg_faces;
    Fvector4 dt_params;
};

struct alignas(16) StaticGlobals {
    float m_V[12];

    float m_P[16];

    float m_VP[16];

    Fvector4 timers;

    Fvector4 fog_plane;
    Fvector4 fog_params;
    Fvector4 fog_color;

    Fvector4 L_ambient;
    Fvector3 L_sun_color;
    float padding1;
    Fvector3 L_sun_dir_w;
    float padding2;
    Fvector4 L_hemi_color;

    Fvector3 eye_position;
    float padding3;

    Fvector4 pos_decompression_params;
    Fvector4 pos_decompression_params2;

    Fvector4 parallax;
};

using GlobalConstants = StaticGlobals;

inline void FillGlobalConstants(GlobalConstants& cb) {

    Fmatrix viewT;
    viewT.transpose(Device.mView);
    cb.m_V[0]  = viewT._11; cb.m_V[1]  = viewT._12; cb.m_V[2]  = viewT._13; cb.m_V[3]  = viewT._14;
    cb.m_V[4]  = viewT._21; cb.m_V[5]  = viewT._22; cb.m_V[6]  = viewT._23; cb.m_V[7]  = viewT._24;
    cb.m_V[8]  = viewT._31; cb.m_V[9]  = viewT._32; cb.m_V[10] = viewT._33; cb.m_V[11] = viewT._34;

    Fmatrix projT;
    projT.transpose(Device.mProject);
    cb.m_P[0]  = projT._11; cb.m_P[1]  = projT._12; cb.m_P[2]  = projT._13; cb.m_P[3]  = projT._14;
    cb.m_P[4]  = projT._21; cb.m_P[5]  = projT._22; cb.m_P[6]  = projT._23; cb.m_P[7]  = projT._24;
    cb.m_P[8]  = projT._31; cb.m_P[9]  = projT._32; cb.m_P[10] = projT._33; cb.m_P[11] = projT._34;
    cb.m_P[12] = projT._41; cb.m_P[13] = projT._42; cb.m_P[14] = projT._43; cb.m_P[15] = projT._44;

    Fmatrix tempVP;
    tempVP.mul(Device.mProject, Device.mView);
    Fmatrix vpT;
    vpT.transpose(tempVP);
    cb.m_VP[0]  = vpT._11; cb.m_VP[1]  = vpT._12; cb.m_VP[2]  = vpT._13; cb.m_VP[3]  = vpT._14;
    cb.m_VP[4]  = vpT._21; cb.m_VP[5]  = vpT._22; cb.m_VP[6]  = vpT._23; cb.m_VP[7]  = vpT._24;
    cb.m_VP[8]  = vpT._31; cb.m_VP[9]  = vpT._32; cb.m_VP[10] = vpT._33; cb.m_VP[11] = vpT._34;
    cb.m_VP[12] = vpT._41; cb.m_VP[13] = vpT._42; cb.m_VP[14] = vpT._43; cb.m_VP[15] = vpT._44;

    cb.timers.set(
        Device.fTimeGlobal,
        Device.fTimeDelta,
        _sin(Device.fTimeGlobal),
        _cos(Device.fTimeGlobal)
    );

    cb.fog_plane.set(0.0f, 1.0f, 0.0f, 0.0f);
    cb.fog_params.set(0.0f, 1000.0f, 0.001f, 0.0f);
    cb.fog_color.set(0.5f, 0.5f, 0.6f, 1.0f);

    cb.L_ambient.set(0.2f, 0.2f, 0.2f, 1.0f);
    cb.L_sun_color.set(1.0f, 0.95f, 0.9f);
    cb.L_sun_dir_w.set(0.577f, -0.577f, 0.577f);
    cb.L_hemi_color.set(0.3f, 0.4f, 0.5f, 1.0f);

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

    cb.pos_decompression_params.set(HorzTan, VertTan, (2.0f * HorzTan) / (float)Device.dwWidth, (2.0f * VertTan) / (float)Device.dwHeight);
    cb.pos_decompression_params2.set((float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth, 1.0f / (float)Device.dwHeight);

    cb.parallax.set(0.02f, -0.01f, 0.0f, 0.0f);

    cb.padding1 = 0.0f;
    cb.padding2 = 0.0f;
    cb.padding3 = 0.0f;
}

inline void FillDynamicTransforms(DynamicTransforms& cb, Fmatrix m_W = Fidentity, float dtParamScale = 1.f) {
    Fmatrix wv, wvp;
    wv.mul_43(Device.mView, m_W);
    wvp.mul(Device.mProject, wv);

    Fmatrix wvpT, wvT, wT;
    wvpT.transpose(wvp);
    wvT.transpose(wv);

    wT.identity();
    wT.transpose(m_W);

    cb.m_WVP[0]  = wvpT._11; cb.m_WVP[1]  = wvpT._12; cb.m_WVP[2]  = wvpT._13; cb.m_WVP[3]  = wvpT._14;
    cb.m_WVP[4]  = wvpT._21; cb.m_WVP[5]  = wvpT._22; cb.m_WVP[6]  = wvpT._23; cb.m_WVP[7]  = wvpT._24;
    cb.m_WVP[8]  = wvpT._31; cb.m_WVP[9]  = wvpT._32; cb.m_WVP[10] = wvpT._33; cb.m_WVP[11] = wvpT._34;
    cb.m_WVP[12] = wvpT._41; cb.m_WVP[13] = wvpT._42; cb.m_WVP[14] = wvpT._43; cb.m_WVP[15] = wvpT._44;

    cb.m_WV[0]  = wvT._11; cb.m_WV[1]  = wvT._12; cb.m_WV[2]  = wvT._13; cb.m_WV[3]  = wvT._14;
    cb.m_WV[4]  = wvT._21; cb.m_WV[5]  = wvT._22; cb.m_WV[6]  = wvT._23; cb.m_WV[7]  = wvT._24;
    cb.m_WV[8]  = wvT._31; cb.m_WV[9]  = wvT._32; cb.m_WV[10] = wvT._33; cb.m_WV[11] = wvT._34;

    cb.m_W[0]  = wT._11; cb.m_W[1]  = wT._12; cb.m_W[2]  = wT._13; cb.m_W[3]  = wT._14;
    cb.m_W[4]  = wT._21; cb.m_W[5]  = wT._22; cb.m_W[6]  = wT._23; cb.m_W[7]  = wT._24;
    cb.m_W[8]  = wT._31; cb.m_W[9]  = wT._32; cb.m_W[10] = wT._33; cb.m_W[11] = wT._34;

    cb.L_material.set(0.01903f, 0.74998f, 0.0f, 0.25f);
    cb.hemi_cube_pos_faces.set(0.08034f, 0.42066f, 0.13277f, 0.0f);
    cb.hemi_cube_neg_faces.set(0.19919f, 0.00392f, 0.09922f, 0.0f);

    cb.dt_params.set(dtParamScale, dtParamScale, dtParamScale,
                     1.0f / xray::render::RENDER_NAMESPACE::r__dtex_range);
}

}
