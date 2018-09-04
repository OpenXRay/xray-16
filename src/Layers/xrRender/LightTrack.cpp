// LightTrack.cpp: implementation of the CROS_impl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LightTrack.h"
#include "Include/xrRender/RenderVisual.h"
#include "xrEngine/xr_object.h"

#ifdef _EDITOR
#include "IGame_Persistent.h"
#include "Environment.h"
#else
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CROS_impl::CROS_impl()
{
    approximate.set(0, 0, 0);
    dwFrame = u32(-1);
    shadow_recv_frame = u32(-1);
    shadow_recv_slot = -1;

    result_count = 0;
    result_iterator = 0;
    result_frame = u32(-1);
    result_sun = 0;
    hemi_value = 0.5f;
    hemi_smooth = 0.5f;
    sun_value = 0.2f;
    sun_smooth = 0.2f;

#if RENDER != R_R1
    last_position.set(0.0f, 0.0f, 0.0f);
    ticks_to_update = 0;
    sky_rays_uptodate = 0;
#endif // RENDER!=R_R1

    //#if RENDER==R_R1
    MODE = IRender_ObjectSpecific::TRACE_ALL;
    //#else
    //	MODE				= IRender_ObjectSpecific::TRACE_HEMI + IRender_ObjectSpecific::TRACE_SUN	;
    //#endif
}

void CROS_impl::add(light* source)
{
    // Search
    for (xr_vector<Item>::iterator I = track.begin(); I != track.end(); I++)
        if (source == I->source)
        {
            I->frame_touched = Device.dwFrame;
            return;
        }

    // Register _new_
    track.push_back(Item());
    Item& L = track.back();
    L.frame_touched = Device.dwFrame;
    L.source = source;
    L.cache.verts[0].set(0, 0, 0);
    L.cache.verts[1].set(0, 0, 0);
    L.cache.verts[2].set(0, 0, 0);
    L.test = 0.f;
    L.energy = 0.f;
}

IC bool pred_energy(const CROS_impl::Light& L1, const CROS_impl::Light& L2) { return L1.energy > L2.energy; }
//////////////////////////////////////////////////////////////////////////
#pragma warning(push)
#pragma warning(disable : 4305)

// const float		hdir		[lt_hemisamples][3] =
// {
// 	{0.00000,	1.00000,	0.00000	},
// 	{0.52573,	0.85065,	0.00000	},
// 	{0.16246,	0.85065,	0.50000	},
// 	{-0.42533,	0.85065,	0.30902	},
// 	{-0.42533,	0.85065,	-0.30902},
// 	{0.16246,	0.85065,	-0.50000},
// 	{0.89443,	0.44721,	0.00000	},
// 	{0.27639,	0.44721,	0.85065	},
// 	{-0.72361,	0.44721,	0.52573	},
// 	{-0.72361,	0.44721,	-0.52573},
// 	{0.27639,	0.44721,	-0.85065},
// 	{0.68819,	0.52573,	0.50000	},
// 	{-0.26287,	0.52573,	0.80902	},
// 	{-0.85065,	0.52573,	-0.00000},
// 	{-0.26287,	0.52573,	-0.80902},
// 	{0.68819,	0.52573,	-0.50000},
// 	{0.95106,	0.00000,	0.30902	},
// 	{0.58779,	0.00000,	0.80902	},
// 	{-0.00000,	0.00000,	1.00000	},
// 	{-0.58779,	0.00000,	0.80902	},
// 	{-0.95106,	0.00000,	0.30902	},
// 	{-0.95106,	0.00000,	-0.30902},
// 	{-0.58779,	0.00000,	-0.80902},
// 	{0.00000,	0.00000,	-1.00000},
// 	{0.58779,	0.00000,	-0.80902},
// 	{0.95106,	0.00000,	-0.30902}
// };

const float hdir[lt_hemisamples][3] = {
    {-0.26287, 0.52573, 0.80902}, {0.27639, 0.44721, 0.85065}, {-0.95106, 0.00000, 0.30902},
    {-0.95106, 0.00000, -0.30902}, {0.58779, 0.00000, -0.80902}, {0.58779, 0.00000, 0.80902},

    {-0.00000, 0.00000, 1.00000}, {0.52573, 0.85065, 0.00000}, {-0.26287, 0.52573, -0.80902},
    {-0.42533, 0.85065, 0.30902}, {0.95106, 0.00000, 0.30902}, {0.95106, 0.00000, -0.30902},

    {0.00000, 1.00000, 0.00000}, {-0.58779, 0.00000, 0.80902}, {-0.72361, 0.44721, 0.52573},
    {-0.72361, 0.44721, -0.52573}, {-0.58779, 0.00000, -0.80902}, {0.16246, 0.85065, -0.50000},

    {0.89443, 0.44721, 0.00000}, {-0.85065, 0.52573, -0.00000}, {0.16246, 0.85065, 0.50000},
    {0.68819, 0.52573, -0.50000}, {0.27639, 0.44721, -0.85065}, {0.00000, 0.00000, -1.00000},

    {-0.42533, 0.85065, -0.30902}, {0.68819, 0.52573, 0.50000},
};
#pragma warning(pop)

// inline CROS_impl::CubeFaces CROS_impl::get_cube_face(Fvector3& dir)
//{
//	float x2 = dir.x*dir.x;
//	float y2 = dir.y*dir.y;
//	float z2 = dir.z*dir.z;
//
//	if (x2 >= y2 + z2)
//	{
//		return (dir.x > 0) ? CUBE_FACE_POS_X : CUBE_FACE_NEG_X;
//	}
//	else if (y2 >= z2 + x2)
//	{
//		return (dir.y > 0) ? CUBE_FACE_POS_Y : CUBE_FACE_NEG_Y;
//	}
//	/*else*/
//	return (dir.z > 0) ? CUBE_FACE_POS_Z : CUBE_FACE_NEG_Z;
//}

inline void CROS_impl::accum_hemi(float* hemi_cube, Fvector3& dir, float scale)
{
    if (dir.x > 0)
        hemi_cube[CUBE_FACE_POS_X] += dir.x * scale;
    else
        hemi_cube[CUBE_FACE_NEG_X] -= dir.x * scale; //	dir.x <= 0

    if (dir.y > 0)
        hemi_cube[CUBE_FACE_POS_Y] += dir.y * scale;
    else
        hemi_cube[CUBE_FACE_NEG_Y] -= dir.y * scale; //	dir.y <= 0

    if (dir.z > 0)
        hemi_cube[CUBE_FACE_POS_Z] += dir.z * scale;
    else
        hemi_cube[CUBE_FACE_NEG_Z] -= dir.z * scale; //	dir.z <= 0
}

//////////////////////////////////////////////////////////////////////////
void CROS_impl::update(IRenderable* O)
{
    // clip & verify
    if (dwFrame == Device.dwFrame)
        return;
    dwFrame = Device.dwFrame;
    if (nullptr == O)
        return;
    if (nullptr == O->GetRenderData().visual)
        return;
    VERIFY(dynamic_cast<CROS_impl*>(O->renderable_ROS()));
    // float	dt			=	Device.fTimeDelta;

    IGameObject* _object = dynamic_cast<IGameObject*>(O);

    // select sample, randomize position inside object
    vis_data& vis = O->GetRenderData().visual->getVisData();
    Fvector position;
    O->GetRenderData().xform.transform_tiny(position, vis.sphere.P);
    position.y += .3f * vis.sphere.R;
    Fvector direction;
    direction.random_dir();
    //.			position.mad(direction,0.25f*radius);
    //.			position.mad(direction,0.025f*radius);

    // function call order is important at least for r1
    for (size_t i = 0; i < NUM_FACES; ++i)
    {
        hemi_cube[i] = 0;
    }

    bool bFirstTime = (0 == result_count);
    calc_sun_value(position, _object);
    calc_sky_hemi_value(position, _object);

    prepare_lights(position, O);

    // Process ambient lighting and approximate average lighting
    // Process our lights to find average luminescences
    CEnvDescriptor& desc = *g_pGamePersistent->Environment().CurrentEnv;
    Fvector accum = {desc.ambient.x, desc.ambient.y, desc.ambient.z};
    Fvector hemi = {desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z};
    Fvector sun_ = {desc.sun_color.x, desc.sun_color.y, desc.sun_color.z};
    if (MODE & IRender_ObjectSpecific::TRACE_HEMI)
        hemi.mul(hemi_smooth);
    else
        hemi.mul(.2f);
    accum.add(hemi);
    if (MODE & IRender_ObjectSpecific::TRACE_SUN)
        sun_.mul(sun_smooth);
    else
        sun_.mul(.2f);
    accum.add(sun_);
    if (MODE & IRender_ObjectSpecific::TRACE_LIGHTS)
    {
        Fvector lacc = {0, 0, 0};
#if RENDER != R_R1
        float hemi_cube_light[NUM_FACES] = {0, 0, 0, 0, 0, 0};
#endif
        for (u32 lit = 0; lit < lights.size(); lit++)
        {
            light* L = lights[lit].source;
            float d = L->position.distance_to(position);

#if RENDER != R_R1
            float a = (1 / (L->attenuation0 + L->attenuation1 * d + L->attenuation2 * d * d) - d * L->falloff) *
                (L->flags.bStatic ? 1.f : 2.f);
            a = (a > 0) ? a : 0.0f;

            Fvector3 dir;
            dir.sub(L->position, position);
            dir.normalize_safe();

            // multiply intensity on attenuation and accumulate result in hemi cube face
            float koef =
                (lights[lit].color.r + lights[lit].color.g + lights[lit].color.b) / 3.0f * a * ps_r2_dhemi_light_scale;

            accum_hemi(hemi_cube_light, dir, koef);
#else
            float r = L->range;
            float a = clampr(1.f - d / (r + EPS), 0.f, 1.f) * (L->flags.bStatic ? 1.f : 2.f);
#endif
            lacc.x += lights[lit].color.r * a;
            lacc.y += lights[lit].color.g * a;
            lacc.z += lights[lit].color.b * a;
        }
#if RENDER != R_R1
        const float minHemiValue = 1 / 255.f;

        float hemi_light = (lacc.x + lacc.y + lacc.z) / 3.0f * ps_r2_dhemi_light_scale;

        hemi_value += hemi_light;
        hemi_value = std::max(hemi_value, minHemiValue);

        for (size_t i = 0; i < NUM_FACES; ++i)
        {
            hemi_cube[i] += hemi_cube_light[i] * (1 - ps_r2_dhemi_light_flow) +
                ps_r2_dhemi_light_flow * hemi_cube_light[(i + NUM_FACES / 2) % NUM_FACES];
            hemi_cube[i] = std::max(hemi_cube[i], minHemiValue);
        }
#endif

        //		lacc.x		*= desc.lmap_color.x;
        //		lacc.y		*= desc.lmap_color.y;
        //		lacc.z		*= desc.lmap_color.z;
        //		Msg				("- rgb[%f,%f,%f]",lacc.x,lacc.y,lacc.z);
        accum.add(lacc);
    }
    else
        accum.set(.1f, .1f, .1f);

    // clamp(hemi_value, 0.0f, 1.0f); //Possibly can change hemi value
    if (bFirstTime)
    {
        hemi_smooth = hemi_value;
        CopyMemory(hemi_cube_smooth, hemi_cube, NUM_FACES * sizeof(float));
    }

    update_smooth();
    approximate = accum;
}

#if RENDER != R_R1

//	Update ticks settings
static const s32 s_iUTFirstTimeMin = 1;
static const s32 s_iUTFirstTimeMax = 1;
static const s32 s_iUTPosChangedMin = 3;
static const s32 s_iUTPosChangedMax = 6;
static const s32 s_iUTIdleMin = 1000;
static const s32 s_iUTIdleMax = 2000;

void CROS_impl::smart_update(IRenderable* O)
{
    if (!O)
        return;
    if (0 == O->GetRenderData().visual)
        return;

    --ticks_to_update;

    //	Acquire current position
    Fvector position;
    VERIFY(dynamic_cast<CROS_impl*>(O->renderable_ROS()));
    vis_data& vis = O->GetRenderData().visual->getVisData();
    O->GetRenderData().xform.transform_tiny(position, vis.sphere.P);

    if (ticks_to_update <= 0)
    {
        update(O);
        last_position = position;

        if (result_count < lt_hemisamples)
            ticks_to_update = ::Random.randI(s_iUTFirstTimeMin, s_iUTFirstTimeMax + 1);
        else if (sky_rays_uptodate < lt_hemisamples)
            ticks_to_update = ::Random.randI(s_iUTPosChangedMin, s_iUTPosChangedMax + 1);
        else
            ticks_to_update = ::Random.randI(s_iUTIdleMin, s_iUTIdleMax + 1);
    }
    else
    {
        if (!last_position.similar(position, 0.15))
        {
            sky_rays_uptodate = 0;
            update(O);
            last_position = position;

            if (result_count < lt_hemisamples)
                ticks_to_update = ::Random.randI(s_iUTFirstTimeMin, s_iUTFirstTimeMax + 1);
            else
                ticks_to_update = ::Random.randI(s_iUTPosChangedMin, s_iUTPosChangedMax + 1);
        }
    }
}

#endif //	#if RENDER!=R_R1

extern float ps_r2_lt_smooth;

// hemi & sun: update and smooth
void CROS_impl::update_smooth(IRenderable* O)
{
    if (dwFrameSmooth == Device.dwFrame)
        return;

    dwFrameSmooth = Device.dwFrame;

#if RENDER == R_R1
    if (O && (0 == result_count))
        update(O); // First time only
#else //	RENDER!=R_R1
    smart_update(O);
#endif //	RENDER!=R_R1

    float l_f = Device.fTimeDelta * ps_r2_lt_smooth;
    clamp(l_f, 0.f, 1.f);
    float l_i = 1.f - l_f;
    hemi_smooth = hemi_value * l_f + hemi_smooth * l_i;
    sun_smooth = sun_value * l_f + sun_smooth * l_i;
    for (size_t i = 0; i < NUM_FACES; ++i)
    {
        hemi_cube_smooth[i] = hemi_cube[i] * l_f + hemi_cube_smooth[i] * l_i;
    }
}

void CROS_impl::calc_sun_value(Fvector& position, IGameObject* _object)
{
#if RENDER == R_R1
    light* sun = (light*)RImplementation.L_DB->sun._get();
#else
    light* sun = (light*)RImplementation.Lights.sun._get();
#endif
    if (MODE & IRender_ObjectSpecific::TRACE_SUN)
    {
        if (--result_sun < 0)
        {
            result_sun += ::Random.randI(lt_hemisamples / 4, lt_hemisamples / 2);
            Fvector direction;
            direction.set(sun->direction).invert().normalize();
            sun_value = !(g_pGameLevel->ObjectSpace.RayTest(
                            position, direction, 500.f, collide::rqtBoth, &cache_sun, _object)) ?
                1.f :
                0.f;
        }
    }
}

void CROS_impl::calc_sky_hemi_value(Fvector& position, IGameObject* _object)
{
    // hemi-tracing
    if (MODE & IRender_ObjectSpecific::TRACE_HEMI)
    {
#if RENDER != R_R1
        sky_rays_uptodate += ps_r2_dhemi_count;
        sky_rays_uptodate = _min(sky_rays_uptodate, lt_hemisamples);
#endif //	RENDER!=R_R1

        for (u32 it = 0; it < (u32)ps_r2_dhemi_count; it++)
        { // five samples per one frame
            u32 sample = 0;
            if (result_count < lt_hemisamples)
            {
                sample = result_count;
                result_count++;
            }
            else
            {
                sample = (result_iterator % lt_hemisamples);
                result_iterator++;
            }

            // take sample
            Fvector direction;
            direction.set(hdir[sample][0], hdir[sample][1], hdir[sample][2]).normalize();
            //.			result[sample]	=
            //! g_pGameLevel->ObjectSpace.RayTest(position,direction,50.f,collide::rqtBoth,&cache[sample],_object);
            result[sample] = !g_pGameLevel->ObjectSpace.RayTest(
                position, direction, 50.f, collide::rqtStatic, &cache[sample], _object);
            //	Msg				("%d:-- %s",sample,result[sample]?"true":"false");
        }
    }
    // hemi & sun: update and smooth
    //	float	l_f				=	dt*lt_smooth;
    //	float	l_i				=	1.f-l_f;
    int _pass = 0;
    for (int it = 0; it < result_count; it++)
        if (result[it])
            _pass++;
    hemi_value = float(_pass) / float(result_count ? result_count : 1);
    hemi_value *= ps_r2_dhemi_sky_scale;

    for (int it = 0; it < result_count; it++)
    {
        if (result[it])
        {
            accum_hemi(hemi_cube, Fvector3().set(hdir[it][0], hdir[it][1], hdir[it][2]), ps_r2_dhemi_sky_scale);
        }
    }
}

void CROS_impl::prepare_lights(Fvector& position, IRenderable* O)
{
    IGameObject* _object = dynamic_cast<IGameObject*>(O);
    float dt = Device.fTimeDelta;

    vis_data& vis = O->GetRenderData().visual->getVisData();
    float radius;
    radius = vis.sphere.R;
    // light-tracing
    BOOL bTraceLights = MODE & IRender_ObjectSpecific::TRACE_LIGHTS;
    if ((!O->renderable_ShadowGenerate()) && (!O->renderable_ShadowReceive()))
        bTraceLights = FALSE;
    if (bTraceLights)
    {
        // Select nearest lights
        Fvector bb_size = {radius, radius, radius};

#if RENDER != R_R1
        g_SpatialSpace->q_box(RImplementation.lstSpatial, 0, STYPE_LIGHTSOURCEHEMI, position, bb_size);
#else
        g_SpatialSpace->q_box(RImplementation.lstSpatial, 0, STYPE_LIGHTSOURCE, position, bb_size);
#endif
        for (u32 o_it = 0; o_it < RImplementation.lstSpatial.size(); o_it++)
        {
            ISpatial* spatial = RImplementation.lstSpatial[o_it];
            light* source = (light*)(spatial->dcast_Light());
            VERIFY(source); // sanity check
            float R = radius + source->range;
            if (position.distance_to(source->position) < R
#if RENDER != R_R1
                && source->flags.bStatic
#endif
                )
                add(source);
        }

        // Trace visibility
        lights.clear();
#if RENDER == R_R1
        float traceR = radius * .5f;
#endif
        for (s32 id = 0; id < s32(track.size()); id++)
        {
            // remove untouched lights
            xr_vector<CROS_impl::Item>::iterator I = track.begin() + id;
            if (I->frame_touched != Device.dwFrame)
            {
                track.erase(I);
                id--;
                continue;
            }

            // Trace visibility
            Fvector P, D;
            float amount = 0;
            light* xrL = I->source;
            Fvector& LP = xrL->position;
#if RENDER == R_R1
            P.mad(position, P.random_dir(), traceR); // Random point inside range
#else
            P = position;
#endif

            // point/spot
            float f = D.sub(P, LP).magnitude();
            if (g_pGameLevel->ObjectSpace.RayTest(LP, D.div(f), f, collide::rqtStatic, &I->cache, _object))
                amount -= lt_dec;
            else
                amount += lt_inc;
            I->test += amount * dt;
            clamp(I->test, -.5f, 1.f);
            I->energy = .9f * I->energy + .1f * I->test;

            //
            float E = I->energy * xrL->color.intensity();
            if (E > EPS)
            {
                // Select light
                lights.push_back(CROS_impl::Light());
                CROS_impl::Light& L = lights.back();
                L.source = xrL;
                L.color.mul_rgb(xrL->color, I->energy / 2);
                L.energy = I->energy / 2;
                if (!xrL->flags.bStatic)
                {
                    L.color.mul_rgb(.5f);
                    L.energy *= .5f;
                }
            }
        }

#if RENDER == R_R1
        light* sun = (light*)RImplementation.L_DB->sun._get();

        // Sun
        float E = sun_smooth * sun->color.intensity();
        if (E > EPS)
        {
            // Select light
            lights.push_back(CROS_impl::Light());
            CROS_impl::Light& L = lights.back();
            L.source = sun;
            L.color.mul_rgb(sun->color, sun_smooth / 2);
            L.energy = sun_smooth;
        }
#endif
        // Sort lights by importance - important for R1-shadows
        std::sort(lights.begin(), lights.end(), pred_energy);
    }
}
