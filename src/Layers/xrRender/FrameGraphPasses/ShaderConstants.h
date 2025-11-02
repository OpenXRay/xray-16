#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_matrix.h"

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
    float padding2[16];
};

struct alignas(16) GlobalConstants {
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

    cb.pos_decompression_params.set(1.0f, 1.0f, 1.0f, 1.0f);
    cb.pos_decompression_params2.set(0.0f, 0.0f, 0.0f, 0.0f);

    cb.parallax.set(0.02f, -0.01f, 0.0f, 0.0f);

    cb.padding1 = 0.0f;
    cb.padding2 = 0.0f;
    cb.padding3 = 0.0f;
}

}
