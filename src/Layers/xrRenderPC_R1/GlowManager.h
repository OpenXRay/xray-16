// GlowManager.h: interface for the CGlowManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GLOWMANAGER_H__EC35911F_479B_469A_845C_1A64D81D0326__INCLUDED_)
#define AFX_GLOWMANAGER_H__EC35911F_479B_469A_845C_1A64D81D0326__INCLUDED_
#pragma once

#include "xrCDB/ISpatial.h"
#include "xrCDB/xr_collide_defs.h"

class CGlow : public IRender_Glow, public SpatialBase
{
public:
    struct
    {
        u32 bActive : 1;
    } flags;
    float fade;
    ref_shader shader;
    u32 dwFrame;

    Fvector position;
    Fvector direction;
    float radius;
    Fcolor color;

    // Ray-testing cache
    BOOL bTestResult;
    collide::ray_cache RayCache;
    u32 qid_pass;
    u32 qid_total;

public:
    CGlow();
    virtual ~CGlow();

    virtual void set_active(bool);
    virtual bool get_active();
    virtual void set_position(const Fvector& P);
    virtual void set_direction(const Fvector& P);
    virtual void set_radius(float R);
    virtual void set_texture(LPCSTR name);
    virtual void set_color(const Fcolor& C);
    virtual void set_color(float r, float g, float b);
    virtual void spatial_move();
};

#define MAX_GlowsPerFrame 64

class CGlowManager
{
    xr_vector<ref_glow> Glows;
    xr_vector<ref_glow> Selected;
    xr_vector<ref_glow> SelectedToTest_2; // 2-frames behind
    xr_vector<ref_glow> SelectedToTest_1; // 1-frames behind
    xr_vector<ref_glow> SelectedToTest_0; // 0-frames behind
    ref_geom hGeom;

    BOOL b_hardware;
    u32 dwTestID;

public:
    void add(ref_glow g);

    void Load(IReader* fs);
    void Unload();

    void render_selected();
    void render_sw();
    void render_hw();
    void Render();

    CGlowManager();
    ~CGlowManager();
};

#endif // !defined(AFX_GLOWMANAGER_H__EC35911F_479B_469A_845C_1A64D81D0326__INCLUDED_)
