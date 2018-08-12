#include "stdafx.h"
#include "light_render_direct.h"

void CLight_Compute_XFORM_and_VIS::compute_xf_spot(light* L)
{
    // Build EYE-space xform
    Fvector L_dir, L_up, L_right, L_pos;
    L_dir.set(L->direction);
    L_dir.normalize();

    if (L->right.square_magnitude() > EPS)
    {
        // use specified 'up' and 'right', just enshure ortho-normalization
        L_right.set(L->right);
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
    L_pos.set(L->position);

    //
    int _cached_size = L->X.S.size;
    L->X.S.posX = L->X.S.posY = 0;
    L->X.S.size = SMAP_adapt_max;
    L->X.S.transluent = FALSE;

    // Compute approximate screen area (treating it as an point light) - R*R/dist_sq
    // Note: we clamp screen space area to ONE, although it is not correct at all
    float dist = Device.vCameraPosition.distance_to(L->spatial.sphere.P) - L->spatial.sphere.R;
    if (dist < 0)
        dist = 0;
    float ssa = clampr(L->range * L->range / (1.f + dist * dist), 0.f, 1.f);

    // compute intensity
    float intensity0 = (L->color.r + L->color.g + L->color.b) / 3.f;
    float intensity1 = (L->color.r * 0.2125f + L->color.g * 0.7154f + L->color.b * 0.0721f);
    float intensity = (intensity0 + intensity1) / 2.f; // intensity1 tends to underestimate...

    // compute how much duelling frusta occurs	[-1..1]-> 1 + [-0.5 .. +0.5]
    float duel_dot = 1.f - 0.5f * Device.vCameraDirection.dotproduct(L_dir);

    // compute how large the light is - give more texels to larger lights, assume 8m as being optimal radius
    float sizefactor = L->range / 8.f; // 4m = .5, 8m=1.f, 16m=2.f, 32m=4.f

    // compute how wide the light frustum is - assume 90deg as being optimal
    float widefactor = L->cone / deg2rad(90.f); //

    // factors
    float factor0 = powf(ssa, 1.f / 2.f); // ssa is quadratic
    float factor1 = powf(intensity, 1.f / 16.f); // less perceptually important?
    float factor2 = powf(duel_dot, 1.f / 4.f); // difficult to fast-change this -> visible
    float factor3 = powf(sizefactor, 1.f / 4.f); // this shouldn't make much difference
    float factor4 = powf(widefactor, 1.f / 2.f); // make it linear ???
    float factor = ps_r2_ls_squality * factor0 * factor1 * factor2 * factor3 * factor4;

    // final size calc
    u32 _size = iFloor(factor * SMAP_adapt_optimal);
    if (_size < SMAP_adapt_min)
        _size = SMAP_adapt_min;
    if (_size > SMAP_adapt_max)
        _size = SMAP_adapt_max;
    int _epsilon = iCeil(float(_size) * 0.01f);
    int _diff = _abs(int(_size) - int(_cached_size));
    L->X.S.size = (_diff >= _epsilon) ? _size : _cached_size;

    // make N pixel border
    L->X.S.view.build_camera_dir(L_pos, L_dir, L_up);
    // float	n			= 2.f						;
    // float	x			= float(L->X.S.size)		;
    // float	alpha		= L->cone/2					;
    // float	tan_beta	= (x+2*n)*tanf(alpha) / x	;
    // float	g_alpha		= 2*rad2deg		(alpha);
    // float	g_beta		= 2*rad2deg		(atanf(tan_beta));
    // Msg				("x(%f) : a(%f), b(%f)",x,g_alpha,g_beta);

    // _min(L->cone + deg2rad(4.5f), PI*0.98f) - Here, it is needed to enlarge the shadow map frustum to include also
    // displaced pixels and the pixels neighbor to the examining one.
    float tan_shift;
    if (L->flags.type == IRender_Light::POINT)
        tan_shift = deg2rad(11.5f);
    else
        tan_shift = deg2rad(3.5f);

    L->X.S.project.build_projection(L->cone + tan_shift, 1.f, L->virtual_size, L->range + EPS_S);
    L->X.S.combine.mul(L->X.S.project, L->X.S.view);
}
