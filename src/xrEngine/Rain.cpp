#include "stdafx.h"

#include "Rain.h"
#include "IGame_Persistent.h"
#include "Environment.h"

#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "Render.h"
#include "IGame_Level.h"
#include "xrCDB/xr_area.h"
#include "xr_object.h"
#endif

// Warning: duplicated in dxRainRender
static const int max_desired_items = 2500;
static const float source_radius = 12.5f;
static const float source_offset = 40.f;
static const float max_distance = source_offset * 1.25f;
static const float sink_offset = -(max_distance - source_offset);
static const float drop_length = 5.f;
static const float drop_width = 0.30f;
static const float drop_angle = 3.0f;
static const float drop_max_angle = deg2rad(10.f);
static const float drop_max_wind_vel = 20.0f;
static const float drop_speed_min = 40.f;
static const float drop_speed_max = 80.f;

const int max_particles = 1000;
const int particles_cache = 400;
const float particles_time = .3f;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEffect_Rain::CEffect_Rain()
{
    state = stIdle;

    snd_Ambient.create("ambient" DELIMITER "rain", st_Effect, sg_Undefined);

    // Moced to p_Render constructor
    /*
    IReader* F = FS.r_open("$game_meshes$","dm" DELIMITER "rain.dm");
    VERIFY3 (F,"Can't open file.","dm" DELIMITER "rain.dm");
    DM_Drop = GlobalEnv.Render->model_CreateDM (F);

    //
    SH_Rain.create ("effects" DELIMITER "rain","fx" DELIMITER "fx_rain");
    hGeom_Rain.create (FVF::F_LIT, RCache.Vertex.Buffer(), RCache.QuadIB);
    hGeom_Drops.create (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, RCache.Vertex.Buffer(), RCache.Index.Buffer());
    */
    p_create();
    // FS.r_close (F);
}

CEffect_Rain::~CEffect_Rain()
{
    snd_Ambient.destroy();

    // Cleanup
    p_destroy();
    // Moved to p_Render destructor
    // GlobalEnv.Render->model_Delete (DM_Drop);
}

// Born
void CEffect_Rain::Born(Item& dest, float radius)
{
    Fvector axis;
    axis.set(0, -1, 0);
    float gust = g_pGamePersistent->Environment().wind_strength_factor / 10.f;
    float k = g_pGamePersistent->Environment().CurrentEnv->wind_velocity * gust / drop_max_wind_vel;
    clamp(k, 0.f, 1.f);
    float pitch = drop_max_angle * k - PI_DIV_2;
    axis.setHP(g_pGamePersistent->Environment().CurrentEnv->wind_direction, pitch);

    Fvector& view = Device.vCameraPosition;
    float angle = ::Random.randF(0, PI_MUL_2);
    float dist = ::Random.randF();
    dist = _sqrt(dist) * radius;
    float x = dist * _cos(angle);
    float z = dist * _sin(angle);
    dest.D.random_dir(axis, deg2rad(drop_angle));
    dest.P.set(x + view.x - dest.D.x * source_offset, source_offset + view.y, z + view.z - dest.D.z * source_offset);
    // dest.P.set (x+view.x,height+view.y,z+view.z);
    dest.fSpeed = ::Random.randF(drop_speed_min, drop_speed_max);

    float height = max_distance;
    RenewItem(dest, height, RayPick(dest.P, dest.D, height, collide::rqtBoth));
}

BOOL CEffect_Rain::RayPick(const Fvector& s, const Fvector& d, float& range, collide::rq_target tgt)
{
    BOOL bRes = TRUE;
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

void CEffect_Rain::RenewItem(Item& dest, float height, BOOL bHit)
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
#ifndef _EDITOR
    if (!g_pGameLevel)
        return;
#endif

    if (GEnv.isDedicatedServer)
        return;

    // Parse states
    float factor = g_pGamePersistent->Environment().CurrentEnv->rain_density;
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

    /*
    float factor = g_pGamePersistent->Environment().CurrentEnv->rain_density;
    if (factor<EPS_L) return;

    u32 desired_items = iFloor (0.5f*(1.f+factor)*float(max_desired_items));
    // visual
    float factor_visual = factor/2.f+.5f;
    Fvector3 f_rain_color = g_pGamePersistent->Environment().CurrentEnv->rain_color;
    u32 u_rain_color = color_rgba_f(f_rain_color.x,f_rain_color.y,f_rain_color.z,factor_visual);

    // born _new_ if needed
    float b_radius_wrap_sqr = _sqr((source_radius+.5f));
    if (items.size()<desired_items) {
    // items.reserve (desired_items);
    while (items.size()<desired_items) {
    Item one;
    Born (one,source_radius);
    items.push_back (one);
    }
    }

    // build source plane
    Fplane src_plane;
    Fvector norm ={0.f,-1.f,0.f};
    Fvector upper; upper.set(Device.vCameraPosition.x,Device.vCameraPosition.y+source_offset,Device.vCameraPosition.z);
    src_plane.build(upper,norm);

    // perform update
    u32 vOffset;
    FVF::LIT *verts = (FVF::LIT *) RCache.Vertex.Lock(desired_items*4,hGeom_Rain->vb_stride,vOffset);
    FVF::LIT *start = verts;
    const Fvector& vEye = Device.vCameraPosition;
    for (u32 I=0; I<items.size(); I++){
    // physics and time control
    Item& one = items[I];

    if (one.dwTime_Hit<Device.dwTimeGlobal) Hit (one.Phit);
    if (one.dwTime_Life<Device.dwTimeGlobal) Born(one,source_radius);

    // последняя дельта ??
    //. float xdt = float(one.dwTime_Hit-Device.dwTimeGlobal)/1000.f;
    //. float dt = Device.fTimeDelta;//xdt<Device.fTimeDelta?xdt:Device.fTimeDelta;
    float dt = Device.fTimeDelta;
    one.P.mad (one.D,one.fSpeed*dt);

    Device.Statistic->TEST1.Begin();
    Fvector wdir; wdir.set(one.P.x-vEye.x,0,one.P.z-vEye.z);
    float wlen = wdir.square_magnitude();
    if (wlen>b_radius_wrap_sqr) {
    wlen = _sqrt(wlen);
    //. Device.Statistic->TEST3.Begin();
    if ((one.P.y-vEye.y)<sink_offset){
    // need born
    one.invalidate();
    }else{
    Fvector inv_dir, src_p;
    inv_dir.invert(one.D);
    wdir.div (wlen);
    one.P.mad (one.P, wdir, -(wlen+source_radius));
    if (src_plane.intersectRayPoint(one.P,inv_dir,src_p)){
    float dist_sqr = one.P.distance_to_sqr(src_p);
    float height = max_distance;
    if (RayPick(src_p,one.D,height,collide::rqtBoth)){
    if (_sqr(height)<=dist_sqr){
    one.invalidate (); // need born
    // Log("1");
    }else{
    RenewItem (one,height-_sqrt(dist_sqr),TRUE); // fly to point
    // Log("2",height-dist);
    }
    }else{
    RenewItem (one,max_distance-_sqrt(dist_sqr),FALSE); // fly ...
    // Log("3",1.5f*b_height-dist);
    }
    }else{
    // need born
    one.invalidate();
    // Log("4");
    }
    }
    //. Device.Statistic->TEST3.End();
    }
    Device.Statistic->TEST1.End();

    // Build line
    Fvector& pos_head = one.P;
    Fvector pos_trail; pos_trail.mad (pos_head,one.D,-drop_length*factor_visual);

    // Culling
    Fvector sC,lineD; float sR;
    sC.sub (pos_head,pos_trail);
    lineD.normalize (sC);
    sC.mul (.5f);
    sR = sC.magnitude();
    sC.add (pos_trail);
    if (!GlobalEnv.Render->ViewBase.testSphere_dirty(sC,sR)) continue;

    static Fvector2 UV[2][4]={
    {{0,1},{0,0},{1,1},{1,0}},
    {{1,0},{1,1},{0,0},{0,1}}
    };

    // Everything OK - build vertices
    Fvector P,lineTop,camDir;
    camDir.sub (sC,vEye);
    camDir.normalize ();
    lineTop.crossproduct(camDir,lineD);
    float w = drop_width;
    u32 s = one.uv_set;
    P.mad(pos_trail,lineTop,-w); verts->set(P,u_rain_color,UV[s][0].x,UV[s][0].y); verts++;
    P.mad(pos_trail,lineTop,w); verts->set(P,u_rain_color,UV[s][1].x,UV[s][1].y); verts++;
    P.mad(pos_head, lineTop,-w); verts->set(P,u_rain_color,UV[s][2].x,UV[s][2].y); verts++;
    P.mad(pos_head, lineTop,w); verts->set(P,u_rain_color,UV[s][3].x,UV[s][3].y); verts++;
    }
    u32 vCount = (u32)(verts-start);
    RCache.Vertex.Unlock (vCount,hGeom_Rain->vb_stride);

    // Render if needed
    if (vCount) {
    HW.pDevice->SetRenderState (D3DRS_CULLMODE,D3DCULL_NONE);
    RCache.set_xform_world (Fidentity);
    RCache.set_Shader (SH_Rain);
    RCache.set_Geometry (hGeom_Rain);
    RCache.Render (D3DPT_TRIANGLELIST,vOffset,0,vCount,0,vCount/2);
    HW.pDevice->SetRenderState (D3DRS_CULLMODE,D3DCULL_CCW);
    }

    // Particles
    Particle* P = particle_active;
    if (0==P) return;

    {
    float dt = Device.fTimeDelta;
    _IndexStream& _IS = RCache.Index;
    RCache.set_Shader (DM_Drop->shader);

    Fmatrix mXform,mScale;
    int pcount = 0;
    u32 v_offset,i_offset;
    u32 vCount_Lock = particles_cache*DM_Drop->number_vertices;
    u32 iCount_Lock = particles_cache*DM_Drop->number_indices;
    IRender_DetailModel::fvfVertexOut* v_ptr= (IRender_DetailModel::fvfVertexOut*) RCache.Vertex.Lock (vCount_Lock,
    hGeom_Drops->vb_stride, v_offset);
    u16* i_ptr = _IS.Lock (iCount_Lock, i_offset);
    while (P) {
    Particle* next = P->next;

    // Update
    // P can be zero sometimes and it crashes
    P->time -= dt;
    if (P->time<0) {
    p_free (P);
    P = next;
    continue;
    }

    // Render
    if (GlobalEnv.Render->ViewBase.testSphere_dirty(P->bounds.P, P->bounds.R))
    {
    // Build matrix
    float scale = P->time / particles_time;
    mScale.scale (scale,scale,scale);
    mXform.mul_43 (P->mXForm,mScale);

    // XForm verts
    DM_Drop->transfer (mXform,v_ptr,u_rain_color,i_ptr,pcount*DM_Drop->number_vertices);
    v_ptr += DM_Drop->number_vertices;
    i_ptr += DM_Drop->number_indices;
    pcount ++;

    if (pcount >= particles_cache) {
    // flush
    u32 dwNumPrimitives = iCount_Lock/3;
    RCache.Vertex.Unlock (vCount_Lock,hGeom_Drops->vb_stride);
    _IS.Unlock (iCount_Lock);
    RCache.set_Geometry (hGeom_Drops);
    RCache.Render (D3DPT_TRIANGLELIST,v_offset, 0,vCount_Lock,i_offset,dwNumPrimitives);

    v_ptr = (IRender_DetailModel::fvfVertexOut*) RCache.Vertex.Lock (vCount_Lock, hGeom_Drops->vb_stride, v_offset);
    i_ptr = _IS.Lock (iCount_Lock, i_offset);

    pcount = 0;
    }
    }

    P = next;
    }

    // Flush if needed
    vCount_Lock = pcount*DM_Drop->number_vertices;
    iCount_Lock = pcount*DM_Drop->number_indices;
    u32 dwNumPrimitives = iCount_Lock/3;
    RCache.Vertex.Unlock (vCount_Lock,hGeom_Drops->vb_stride);
    _IS.Unlock (iCount_Lock);
    if (pcount) {
    RCache.set_Geometry (hGeom_Drops);
    RCache.Render (D3DPT_TRIANGLELIST,v_offset,0,vCount_Lock,i_offset,dwNumPrimitives);
    }
    }
    */
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
    for (u32 it = 0; it < particle_pool.size(); it++)
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
