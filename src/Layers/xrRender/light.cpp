#include "stdafx.h"
#include <atomic>
#include "light.h"
#include "xrRender_console.h"

namespace xray::render::fg
{
static constexpr float RSQRTDIV2 = 0.70710678118654752440084436210485f;

light::light() : SpatialBase(g_pGamePersistent->SpatialSpace)
{
    m_lightID = AllocateLightID();
    spatial.type = STYPE_LIGHTSOURCE;
    flags.type = POINT;
    flags.bStatic = false;
    flags.bActive = false;
    flags.bShadow = false;
    flags.bVolumetric = false;
    flags.bHudMode = false;
    hud_spotlight = false;
    position.set(0, -1000, 0);
    direction.set(0, -1, 0);
    right.set(0, 0, 0);
    range = 8.f;
    virtual_size = 0.1f;
    cone = deg2rad(60.f);
    area_length = 0.f;
    area_radius = 0.05f;
    area_shadow_sample = false;
    ZeroMemory(area_shadow_slots, sizeof(area_shadow_slots));
    ZeroMemory(area_shadow_samples, sizeof(area_shadow_samples));
    color.set(1, 1, 1, 1);

    m_volumetric_quality = 1;
    // m_volumetric_quality = 0.5;
    m_volumetric_intensity = 1;
    m_volumetric_distance = 1;

    frame_render = 0;

#if (RENDER == R_R2) || (RENDER == R_R3) || (RENDER == R_R4) || (RENDER == R_GL)
    ZeroMemory(omnipart, sizeof(omnipart));
    vis.frame2test = 0; // xffffffff;
    vis.query_id = 0;
    vis.query_order = 0;
    vis.visible = true;
    vis.pending = false;
#endif // (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4) || (RENDER==R_GL)
}

light::~light()
{
#if (RENDER == R_R2) || (RENDER == R_R3) || (RENDER == R_R4) || (RENDER == R_GL)
    for (auto& f : omnipart)
        xr_delete(f);
#endif
    for (auto& f : area_shadow_samples)
        xr_delete(f);
    set_active(false);
}

u64 light::AllocateLightID()
{
    static std::atomic<u64> next{ 1 };
    return next.fetch_add(1, std::memory_order_relaxed);
}

#if (RENDER == R_R2) || (RENDER == R_R3) || (RENDER == R_R4) || (RENDER == R_GL)
void light::set_texture(LPCSTR name)
{
    spot_texture_name = (name && name[0]) ? name : nullptr;
}
#endif

#if RENDER == R_R1
void light::set_texture(LPCSTR name) {}
#endif

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
    if (position.x == P.x && position.y == P.y && position.z == P.z)
        return;
    position.set(P);
    spatial_move();
}

void light::set_range(float R)
{
    if (range == R)
        return;
    range = R;
    spatial_move();
};

void light::set_cone(float angle)
{
    VERIFY(angle < deg2rad(121.f));
    if (cone == angle)
        return;
    cone = angle;
    spatial_move();
}
void light::set_type(LT type)
{
    if (flags.type == type)
        return;
    flags.type = type;
    if (type == AREA)
        orthonormalize_area_basis(direction, right);
    spatial_move();
}

void light::set_area(float length, float radius)
{
    const float newLength = _valid(length) ? std::max(length, 0.f) : 0.f;
    const float newRadius = _valid(radius) ? std::max(radius, kCapsuleLightMinRadius) : kCapsuleLightMinRadius;
    if (area_length == newLength && area_radius == newRadius)
        return;
    area_length = newLength;
    area_radius = newRadius;
    if (flags.type == AREA)
        spatial_move();
}

void light::orthonormalize_area_basis(const Fvector& D, const Fvector& R)
{
    Fvector d;
    const float dm = D.magnitude();
    if (_valid(dm) && dm > EPS_S)
        d.set(D).div(dm);
    else
        d.set(0.f, 0.f, 1.f);

    Fvector r;
    r.mad(R, d, -R.dotproduct(d));
    const float rm = r.magnitude();
    if (_valid(rm) && rm > EPS_S)
        r.div(rm);
    else
    {
        Fvector axis;
        axis.set(0.f, 1.f, 0.f);
        if (_abs(axis.dotproduct(d)) > .99f)
            axis.set(0.f, 0.f, 1.f);
        r.crossproduct(axis, d);
        r.normalize();
    }

    Fvector u;
    u.crossproduct(d, r);
    u.normalize();
    r.crossproduct(u, d);
    r.normalize();

    direction.set(d);
    right.set(r);
}

void light::set_rotation(const Fvector& D, const Fvector& R)
{
    const Fvector old_D = direction;
    if (flags.type == AREA)
    {
        orthonormalize_area_basis(D, R);
        if (direction.x != old_D.x || direction.y != old_D.y || direction.z != old_D.z)
            spatial_move();
        return;
    }
    direction.normalize(D);
    right.normalize(R);
    if (direction.x != old_D.x || direction.y != old_D.y || direction.z != old_D.z)
        spatial_move();
}

void light::area_surface_sample(float u, float v, Fvector& P, Fvector& N) const
{
    Fvector up;
    up.crossproduct(direction, right);

    const float r = area_radius;
    const float h = u * (area_length + 2.f * r) - r;
    float a = 0.f;
    if (h < 0.f)
        a = h / r;
    else if (h > area_length)
        a = (h - area_length) / r;
    const float radial = _sqrt(std::max(0.f, 1.f - a * a));
    const float phi = PI_MUL_2 * v;
    const float c = _cos(phi) * radial;
    const float s = _sin(phi) * radial;

    N.set(0.f, 0.f, 0.f);
    N.mad(direction, a);
    N.mad(right, c);
    N.mad(up, s);

    P.mad(position, direction, clampr(h, 0.f, area_length));
    P.mad(N, r);
}

void light::UpdateAreaShadowSamples() const
{
    for (u32 i = 0; i < kCapsuleLightSamples; ++i)
    {
        light*& proxy = area_shadow_samples[i];
        if (!proxy)
        {
            proxy = xr_new<light>();
            proxy->flags.type = POINT;
            proxy->area_shadow_sample = true;
        }

        const float u = (float(i) + 0.5f) / float(kCapsuleLightSamples);
        const float t = float(i) * 0.61803398875f;
        const float v = t - std::floor(t);

        Fvector P, N;
        area_surface_sample(u, v, P, N);

        proxy->flags.bStatic = flags.bStatic;
        proxy->flags.bShadow = flags.bShadow;
        proxy->flags.bHudMode = flags.bHudMode;
        proxy->position = P;
        proxy->direction = direction;
        proxy->right = right;
        proxy->range = range;
        proxy->virtual_size = kCapsuleShadowNearClip;
        proxy->cone = cone;
        proxy->color = color;
        proxy->spatial.sector_id = spatial.sector_id;
        proxy->spatial.sphere.set(P, range);
    }
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
    case IRender_Light::AREA:
    {
        spatial.sphere.P.mad(position, direction, area_length * 0.5f);
        spatial.sphere.R = range + area_length * 0.5f + area_radius;
    }
    break;
    }

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

float light::get_LOD() const
{
    if (!flags.bShadow)
        return 1.0f;
    const float screen = float(Device.dwWidth) * float(Device.dwHeight)
        * _sqr(90.0f / Device.fFOV) * (EPS_S + ps_r__LOD);
    const float start = _sqr(ps_r__GLOD_ssa_start / 3.0f) / screen;
    const float end = _sqr(ps_r__GLOD_ssa_end / 3.0f) / screen;
    const float distSq = Device.vCameraPosition.distance_to_sqr(spatial.sphere.P) + EPS;
    const float ssa = ps_r2_slight_fade * spatial.sphere.R / distSq;
    return _sqrt(clampr((ssa - end) / (start - end), 0.0f, 1.0f));
}
//////////////////////////////////////////////////////////////////////////
#if (RENDER == R_R2) || (RENDER == R_R3) || (RENDER == R_R4) || (RENDER == R_GL)
// Xforms
void light::xform_calc()
{
    ZoneScoped;
    if (Device.dwFrame == m_xform_frame)
        return;
    m_xform_frame = Device.dwFrame;

    // build final rotation / translation
    Fvector L_dir, L_up, L_right;

    // dir
    L_dir.set(direction);
    float l_dir_m = L_dir.magnitude();
    if (_valid(l_dir_m) && l_dir_m > EPS_S)
        L_dir.div(l_dir_m);
    else
        L_dir.set(0, 0, 1);

    // R&N
    if (right.square_magnitude() > EPS)
    {
        // use specified 'up' and 'right', just enshure ortho-normalization
        L_right.set(right);
        L_right.normalize();
        L_up.crossproduct(L_dir, L_right);
        L_up.normalize();
        L_right.crossproduct(L_up, L_dir);
        L_right.normalize();
    }
    else
    {
        // auto find 'up' and 'right' vectors
        L_up.set(0, 1, 0);
        if (_abs(L_up.dotproduct(L_dir)) > .99f)
            L_up.set(0, 0, 1);
        L_right.crossproduct(L_up, L_dir);
        L_right.normalize();
        L_up.crossproduct(L_dir, L_right);
        L_up.normalize();
    }

    // matrix
    Fmatrix mR;
    mR.i = L_right;
    mR._14 = 0;
    mR.j = L_up;
    mR._24 = 0;
    mR.k = L_dir;
    mR._34 = 0;
    mR.c = position;
    mR._44 = 1;

    // switch
    switch (flags.type)
    {
    case IRender_Light::REFLECTED:
    case IRender_Light::POINT:
    {
        // scale of identity sphere
        float L_R = range;
        Fmatrix mScale;
        mScale.scale(L_R, L_R, L_R);
        m_xform.mul_43(mR, mScale);
    }
    break;
    case IRender_Light::SPOT:
    {
        // scale to account range and angle
        float s = 2.f * range * tanf(cone / 2.f);
        Fmatrix mScale;
        mScale.scale(s, s, range); // make range and radius
        m_xform.mul_43(mR, mScale);
    }
    break;
    case IRender_Light::OMNIPART:
    {
        float L_R = 2 * range; // volume is half-radius
        Fmatrix mScale;
        mScale.scale(L_R, L_R, L_R);
        m_xform.mul_43(mR, mScale);
    }
    break;
    default: m_xform.identity(); break;
    }
}

//                           +X,                -X,                +Y,                 -Y,                +Z,                -Z
Fvector cmNorm[6] = { { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, -1.f }, { 0.f, 0.f, 1.f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f } };
Fvector cmDir [6] = { { 1.f, 0.f, 0.f }, {-1.f, 0.f, 0.f }, { 0.f, 1.f,  0.f }, { 0.f,-1.f, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 0.f,-1.f } };

void light::Export(light_Package& package)
{
    if (flags.bShadow)
    {
        switch (flags.type)
        {
        case IRender_Light::POINT:
        {
            // tough: create/update 6 shadowed lights
            if (nullptr == omnipart[0])
                for (auto& p_light : omnipart)
                    p_light = xr_new<light>();
            for (int f = 0; f < 6; f++)
            {
                light* L = omnipart[f];
                Fvector R;
                R.crossproduct(cmNorm[f], cmDir[f]);
                L->set_type(IRender_Light::OMNIPART);
                L->set_shadow(true);
                L->set_position(position);
                L->set_rotation(cmDir[f], R);
                L->set_cone(PI_DIV_2);
                L->set_range(range);
                L->set_virtual_size(virtual_size);
                L->set_color(color);
                L->spatial.sector_id = spatial.sector_id; //. dangerous?

                //  Igor: add volumetric support
                L->set_volumetric(flags.bVolumetric);
                L->set_volumetric_quality(m_volumetric_quality);
                L->set_volumetric_intensity(m_volumetric_intensity);
                L->set_volumetric_distance(m_volumetric_distance);

                package.v_shadowed.push_back(L);
            }
        }
        break;
        case IRender_Light::SPOT: package.v_shadowed.push_back(this); break;
        }
    }
    else
    {
        switch (flags.type)
        {
        case IRender_Light::POINT: package.v_point.push_back(this); break;
        case IRender_Light::SPOT: package.v_spot.push_back(this); break;
        }
    }
}

void light::set_attenuation_params(float a0, float a1, float a2, float fo)
{
    attenuation0 = a0;
    attenuation1 = a1;
    attenuation2 = a2;
    falloff = fo;
}

#endif // (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4) || (RENDER==R_GL)

} // namespace xray::render::fg
