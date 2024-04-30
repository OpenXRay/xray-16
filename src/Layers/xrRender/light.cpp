#include "stdafx.h"
#include "light.h"

static constexpr float RSQRTDIV2 = 0.70710678118654752440084436210485f;

light::light() : SpatialBase(g_pGamePersistent->SpatialSpace)
{
    spatial.type = STYPE_LIGHTSOURCE;
    flags.type = POINT;
    flags.bStatic = false;
    flags.bActive = false;
    flags.bShadow = false;
    flags.bVolumetric = false;
    flags.bHudMode = false;
    position.set(0, -1000, 0);
    direction.set(0, -1, 0);
    right.set(0, 0, 0);
    range = 8.f;
    virtual_size = 0.1f;
    cone = deg2rad(60.f);
    color.set(1, 1, 1, 1);

    m_volumetric_quality = 1;
    // m_volumetric_quality = 0.5;
    m_volumetric_intensity = 1;
    m_volumetric_distance = 1;

    frame_render = 0;

}

light::~light()
{
    set_active(false);
}

void light::set_texture(LPCSTR name) {}

void light::set_active(bool a)
{
    if (a)
    {
        if (flags.bActive)
            return;
        flags.bActive = true;
        spatial_register();
        spatial_move();
//Msg("!!! L-register: %X", u32(this));

#ifdef DEBUG
        const Fvector zero = {0, -1000, 0};
        if (position.similar(zero))
        {
            Msg("- Uninitialized light position.");
        }
#endif // DEBUG
    }
    else
    {
        if (!flags.bActive)
            return;
        flags.bActive = false;
        spatial_move();
        spatial_unregister();
        //Msg("!!! L-unregister: %X", u32(this));
    }
}

void light::set_position(const Fvector& P)
{
    const float eps = EPS_L; //_max(range*0.001f,EPS_L);
    if (position.similar(P, eps))
        return;
    position.set(P);
    spatial_move();
}

void light::set_range(float R)
{
    const float eps = std::max(range * 0.1f, EPS_L);
    if (fsimilar(range, R, eps))
        return;
    range = R;
    spatial_move();
};

void light::set_cone(float angle)
{
    if (fsimilar(cone, angle))
        return;
    VERIFY(cone < deg2rad(121.f)); // 120 is hard limit for lights
    cone = angle;
    spatial_move();
}
void light::set_rotation(const Fvector& D, const Fvector& R)
{
    const Fvector old_D = direction;
    direction.normalize(D);
    right.normalize(R);
    if (!fsimilar(1.f, old_D.dotproduct(D)))
        spatial_move();
}

void light::spatial_move()
{
    switch (flags.type)
    {
    case IRender_Light::REFLECTED:
    case IRender_Light::POINT: { spatial.sphere.set(position, range);
    }
    break;
    case IRender_Light::SPOT:
    {
        // minimal enclosing sphere around cone
        VERIFY2(cone < deg2rad(121.f), "Too large light-cone angle. Maybe you have passed it in 'degrees'?");
        if (cone >= PI_DIV_2)
        {
            // obtused-angled
            spatial.sphere.P.mad(position, direction, range);
            spatial.sphere.R = range * tanf(cone / 2.f);
        }
        else
        {
            // acute-angled
            spatial.sphere.R = range / (2.f * _sqr(_cos(cone / 2.f)));
            spatial.sphere.P.mad(position, direction, spatial.sphere.R);
        }
    }
    break;
    case IRender_Light::OMNIPART:
    {
        // is it optimal? seems to be...
        //spatial.sphere.P.mad(position, direction, range);
        //spatial.sphere.R = range;
        // This is optimal.
        const float fSphereR = range * RSQRTDIV2;
        spatial.sphere.P.mad(position, direction, fSphereR);
        spatial.sphere.R = fSphereR;
    }
    break;
    }

    // update spatial DB
    SpatialBase::spatial_move();
}

vis_data& light::get_homdata()
{
    // commit vis-data
    hom.sphere.set(spatial.sphere.P, spatial.sphere.R);
    hom.box.set(spatial.sphere.P, spatial.sphere.P);
    hom.box.grow(spatial.sphere.R);
    return hom;
};

Fvector light::spatial_sector_point() { return position; }
//////////////////////////////////////////////////////////////////////////

extern float r_ssaGLOD_start, r_ssaGLOD_end;
extern float ps_r2_slight_fade;
float light::get_LOD() const
{
    if (!flags.bShadow)
        return 1;
    const float distSQ = Device.vCameraPosition.distance_to_sqr(spatial.sphere.P) + EPS;
    const float ssa = ps_r2_slight_fade * spatial.sphere.R / distSQ;
    const float lod = _sqrt(clampr((ssa - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
    return lod;
}
