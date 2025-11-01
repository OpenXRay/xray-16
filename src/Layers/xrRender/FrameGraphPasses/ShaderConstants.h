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
    float m_V[3][4];

    Fmatrix m_P;
    Fmatrix m_VP;

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
    const Fmatrix& view = Device.mView;
    cb.m_V[0][0] = view._11; cb.m_V[0][1] = view._12; cb.m_V[0][2] = view._13; cb.m_V[0][3] = view._14;
    cb.m_V[1][0] = view._21; cb.m_V[1][1] = view._22; cb.m_V[1][2] = view._23; cb.m_V[1][3] = view._24;
    cb.m_V[2][0] = view._31; cb.m_V[2][1] = view._32; cb.m_V[2][2] = view._33; cb.m_V[2][3] = view._34;

    cb.m_P = Device.mProject;

    cb.m_VP.mul(Device.mView, Device.mProject);

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
