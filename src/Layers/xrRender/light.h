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

#if (RENDER == R_R2) || (RENDER == R_R3) || (RENDER == R_R4) || (RENDER == R_GL)
    float falloff; // precalc to make light equal to zero at light range
    float attenuation0; // Constant attenuation
    float attenuation1; // Linear attenuation
    float attenuation2; // Quadratic attenuation

    light* omnipart[6];
    xr_vector<light_indirect> indirect;
    u32 indirect_photons;

    smapvis svis; // used for 6-cubemap faces

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
    } vis;

    union _xform
    {
        struct _D
        {
            Fmatrix combine;
            s32 minX, maxX;
            s32 minY, maxY;
            BOOL transluent;
        } D;
        struct _P
        {
            Fmatrix world;
            Fmatrix view;
            Fmatrix project;
            Fmatrix combine;
        } P;
        struct _S
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
    virtual void set_type(LT type) { flags.type = type; }
    virtual void set_active(bool b);
    virtual bool get_active() { return flags.bActive; }
    virtual void set_shadow(bool b) { flags.bShadow = b; }
    virtual void set_volumetric(bool b) { flags.bVolumetric = b; }
    virtual void set_volumetric_quality(float fValue) { m_volumetric_quality = fValue; }
    virtual void set_volumetric_intensity(float fValue) { m_volumetric_intensity = fValue; }
    virtual void set_volumetric_distance(float fValue) { m_volumetric_distance = fValue; }
    virtual void set_position(const Fvector& P);
    virtual void set_rotation(const Fvector& D, const Fvector& R);
    virtual void set_cone(float angle);
    virtual void set_range(float R);
    virtual void set_virtual_size(float R) { virtual_size = R; }
    virtual void set_color(const Fcolor& C) { color.set(C); }
    virtual void set_color(float r, float g, float b) { color.set(r, g, b, 1); }
    virtual void set_texture(LPCSTR name);
    virtual void set_hud_mode(bool b) { flags.bHudMode = b; }
    virtual bool get_hud_mode() { return flags.bHudMode; };
    virtual void spatial_move();
    virtual Fvector spatial_sector_point();

    virtual IRender_Light* dcast_Light() { return this; }
    vis_data& get_homdata();
#if (RENDER == R_R2) || (RENDER == R_R3) || (RENDER == R_R4) || (RENDER == R_GL)
    void gi_generate();
    void xform_calc();
    void vis_prepare();
    void vis_update();
    void Export(light_Package& dest);
    void set_attenuation_params(float a0, float a1, float a2, float fo);
#endif // (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4) || (RENDER==R_GL)

    float get_LOD();

    light();
    virtual ~light();
};

#endif // #define LAYERS_XRRENDER_LIGHT_H_INCLUDED
