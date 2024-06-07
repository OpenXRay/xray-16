#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "Blender_Recorder.h"
#include "Blender.h"

#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"

// matrices
#define BIND_DECLARE(xf)\
    class cl_xform_##xf : public R_constant_setup\
    {\
        void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.xforms.set_c_##xf(C); }\
    };\
    static cl_xform_##xf binder_##xf
BIND_DECLARE(w);
BIND_DECLARE(invw);
BIND_DECLARE(v);
BIND_DECLARE(p);
BIND_DECLARE(wv);
BIND_DECLARE(vp);
BIND_DECLARE(wvp);

#define DECLARE_TREE_BIND(c)\
    class cl_tree_##c : public R_constant_setup\
    {\
        void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.tree.set_c_##c(C); }\
    };\
    static cl_tree_##c tree_binder_##c

DECLARE_TREE_BIND(m_xform_v);
DECLARE_TREE_BIND(m_xform);
DECLARE_TREE_BIND(consts);
DECLARE_TREE_BIND(wave);
DECLARE_TREE_BIND(wind);
DECLARE_TREE_BIND(c_scale);
DECLARE_TREE_BIND(c_bias);
DECLARE_TREE_BIND(c_sun);

class cl_hemi_cube_pos_faces : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.hemi.set_c_pos_faces(C); }
};

static cl_hemi_cube_pos_faces binder_hemi_cube_pos_faces;

class cl_hemi_cube_neg_faces : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.hemi.set_c_neg_faces(C); }
};

static cl_hemi_cube_neg_faces binder_hemi_cube_neg_faces;

class cl_material : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.hemi.set_c_material(C); }
};

static cl_material binder_material;

class cl_texgen : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        Fmatrix mTexgen;

#if defined(USE_DX11)
        Fmatrix mTexelAdjust =
        {
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f
        };
#elif defined(USE_OGL)
        Fmatrix mTexelAdjust =
        {
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f
        };
#else
#    error No graphics API selected or in use!
#endif

        mTexgen.mul(mTexelAdjust, cmd_list.xforms.m_wvp);
        cmd_list.set_c(C, mTexgen);
    }
};
static cl_texgen binder_texgen;

class cl_VPtexgen : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        Fmatrix mTexgen;

#if defined(USE_DX11)
        Fmatrix mTexelAdjust =
        {
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f
        };
#elif defined(USE_OGL)
        Fmatrix mTexelAdjust =
        {
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f
        };
#else
#    error No graphics API selected or in use!
#endif

        mTexgen.mul(mTexelAdjust, cmd_list.xforms.m_vp);
        cmd_list.set_c(C, mTexgen);
    }
};
static cl_VPtexgen binder_VPtexgen;

// fog
#ifndef _EDITOR
class cl_fog_plane : public R_constant_setup
{
    u32 marker;
    Fvector4 result;
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        if (marker != Device.dwFrame)
        {
            // Plane
            Fvector4 plane;
            Fmatrix& M = Device.mFullTransform;
            plane.x = -(M._14 + M._13);
            plane.y = -(M._24 + M._23);
            plane.z = -(M._34 + M._33);
            plane.w = -(M._44 + M._43);
            float denom = -1.0f / _sqrt(_sqr(plane.x) + _sqr(plane.y) + _sqr(plane.z));
            plane.mul(denom);

            // Near/Far
            float A = g_pGamePersistent->Environment().CurrentEnv.fog_near;
            float B = 1 / (g_pGamePersistent->Environment().CurrentEnv.fog_far - A);
            result.set(-plane.x * B, -plane.y * B, -plane.z * B, 1 - (plane.w - A) * B); // view-plane
        }
        cmd_list.set_c(C, result);
    }
};
static cl_fog_plane binder_fog_plane;

// fog-params
class cl_fog_params : public R_constant_setup
{
    u32 marker;
    Fvector4 result;
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        if (marker != Device.dwFrame)
        {
            // Near/Far
            float n = g_pGamePersistent->Environment().CurrentEnv.fog_near;
            float f = g_pGamePersistent->Environment().CurrentEnv.fog_far;
            float r = 1 / (f - n);
            result.set(-n * r, n, f, r);
        }
        cmd_list.set_c(C, result);
    }
};
static cl_fog_params binder_fog_params;

// fog-color
class cl_fog_color : public R_constant_setup
{
    u32 marker;
    Fvector4 result;
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        if (marker != Device.dwFrame)
        {
            const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
            result.set(desc.fog_color.x, desc.fog_color.y, desc.fog_color.z, desc.fog_density);
        }
        cmd_list.set_c(C, result);
    }
};
static cl_fog_color binder_fog_color;
#endif

// times
class cl_times : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        float t = Device.fTimeGlobal;
        cmd_list.set_c(C, t, t * 10, t / 10, _sin(t));
    }
};
static cl_times binder_times;

// eye-params
class cl_eye_P : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        Fvector& V = Device.vCameraPosition;
        cmd_list.set_c(C, V.x, V.y, V.z, 1.f);
    }
};
static cl_eye_P binder_eye_P;

// eye-params
class cl_eye_D : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        Fvector& V = Device.vCameraDirection;
        cmd_list.set_c(C, V.x, V.y, V.z, 0.f);
    }
};
static cl_eye_D binder_eye_D;

// eye-params
class cl_eye_N : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        Fvector& V = Device.vCameraTop;
        cmd_list.set_c(C, V.x, V.y, V.z, 0.f);
    }
};
static cl_eye_N binder_eye_N;

#ifndef _EDITOR
// D-Light0
class cl_sun0_color : public R_constant_setup
{
    u32 marker;
    Fvector4 result;
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        if (marker != Device.dwFrame)
        {
            const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
            result.set(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z, 0);
        }
        cmd_list.set_c(C, result);
    }
};
static cl_sun0_color binder_sun0_color;
class cl_sun0_dir_w : public R_constant_setup
{
    u32 marker;
    Fvector4 result;
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        if (marker != Device.dwFrame)
        {
            const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
            result.set(desc.sun_dir.x, desc.sun_dir.y, desc.sun_dir.z, 0);
        }
        cmd_list.set_c(C, result);
    }
};
static cl_sun0_dir_w binder_sun0_dir_w;
class cl_sun0_dir_e : public R_constant_setup
{
    u32 marker;
    Fvector4 result;
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        if (marker != Device.dwFrame)
        {
            Fvector D;
            const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
            Device.mView.transform_dir(D, desc.sun_dir);
            D.normalize();
            result.set(D.x, D.y, D.z, 0);
        }
        cmd_list.set_c(C, result);
    }
};
static cl_sun0_dir_e binder_sun0_dir_e;

//
class cl_amb_color : public R_constant_setup
{
    u32 marker;
    Fvector4 result;
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        if (marker != Device.dwFrame)
        {
            const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
            result.set(desc.ambient.x, desc.ambient.y, desc.ambient.z, desc.weight);
        }
        cmd_list.set_c(C, result);
    }
};
static cl_amb_color binder_amb_color;
class cl_hemi_color : public R_constant_setup
{
    u32 marker;
    Fvector4 result;
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        if (marker != Device.dwFrame)
        {
            const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
            result.set(desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z, desc.hemi_color.w);
        }
        cmd_list.set_c(C, result);
    }
};
static cl_hemi_color binder_hemi_color;
#endif

static class cl_screen_res : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, (float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth,
            1.0f / (float)Device.dwHeight);
    }
} binder_screen_res;

// SM_TODO: cmd_list.hemi заменить на более "логичное" место
static class cl_hud_params : public R_constant_setup //--#SM+#--
{
    void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.set_c(C, g_pGamePersistent->m_pGShaderConstants->hud_params); }
} binder_hud_params;

static class cl_script_params : public R_constant_setup //--#SM+#--
{
    void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.set_c(C, g_pGamePersistent->m_pGShaderConstants->m_script_params); }
} binder_script_params;

static class cl_blend_mode : public R_constant_setup //--#SM+#--
{
    void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.set_c(C, g_pGamePersistent->m_pGShaderConstants->m_blender_mode); }
} binder_blend_mode;

class cl_camo_data : public R_constant_setup //--#SM+#--
{
    void setup(CBackend& cmd_list, R_constant* C) override  { cmd_list.hemi.c_camo_data = C; }
};
static cl_camo_data binder_camo_data;

class cl_custom_data : public R_constant_setup //--#SM+#--
{
    void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.hemi.c_custom_data = C; }
};
static cl_custom_data binder_custom_data;

class cl_entity_data : public R_constant_setup //--#SM+#--
{
    void setup(CBackend& cmd_list, R_constant* C) override { cmd_list.hemi.c_entity_data = C; }
};
static cl_entity_data binder_entity_data;

// Ascii1457's Screen Space Shaders
extern ENGINE_API Fvector4 ps_ssfx_hud_drops_1;
extern ENGINE_API Fvector4 ps_ssfx_hud_drops_2;
extern ENGINE_API Fvector4 ps_ssfx_blood_decals;
extern ENGINE_API Fvector4 ps_ssfx_wpn_dof_1;
extern ENGINE_API Fvector4 ps_ssfx_wpn_dof_2;
extern ENGINE_API Fvector4 ps_ssfx_florafixes_1;
extern ENGINE_API Fvector4 ps_ssfx_florafixes_2;
extern ENGINE_API float ps_ssfx_gloss_factor;
extern ENGINE_API Fvector3 ps_ssfx_gloss_minmax;
extern ENGINE_API Fvector4 ps_ssfx_wetsurfaces_1;
extern ENGINE_API Fvector4 ps_ssfx_wetsurfaces_2;
extern ENGINE_API int ps_ssfx_is_underground;
extern ENGINE_API Fvector4 ps_ssfx_lightsetup_1;

class cl_inv_v : public R_constant_setup
{
    Fmatrix result;
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        result.invert(Device.mView);
        cmd_list.set_c(C, result);
    }
};
static cl_inv_v binder_inv_v;

class cl_rain_params : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        float rainDensity = g_pGamePersistent->Environment().CurrentEnv.rain_density;
        float rainWetness = g_pGamePersistent->Environment().wetness_factor;
        cmd_list.set_c(C, rainDensity, rainWetness, 0.0f, 0.0f);
    }
};
static cl_rain_params binder_rain_params;

class pp_image_corrections : public R_constant_setup
{
    virtual void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_r2_img_exposure, ps_r2_img_gamma, ps_r2_img_saturation, 1.f);
    }
};
static pp_image_corrections binder_image_corrections;

class pp_color_grading : public R_constant_setup
{
    virtual void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_r2_img_cg.x, ps_r2_img_cg.y, ps_r2_img_cg.z, 1.f);
    }
};
static pp_color_grading binder_color_grading;

class cl_sky_color : public R_constant_setup
{
    u32 marker;
    Fvector4 result;

    void setup(CBackend& cmd_list, R_constant* C) override
    {
        if (marker != Device.dwFrame)
        {
            CEnvDescriptor& desc = g_pGamePersistent->Environment().CurrentEnv;
            result.set(desc.sky_color.x, desc.sky_color.y, desc.sky_color.z, desc.sky_rotation);
        }
        cmd_list.set_c(C, result);
    }
};
static cl_sky_color binder_sky_color;

//Sneaky debug stuff
extern ENGINE_API Fvector4 ps_dev_param_1;
extern ENGINE_API Fvector4 ps_dev_param_2;
extern ENGINE_API Fvector4 ps_dev_param_3;
extern ENGINE_API Fvector4 ps_dev_param_4;
extern ENGINE_API Fvector4 ps_dev_param_5;
extern ENGINE_API Fvector4 ps_dev_param_6;
extern ENGINE_API Fvector4 ps_dev_param_7;
extern ENGINE_API Fvector4 ps_dev_param_8;

static class dev_param_1 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_dev_param_1.x, ps_dev_param_1.y, ps_dev_param_1.z, ps_dev_param_1.w);
    }
} dev_param_1;

static class dev_param_2 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_dev_param_2.x, ps_dev_param_2.y, ps_dev_param_2.z, ps_dev_param_2.w);
    }
} dev_param_2;

static class dev_param_3 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_dev_param_3.x, ps_dev_param_3.y, ps_dev_param_3.z, ps_dev_param_3.w);
    }
} dev_param_3;

static class dev_param_4 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_dev_param_4.x, ps_dev_param_4.y, ps_dev_param_4.z, ps_dev_param_4.w);
    }
} dev_param_4;

static class dev_param_5 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_dev_param_5.x, ps_dev_param_5.y, ps_dev_param_5.z, ps_dev_param_5.w);
    }
} dev_param_5;

static class dev_param_6 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_dev_param_6.x, ps_dev_param_6.y, ps_dev_param_6.z, ps_dev_param_6.w);
    }
} dev_param_6;

static class dev_param_7 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_dev_param_7.x, ps_dev_param_7.y, ps_dev_param_7.z, ps_dev_param_7.w);
    }
} dev_param_7;

static class dev_param_8 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_dev_param_8.x, ps_dev_param_8.y, ps_dev_param_8.z, ps_dev_param_8.w);
    }
} dev_param_8;

class ssfx_wpn_dof_1 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_ssfx_wpn_dof_1.x, ps_ssfx_wpn_dof_1.y, ps_ssfx_wpn_dof_1.z, ps_ssfx_wpn_dof_1.w);
    }
};
static ssfx_wpn_dof_1 binder_ssfx_wpn_dof_1;

class ssfx_wpn_dof_2 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_ssfx_wpn_dof_2.x, ps_ssfx_wpn_dof_2.y, ps_ssfx_wpn_dof_2.z, ps_ssfx_wpn_dof_2.w);
    }
};
static ssfx_wpn_dof_2 binder_ssfx_wpn_dof_2;

class ssfx_blood_decals : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_ssfx_blood_decals);
    }
};
static ssfx_blood_decals binder_ssfx_blood_decals;

class ssfx_hud_drops_1 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_ssfx_hud_drops_1);
    }
};
static ssfx_hud_drops_1 binder_ssfx_hud_drops_1;

class ssfx_hud_drops_2 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
    {
        cmd_list.set_c(C, ps_ssfx_hud_drops_2);
    }
};
static ssfx_hud_drops_2 binder_ssfx_hud_drops_2;

class ssfx_lightsetup_1 : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override
	{
		cmd_list.set_c(C, ps_ssfx_lightsetup_1);
	}
};
static ssfx_lightsetup_1 binder_ssfx_lightsetup_1;

class ssfx_is_underground : public R_constant_setup
{
	void setup(CBackend& cmd_list, R_constant* C) override
	{
		cmd_list.set_c(C, (float)ps_ssfx_is_underground, 0.f, 0.f, 0.f);
	}
};
static ssfx_is_underground binder_ssfx_is_underground;

class ssfx_wetsurfaces_1 : public R_constant_setup
{
	void setup(CBackend& cmd_list, R_constant* C) override
	{
		cmd_list.set_c(C, ps_ssfx_wetsurfaces_1);
	}
};
static ssfx_wetsurfaces_1 binder_ssfx_wetsurfaces_1;

class ssfx_wetsurfaces_2 : public R_constant_setup
{
	void setup(CBackend& cmd_list, R_constant* C) override
	{
		cmd_list.set_c(C, ps_ssfx_wetsurfaces_2);
	}
};
static ssfx_wetsurfaces_2 binder_ssfx_wetsurfaces_2;

class ssfx_gloss : public R_constant_setup
{
	void setup(CBackend& cmd_list, R_constant* C) override
	{
		cmd_list.set_c(C, ps_ssfx_gloss_minmax.x, ps_ssfx_gloss_minmax.y, ps_ssfx_gloss_factor, 0.f);
	}
};
static ssfx_gloss binder_ssfx_gloss;

class ssfx_florafixes_1 : public R_constant_setup
{
	void setup(CBackend& cmd_list, R_constant* C) override
	{
		cmd_list.set_c(C, ps_ssfx_florafixes_1);
	}
};
static ssfx_florafixes_1 binder_ssfx_florafixes_1;

class ssfx_florafixes_2 : public R_constant_setup
{
	void setup(CBackend& cmd_list, R_constant* C) override
	{
		cmd_list.set_c(C, ps_ssfx_florafixes_2);
	}
};
static ssfx_florafixes_2 binder_ssfx_florafixes_2;

// Standart constant-binding
void CBlender_Compile::SetMapping()
{
    // misc
    r_Constant("m_hud_params", &binder_hud_params); //--#SM+#--
    r_Constant("m_script_params", &binder_script_params); //--#SM+#--
    r_Constant("m_blender_mode", &binder_blend_mode); //--#SM+#--

    // objects data
    r_Constant("m_obj_camo_data", &binder_camo_data); //--#SM+#--
    r_Constant("m_obj_custom_data", &binder_custom_data); //--#SM+#--
    r_Constant("m_obj_entity_data", &binder_entity_data); //--#SM+#--

    // matrices
    r_Constant("m_W", &binder_w);
    r_Constant("m_invW", &binder_invw);
    r_Constant("m_V", &binder_v);
    r_Constant("m_P", &binder_p);
    r_Constant("m_WV", &binder_wv);
    r_Constant("m_VP", &binder_vp);
    r_Constant("m_WVP", &binder_wvp);

    r_Constant("m_xform_v", &tree_binder_m_xform_v);
    r_Constant("m_xform", &tree_binder_m_xform);
    r_Constant("consts", &tree_binder_consts);
    r_Constant("wave", &tree_binder_wave);
    r_Constant("wind", &tree_binder_wind);
    r_Constant("c_scale", &tree_binder_c_scale);
    r_Constant("c_bias", &tree_binder_c_bias);
    r_Constant("c_sun", &tree_binder_c_sun);

    // hemi cube
    r_Constant("L_material", &binder_material);
    r_Constant("hemi_cube_pos_faces", &binder_hemi_cube_pos_faces);
    r_Constant("hemi_cube_neg_faces", &binder_hemi_cube_neg_faces);

    // Igor temp solution for the texgen functionality in the shader
    r_Constant("m_texgen", &binder_texgen);
    r_Constant("mVPTexgen", &binder_VPtexgen);

#ifndef _EDITOR
    // fog-params
    r_Constant("fog_plane", &binder_fog_plane);
    r_Constant("fog_params", &binder_fog_params);
    r_Constant("fog_color", &binder_fog_color);
#endif
    // time
    r_Constant("timers", &binder_times);

    // eye-params
    r_Constant("eye_position", &binder_eye_P);
    r_Constant("eye_direction", &binder_eye_D);
    r_Constant("eye_normal", &binder_eye_N);

#ifndef _EDITOR
    // global-lighting (env params)
    r_Constant("L_sun_color", &binder_sun0_color);
    r_Constant("L_sun_dir_w", &binder_sun0_dir_w);
    r_Constant("L_sun_dir_e", &binder_sun0_dir_e);
    //r_Constant("L_lmap_color", &binder_lm_color);
    r_Constant("L_hemi_color", &binder_hemi_color);
    r_Constant("L_ambient", &binder_amb_color);
#endif
    r_Constant("screen_res", &binder_screen_res);

    // detail
    // if (bDetail  && detail_scaler)
    // Igor: bDetail can be overridden by no_detail_texture option.
    // But shader can be deatiled implicitly, so try to set this parameter
    // anyway.
    if (detail_scaler)
        r_Constant("dt_params", detail_scaler);

    // other common
    for (u32 it = 0; it < RImplementation.Resources->v_constant_setup.size(); it++)
    {
        std::pair<shared_str, R_constant_setup*> cs = RImplementation.Resources->v_constant_setup[it];
        r_Constant(*cs.first, cs.second);
    }

    // Anomaly
    r_Constant("rain_params", &binder_rain_params);
    r_Constant("pp_img_corrections", &binder_image_corrections);
    r_Constant("pp_img_cg", &binder_color_grading);
    r_Constant("m_inv_V", &binder_inv_v);

    r_Constant("shader_param_1", &dev_param_1);
    r_Constant("shader_param_2", &dev_param_2);
    r_Constant("shader_param_3", &dev_param_3);
    r_Constant("shader_param_4", &dev_param_4);
    r_Constant("shader_param_5", &dev_param_5);
    r_Constant("shader_param_6", &dev_param_6);
    r_Constant("shader_param_7", &dev_param_7);
    r_Constant("shader_param_8", &dev_param_8);

    // Ascii1457's Screen Space Shaders
    r_Constant("sky_color", &binder_sky_color);
    r_Constant("ssfx_wpn_dof_1", &binder_ssfx_wpn_dof_1);
    r_Constant("ssfx_wpn_dof_2", &binder_ssfx_wpn_dof_2);
    r_Constant("ssfx_blood_decals", &binder_ssfx_blood_decals);
    r_Constant("ssfx_hud_drops_1", &binder_ssfx_hud_drops_1);
    r_Constant("ssfx_hud_drops_2", &binder_ssfx_hud_drops_2);
	r_Constant("ssfx_lightsetup_1", &binder_ssfx_lightsetup_1);
	r_Constant("ssfx_is_underground", &binder_ssfx_is_underground);
	r_Constant("ssfx_wetsurfaces_1", &binder_ssfx_wetsurfaces_1);
	r_Constant("ssfx_wetsurfaces_2", &binder_ssfx_wetsurfaces_2);
	r_Constant("ssfx_gloss", &binder_ssfx_gloss);
	r_Constant("ssfx_florafixes_1", &binder_ssfx_florafixes_1);
	r_Constant("ssfx_florafixes_2", &binder_ssfx_florafixes_2);
}
