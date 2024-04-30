#ifndef LAYERS_XRRENDER_LIGHT_H_INCLUDED
#define LAYERS_XRRENDER_LIGHT_H_INCLUDED

#include "xrCDB/ISpatial.h"

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

    Flight ldata;

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

    [[nodiscard]]
    float get_LOD() const;

    light();
    ~light() override;
};

#endif // #define LAYERS_XRRENDER_LIGHT_H_INCLUDED
