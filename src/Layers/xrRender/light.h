#ifndef LAYERS_XRRENDER_LIGHT_H_INCLUDED
#define LAYERS_XRRENDER_LIGHT_H_INCLUDED

#include "xrCDB/ISpatial.h"

#if (RENDER == R_R2) || (RENDER == R_R3) || (RENDER == R_R4) || (RENDER==R_GL)
#include "Light_Package.h"
#include "light_smapvis.h"
#include "light_gi.h"
#endif //(RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4) || (RENDER==R_GL)

class light : public IRender_Light, public SpatialBase
{
public:
    struct
    {
        u32 type : 4;
        u32 bStatic : 1;
        u32 bActive : 1;
        u32 bShadow : 1;
        u32 bVolumetric : 1;
        u32 bHudMode : 1;

    } flags;
    Fvector position;
    Fvector direction;
    Fvector right;
    float range;
    float virtual_size;
    float cone;
    Fcolor color;

    vis_data hom;
    u32 frame_render;

    float m_volumetric_quality;
    float m_volumetric_intensity;
    float m_volumetric_distance;

#if RENDER == R_R1
    Flight ldata;
#else
    float falloff; // precalc to make light equal to zero at light range
    float attenuation0; // Constant attenuation
    float attenuation1; // Linear attenuation
    float attenuation2; // Quadratic attenuation

    light* omnipart[6];
    xr_vector<light_indirect> indirect;
    u32 indirect_photons;

    smapvis svis[R__NUM_CONTEXTS]; // used for 6-cubemap faces

    ref_shader s_spot;
    ref_shader s_point;
    ref_shader s_volumetric;

#if (RENDER == R_R3) || (RENDER == R_R4) || (RENDER == R_GL)
    ref_shader s_spot_msaa[8];
    ref_shader s_point_msaa[8];
    ref_shader s_volumetric_msaa[8];
#endif //	(RENDER==R_R3) || (RENDER==R_R4) || (RENDER==R_GL)

    u32 m_xform_frame;
    Fmatrix m_xform;

    struct _vis
    {
        u32 frame2test; // frame the test is sheduled to
        u32 query_id; // ID of occlusion query
        u32 query_order; // order of occlusion query
        bool visible; // visible/invisible
        bool pending; // test is still pending
        u16 smap_ID;
        float distance;
    } vis;

    union _xform
    {
        struct Directional
        {
            Fmatrix combine;
            s32 minX, maxX;
            s32 minY, maxY;
            BOOL transluent;
        } D[R__NUM_SUN_CASCADES];
        struct Point
        {
            Fmatrix world;
            Fmatrix view;
            Fmatrix project;
            Fmatrix combine;
        } P;
        struct Spot
        {
            Fmatrix view;
            Fmatrix project;
            Fmatrix combine;
            u32 size;
            u32 posX;
            u32 posY;
            BOOL transluent;
        } S;
    } X;
#endif //	(RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4) || (RENDER==R_GL)

public:
    void set_type(LT type) override
    {
        flags.type = type;
    }

    void set_active(bool b) override;

    [[nodiscard]]
    bool get_active() override { return flags.bActive; }

    void set_shadow(bool b) override { flags.bShadow = b; }

    void set_volumetric(bool b) override { flags.bVolumetric = b; }

    void set_volumetric_quality(float fValue) override { m_volumetric_quality = fValue; }

    void set_volumetric_intensity(float fValue) override { m_volumetric_intensity = fValue; }

    void set_volumetric_distance(float fValue) override { m_volumetric_distance = fValue; }

    void set_position(const Fvector& P) override;

    void set_rotation(const Fvector& D, const Fvector& R) override;

    void set_cone(float angle) override;

    void set_range(float R) override;

    void set_virtual_size(float R) override { virtual_size = R; }

    void set_color(const Fcolor& C) override
    {
        color.set(C);
    }

    void set_color(float r, float g, float b) override
    {
        color.set(r, g, b, 1);
    }

    void set_texture(LPCSTR name) override;

    void set_hud_mode(bool b) override { flags.bHudMode = b; }
    [[nodiscard]]
    bool get_hud_mode() override { return flags.bHudMode; }

    void spatial_move() override;
    Fvector spatial_sector_point() override;

    IRender_Light* dcast_Light() override { return this; }
    vis_data& get_homdata();

#if (RENDER == R_R2) || (RENDER == R_R3) || (RENDER == R_R4) || (RENDER == R_GL)
    void gi_generate();
    void xform_calc();
    void vis_prepare(CBackend& cmd_list);
    void vis_update();
    void Export(light_Package& dest);
    void set_attenuation_params(float a0, float a1, float a2, float fo);
#endif // (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4) || (RENDER==R_GL)

    [[nodiscard]]
    float get_LOD() const;

    light();
    ~light() override;
};

#endif // #define LAYERS_XRRENDER_LIGHT_H_INCLUDED
