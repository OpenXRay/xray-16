#include "stdafx.h"

#include "Rain.h"
#include "IGame_Persistent.h"
#include "Environment.h"

#include "perlin.h"

#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "Render.h"
#include "IGame_Level.h"
#include "xrCDB/xr_area.h"
#include "xr_object.h"
#endif

// Warning: duplicated in dxRainRender
//static const int max_desired_items = 2500;
//static const float source_radius = 12.5f;
static const float source_offset = 20.f; // 40
static const float max_distance = source_offset * 1.5f;//1.25f;
//static const float sink_offset = -(max_distance - source_offset);
//static const float drop_length = 5.f;
//static const float drop_width = 0.30f;
static const float drop_angle = deg2rad(15.0f); // 3.0
static const float drop_max_angle = deg2rad(35.f); // 10
static const float drop_max_wind_vel = 20.0f;
static const float drop_speed_min = 40.f;
static const float drop_speed_max = 80.f;

const int max_particles = 1000;
//const int particles_cache = 400;
const float particles_time = .3f;
CPerlinNoise1D* RainPerlin;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEffect_Rain::CEffect_Rain()
{
    state = stIdle;

    snd_Ambient.create("ambient" DELIMITER "rain", st_Effect, sg_Undefined);

    RainPerlin = xr_new<CPerlinNoise1D>(Random.randI(0, 0xFFFF));
    RainPerlin->SetOctaves(2);
    RainPerlin->SetAmplitude(0.66666f);

    p_create();
}

CEffect_Rain::~CEffect_Rain()
{
    xr_delete(RainPerlin);
    snd_Ambient.destroy();

    // Cleanup
    p_destroy();
}

void CEffect_Rain::Prepare(Fvector2& offset, Fvector3& axis, float W_Velocity, float W_Direction)
{
    // Wind gust, to add variation.
    float Wind_Gust = RainPerlin->GetContinious(Device.fTimeGlobal * 0.3f) * 2.0f;

    // Wind velocity [ 0 ~ 1 ]
    float Wind_Velocity = W_Velocity + Wind_Gust;

    clamp(Wind_Velocity, 0.0f, 1.0f);

    // Wind velocity controles the angle
    float pitch = drop_max_angle * Wind_Velocity;
    axis.setHP(W_Direction, pitch - PI_DIV_2);

    // Get distance
    float dist = _sin(pitch) * source_offset;
    float C = PI_DIV_2 - pitch;
    dist /= _sin(C);

    // 0 is North
    float fixNorth = W_Direction - PI_DIV_2;

    // Set offset
    offset.set(dist * _cos(fixNorth), dist * _sin(fixNorth));
}

// Born
void CEffect_Rain::Born(Item& dest, float radius, float speed)
{
    ZoneScoped;

    // Prepare correct angle and distance to hit the player
    Fvector Rain_Axis = { 0, -1, 0 };
    Fvector2 Rain_Offset;

    float Wind_Direction = -g_pGamePersistent->Environment().CurrentEnv.wind_direction;

    // Wind Velocity [ From 0 ~ 1000 to 0 ~ 1 ]
    float Wind_Velocity = g_pGamePersistent->Environment().CurrentEnv.wind_velocity * 0.001f;
    clamp(Wind_Velocity, 0.0f, 1.0f);

    Prepare(Rain_Offset, Rain_Axis, Wind_Velocity, Wind_Direction);

    // Camera Position
    Fvector& view = Device.vCameraPosition;
    // Random Position
    float r = radius * 0.5f;
    Fvector2 RandomP = { ::Random.randF(-r, r), ::Random.randF(-r, r) };

    // Aim ahead of where the player is facing
    Fvector FinalView = Fvector().mad(view, Device.vCameraDirection, 5.0f);

    // Random direction. Higher angle at lower velocity
    dest.D.random_dir(Rain_Axis, ::Random.randF(-drop_angle, drop_angle) * (1.5f - Wind_Velocity));

    // Set final destination
    dest.P.set(Rain_Offset.x + FinalView.x + RandomP.x, source_offset + view.y, Rain_Offset.y + FinalView.z + RandomP.y);

    // Set speed
    dest.fSpeed = ::Random.randF(drop_speed_min, drop_speed_max) * speed * clampr(Wind_Velocity * 1.5f, 0.5f, 1.0f);

    // Born
    float height = max_distance;
    RenewItem(dest, height, RayPick(dest.P, dest.D, height, collide::rqtBoth));
}

bool CEffect_Rain::RayPick(const Fvector& s, const Fvector& d, float& range, collide::rq_target tgt)
{
    ZoneScoped;

    bool bRes = true;
#ifdef _EDITOR
    Tools->RayPick(s, d, range);
#else
    collide::rq_result RQ;
    IGameObject* E = g_pGameLevel->CurrentViewEntity();
    bRes = g_pGameLevel->ObjectSpace.RayPick(s, d, range, tgt, RQ, E);
    if (bRes)
        range = RQ.range;
#endif
    return bRes;
}

void CEffect_Rain::RenewItem(Item& dest, float height, bool bHit)
{
    dest.uv_set = Random.randI(2);
    if (bHit)
    {
        dest.dwTime_Life = Device.dwTimeGlobal + iFloor(1000.f * height / dest.fSpeed) - Device.dwTimeDelta;
        dest.dwTime_Hit = Device.dwTimeGlobal + iFloor(1000.f * height / dest.fSpeed) - Device.dwTimeDelta;
        dest.Phit.mad(dest.P, dest.D, height);
    }
    else
    {
        dest.dwTime_Life = Device.dwTimeGlobal + iFloor(1000.f * height / dest.fSpeed) - Device.dwTimeDelta;
        dest.dwTime_Hit = Device.dwTimeGlobal + iFloor(2 * 1000.f * height / dest.fSpeed) - Device.dwTimeDelta;
        dest.Phit.set(dest.P);
    }
}

void CEffect_Rain::OnFrame()
{
    ZoneScoped;

#ifndef _EDITOR
    if (!g_pGameLevel)
        return;
#endif

    if (GEnv.isDedicatedServer)
        return;

    // Parse states
    float rain_density = g_pGamePersistent->Environment().CurrentEnv.rain_density;
    float wind_velocity = g_pGamePersistent->Environment().CurrentEnv.wind_velocity * 0.001f;
    clamp(wind_velocity, 0.0f, 1.0f);

    wind_velocity *= (rain_density > 0.0f ? 1.0f : 0.0f); // Only when raining

    // 50% of the volume is by rain_density and 50% wind_velocity;
    float factor = rain_density * 0.5f + wind_velocity * 0.5f;
    static float hemi_factor = 0.f;
#ifndef _EDITOR
    IGameObject* E = g_pGameLevel->CurrentViewEntity();
    if (E && E->renderable_ROS())
    {
        // hemi_factor = 1.f-2.0f*(0.3f-_min(_min(1.f,E->renderable_ROS()->get_luminocity_hemi()),0.3f));
        float* hemi_cube = E->renderable_ROS()->get_luminocity_hemi_cube();
        float hemi_val = _max(hemi_cube[0], hemi_cube[1]);
        hemi_val = _max(hemi_val, hemi_cube[2]);
        hemi_val = _max(hemi_val, hemi_cube[3]);
        hemi_val = _max(hemi_val, hemi_cube[5]);

        // float f = 0.9f*hemi_factor + 0.1f*hemi_val;
        float f = hemi_val;
        float t = Device.fTimeDelta;
        clamp(t, 0.001f, 1.0f);
        hemi_factor = hemi_factor * (1.0f - t) + f * t;
        rain_hemi = hemi_val;
    }
#endif

    switch (state)
    {
    case stIdle:
        if (factor < EPS_L)
            return;
        state = stWorking;
        snd_Ambient.play(0, sm_Looped);
        snd_Ambient.set_position(Fvector().set(0, 0, 0));
        snd_Ambient.set_range(source_offset, source_offset * 2.f);
        break;
    case stWorking:
        if (factor < EPS_L)
        {
            state = stIdle;
            snd_Ambient.stop();
            return;
        }
        break;
    }

    // ambient sound
    if (snd_Ambient._feedback())
    {
        // Fvector sndP;
        // sndP.mad (Device.vCameraPosition,Fvector().set(0,1,0),source_offset);
        // snd_Ambient.set_position(sndP);
        snd_Ambient.set_volume(_max(0.1f, factor) * hemi_factor);
    }
}

void CEffect_Rain::Render()
{
#ifndef _EDITOR
    if (!g_pGameLevel)
        return;
#endif

    m_pRender->Render(*this);
}

// startup _new_ particle system
void CEffect_Rain::Hit(Fvector& pos)
{
    if (0 != ::Random.randI(2))
        return;
    Particle* P = p_allocate();
    if (0 == P)
        return;

    const Fsphere& bv_sphere = m_pRender->GetDropBounds();

    P->time = particles_time;
    P->mXForm.rotateY(::Random.randF(PI_MUL_2));
    P->mXForm.translate_over(pos);
    P->mXForm.transform_tiny(P->bounds.P, bv_sphere.P);
    P->bounds.R = bv_sphere.R;
}

// initialize particles pool
void CEffect_Rain::p_create()
{
    // pool
    particle_pool.resize(max_particles);
    for (size_t it = 0; it < particle_pool.size(); it++)
    {
        Particle& P = particle_pool[it];
        P.prev = it ? (&particle_pool[it - 1]) : 0;
        P.next = (it < (particle_pool.size() - 1)) ? (&particle_pool[it + 1]) : 0;
    }

    // active and idle lists
    particle_active = 0;
    particle_idle = &particle_pool.front();
}

// destroy particles pool
void CEffect_Rain::p_destroy()
{
    // active and idle lists
    particle_active = 0;
    particle_idle = 0;

    // pool
    particle_pool.clear();
}

// _delete_ node from _list_
void CEffect_Rain::p_remove(Particle* P, Particle*& LST)
{
    VERIFY(P);
    Particle* prev = P->prev;
    P->prev = NULL;
    Particle* next = P->next;
    P->next = NULL;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    if (LST == P)
        LST = next;
}

// insert node at the top of the head
void CEffect_Rain::p_insert(Particle* P, Particle*& LST)
{
    VERIFY(P);
    P->prev = 0;
    P->next = LST;
    if (LST)
        LST->prev = P;
    LST = P;
}

// determine size of _list_
int CEffect_Rain::p_size(Particle* P)
{
    if (0 == P)
        return 0;
    int cnt = 0;
    while (P)
    {
        P = P->next;
        cnt += 1;
    }
    return cnt;
}

// alloc node
CEffect_Rain::Particle* CEffect_Rain::p_allocate()
{
    Particle* P = particle_idle;
    if (0 == P)
        return NULL;
    p_remove(P, particle_idle);
    p_insert(P, particle_active);
    return P;
}

// xr_free node
void CEffect_Rain::p_free(Particle* P)
{
    p_remove(P, particle_active);
    p_insert(P, particle_idle);
}
