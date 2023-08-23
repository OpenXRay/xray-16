// LightPPA.h: interface for the CLightPPA class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LIGHTPPA_H__E5B97AC9_84A6_4773_9FEF_3BC5D1CEF8B6__INCLUDED_)
#define AFX_LIGHTPPA_H__E5B97AC9_84A6_4773_9FEF_3BC5D1CEF8B6__INCLUDED_
#pragma once

#include "Layers/xrRender/light.h"

struct CLightR_Vertex
{
    Fvector P;
    Fvector N;
    float u0, v0;
    float u1, v1;
};

class CLightR_Manager
{
    xrXRC xrc;
    xr_vector<light*> selected_point;
    xr_vector<light*> selected_spot;

    // For FFP
    ref_shader hShader;
    ref_geom hGeom;

public:
    CLightR_Manager();
    virtual ~CLightR_Manager();

    void add(light* L);
    void render(u32 _priority);
    void render_point(u32 _priority);
    void render_spot(u32 _priority);
    void render_ffp();

private:
    void render_ffp_light(const light& L);
};

//////////////////////////////////////////////////////////////////////////
// binders for lighting
//////////////////////////////////////////////////////////////////////////
class cl_light_PR : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override;
};
class cl_light_C : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override;
};
class cl_light_XFORM : public R_constant_setup
{
    void setup(CBackend& cmd_list, R_constant* C) override;
};

#endif // !defined(AFX_LIGHTPPA_H__E5B97AC9_84A6_4773_9FEF_3BC5D1CEF8B6__INCLUDED_)
