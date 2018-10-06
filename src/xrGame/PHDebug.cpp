#include "StdAfx.h"
#ifdef DEBUG

#include "xrEngine/StatGraph.h"
#include "PHDebug.h"

#include "xrPhysics/MathUtils.h"

#include "xrPhysics/ExtendedGeom.h"
#include "xrPhysics/IPHWorld.h"
#include "xrPhysics/PhysicsShell.h"

#include "Level.h"

#include "debug_renderer.h"
#include "PhysicsShellHolder.h"

#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrEngine/IPHdebug.h"
#include "xrCore/xr_token.h"
#include "xrEngine/GameFont.h"

#include "xrUICore/ui_base.h"

Flags32 ph_dbg_draw_mask;
Flags32 ph_dbg_draw_mask1;
bool draw_frame = 0;

// LPCSTR	dbg_trace_object_name					=NULL;
string64 s_dbg_trace_obj_name = "none";
IGameObject* trace_object = NULL;
u32 dbg_bodies_num = 0;
u32 dbg_joints_num = 0;
u32 dbg_islands_num = 0;
u32 dbg_contacts_num = 0;
u32 dbg_tries_num = 0;
u32 dbg_saved_tries_for_active_objects = 0;
u32 dbg_total_saved_tries = 0;
u32 dbg_reused_queries_per_step = 0;
u32 dbg_new_queries_per_step = 0;
float dbg_vel_collid_damage_to_display = 7.f;

float dbg_text_height_scale = 1.f;
float dbg_text_current_height_scale = 1.f;

PHABS_DBG_V dbg_draw_abstruct0;
PHABS_DBG_V dbg_draw_abstruct1;

PHABS_DBG_V dbg_draw_cashed;
PHABS_DBG_V dbg_draw_cashed_secondary;
PHABS_DBG_V dbg_draw_simple;

enum EDBGPHDrawMode
{
    dmSecondaryThread,
    dmCashed,
    dmCashedSecondary,
    dmSimple
} dbg_ph_draw_mode = dmSecondaryThread;
u32 cash_draw_remove_time = u32(-1);

struct SPHObjDBGDraw : public SPHDBGDrawAbsract
{
    SPHObjDBGDraw(const CPHObject* obj)
    {
        AABB.set(obj->AABB);
        AABB_center.set(obj->spatial.sphere.P);
    }
    void render() { Level().debug_renderer().draw_aabb(AABB_center, AABB.x, AABB.y, AABB.z, color_xrgb(255, 0, 0)); }
    Fvector AABB;
    Fvector AABB_center;
};
void DBG_DrawPHObject(const CPHObject* obj) { DBG_DrawPHAbstruct(new SPHObjDBGDraw(obj)); }
struct SPHContactDBGDraw : public SPHDBGDrawAbsract
{
    // int geomClass;
    bool is_cyl;
    Fvector norm;
    Fvector pos;
    float depth;
    SPHContactDBGDraw(const dContact& c)
    {
        // if(dGeomGetBody(c.geom.g1))
        //{
        //	geomClass =dGeomGetClass(retrieveGeom(c.geom.g1));
        //}
        // else
        //{
        //	geomClass=dGeomGetClass(retrieveGeom(c.geom.g2));
        //}

        // is_cyl= (geomClass==dCylinderClassUser);
        is_cyl = IsCyliderContact(c);
        norm.set(cast_fv(c.geom.normal));
        pos.set(cast_fv(c.geom.pos));
        depth = c.geom.depth;
    }
    void render()
    {
        // bool is_cyl= (geomClass==dCylinderClassUser);
        Level().debug_renderer().draw_aabb(pos, .01f, .01f, .01f, color_xrgb(255 * is_cyl, 0, 255 * !is_cyl));
        Fvector dir;
        dir.set(norm);
        dir.mul(depth * 100.f);
        dir.add(pos);
        Level().debug_renderer().draw_line(Fidentity, pos, dir, color_xrgb(255 * is_cyl, 0, 255 * !is_cyl));
    }
};

void DBG_DrawContact(const dContact& c) { DBG_DrawPHAbstruct(new SPHContactDBGDraw(c)); }
struct SPHDBGDrawTri : public SPHDBGDrawAbsract
{
    Fvector v[3];
    u32 c;
    bool solid;
    SPHDBGDrawTri(CDB::RESULT* T, u32 ac)
    {
        v[0].set(T->verts[0]);
        v[1].set(T->verts[1]);
        v[2].set(T->verts[2]);
        c = ac;
        solid = false;
    }
    SPHDBGDrawTri(CDB::TRI* T, const Fvector* V_array, u32 ac)
    {
        v[0].set(V_array[T->verts[0]]);
        v[1].set(V_array[T->verts[1]]);
        v[2].set(V_array[T->verts[2]]);
        c = ac;
        solid = false;
    }
    SPHDBGDrawTri(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 ac, bool solid_)
    {
        v[0].set(v0);
        v[1].set(v1);
        v[2].set(v2);
        c = ac;
        solid = solid_;
    }
    virtual void render()
    {
        if (solid)
        {
            GEnv.DRender->dbg_DrawTRI(Fidentity, v[0], v[1], v[2], c);
            GEnv.DRender->dbg_DrawTRI(Fidentity, v[2], v[1], v[0], c);
        }
        else
        {
            Level().debug_renderer().draw_line(Fidentity, v[0], v[1], c);
            Level().debug_renderer().draw_line(Fidentity, v[1], v[2], c);
            Level().debug_renderer().draw_line(Fidentity, v[2], v[0], c);
        }
    }
};

static void clear_vector(PHABS_DBG_V& v)
{
    PHABS_DBG_I i, e;
    i = v.begin();
    e = v.end();
    for (; e != i; ++i)
    {
        xr_delete(*i);
    }
    v.clear();
}

void DBG_DrawTri(CDB::RESULT* T, u32 c) { DBG_DrawPHAbstruct(new SPHDBGDrawTri(T, c)); }
void DBG_DrawTri(CDB::TRI* T, const Fvector* V_verts, u32 c) { DBG_DrawPHAbstruct(new SPHDBGDrawTri(T, V_verts, c)); }
void DBG_DrawTri(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 ac, bool solid)
{
    DBG_DrawPHAbstruct(new SPHDBGDrawTri(v0, v1, v2, ac, solid));
}

struct SPHDBGDrawLine : public SPHDBGDrawAbsract
{
    Fvector p[2];
    u32 c;
    SPHDBGDrawLine(const Fvector& p0, const Fvector& p1, u32 ca)
    {
        p[0].set(p0);
        p[1].set(p1);
        c = ca;
    }
    virtual void render() { Level().debug_renderer().draw_line(Fidentity, p[0], p[1], c); }
};

void DBG_DrawLine(const Fvector& p0, const Fvector& p1, u32 c) { DBG_DrawPHAbstruct(new SPHDBGDrawLine(p0, p1, c)); }
void DBG_DrawMatrix(const Fmatrix& m, float size, u8 a /* = 255*/)
{
    Fvector to;
    to.add(m.c, Fvector().mul(m.i, size));
    DBG_DrawPHAbstruct(new SPHDBGDrawLine(m.c, to, color_xrgb(a, 0, 0)));
    to.add(m.c, Fvector().mul(m.j, size));
    DBG_DrawPHAbstruct(new SPHDBGDrawLine(m.c, to, color_xrgb(0, a, 0)));
    to.add(m.c, Fvector().mul(m.k, size));
    DBG_DrawPHAbstruct(new SPHDBGDrawLine(m.c, to, color_xrgb(0, 0, a)));
}

template <int>
IC void rotate(Fmatrix& m, float ang);

template <>
IC void rotate<0>(Fmatrix& m, float ang)
{
    m.rotateX(ang);
}
template <>
IC void rotate<1>(Fmatrix& m, float ang)
{
    m.rotateY(ang);
}

template <>
IC void rotate<2>(Fmatrix& m, float ang)
{
    m.rotateZ(ang);
}

template <int ax>
void DBG_DrawRotation(
    float ang0, float ang1, const Fmatrix& m, const Fvector& l, float size, u32 ac, bool solid, u32 tessel)
{
    Fvector from;
    from.set(m.c);
    Fvector ln;
    ln.set(l);
    ln.mul(size);

    const float ftess = (float)tessel;
    Fmatrix mm;
    rotate<ax>(mm, ang0);
    mm.mulA_43(m);
    Fmatrix r;
    rotate<ax>(r, (ang1 - ang0) / ftess);
    for (u32 i = 0; tessel > i; ++i)
    {
        Fvector tmp;
        mm.transform_dir(tmp, ln);
        Fvector to0;
        to0.add(from, tmp);
        mm.mulB_43(r);
        mm.transform_dir(tmp, ln);
        Fvector to1;
        to1.add(from, tmp);
        DBG_DrawPHAbstruct(new SPHDBGDrawTri(from, to0, to1, ac, solid));
    }
}

void DBG_DrawRotationX(const Fmatrix& m, float ang0, float ang1, float size, u32 ac, bool solid, u32 tessel)
{
    DBG_DrawRotation<0>(ang0, ang1, m, Fvector().set(0, 0, 1), size, ac, solid, tessel);
}

void DBG_DrawRotationY(const Fmatrix& m, float ang0, float ang1, float size, u32 ac, bool solid, u32 tessel)
{
    DBG_DrawRotation<1>(ang0, ang1, m, Fvector().set(1, 0, 0), size, ac, solid, tessel);
}

void DBG_DrawRotationZ(const Fmatrix& m, float ang0, float ang1, float size, u32 ac, bool solid, u32 tessel)
{
    DBG_DrawRotation<2>(ang0, ang1, m, Fvector().set(0, 1, 0), size, ac, solid, tessel);
}

struct SPHDBGDrawAABB : public SPHDBGDrawAbsract
{
    Fvector p[2];
    u32 c;
    SPHDBGDrawAABB(const Fvector& center, const Fvector& AABB, u32 ac)
    {
        p[0].set(center);
        p[1].set(AABB);
        c = ac;
    }
    virtual void render() { Level().debug_renderer().draw_aabb(p[0], p[1].x, p[1].y, p[1].z, c); }
};

void DBG_DrawAABB(const Fvector& center, const Fvector& AABB, u32 c)
{
    DBG_DrawPHAbstruct(new SPHDBGDrawAABB(center, AABB, c));
}

struct SPHDBGDrawOBB : public SPHDBGDrawAbsract
{
    Fmatrix m;
    Fvector h;
    u32 c;
    SPHDBGDrawOBB(const Fmatrix am, const Fvector ah, u32 ac)
    {
        m.set(am);
        h.set(ah);
        c = ac;
    }
    virtual void render() { Level().debug_renderer().draw_obb(m, h, c); }
};

void DBG_DrawOBB(const Fmatrix& m, const Fvector h, u32 c) { DBG_DrawPHAbstruct(new SPHDBGDrawOBB(m, h, c)); };
void DBG_DrawOBB(const Fobb& b, u32 c)
{
    Fmatrix m;
    b.xform_get(m);
    DBG_DrawOBB(m, b.m_halfsize, c);
}
struct SPHDBGDrawPoint : public SPHDBGDrawAbsract
{
    Fvector p;
    float size;
    u32 c;
    SPHDBGDrawPoint(const Fvector ap, float s, u32 ac)
    {
        p.set(ap), size = s;
        c = ac;
    }
    virtual void render()
    {
        // Level().debug_renderer().draw_aabb(p,size,size,size,c);
        Fmatrix m;
        m.identity();
        m.scale(size, size, size);
        m.c.set(p);
        Level().debug_renderer().draw_ellipse(m, c);
    }
};
void DBG_DrawPoint(const Fvector& p, float size, u32 c) { DBG_DrawPHAbstruct(new SPHDBGDrawPoint(p, size, c)); }
struct SPHDBGOutText : public SPHDBGDrawAbsract
{
    string1024 s;
    bool rendered;
    SPHDBGOutText(LPCSTR t)
    {
        xr_strcpy(s, t);
        rendered = false;
    }
    virtual void render()
    {
        // if(rendered) return;
        if (!fsimilar(dbg_text_current_height_scale, dbg_text_height_scale))
        {
            UI().Font().pFontStat->SetHeight(
                UI().Font().pFontStat->GetHeight() * dbg_text_height_scale / dbg_text_current_height_scale);
            dbg_text_current_height_scale = dbg_text_height_scale;
        }
        UI().Font().pFontStat->OutNext("%s", s);
        rendered = true;
    }
};

void _cdecl DBG_OutText(LPCSTR s, ...)
{
    string1024 t;
    va_list marker;
    va_start(marker, s);
    vsprintf(t, s, marker);
    va_end(marker);
    DBG_DrawPHAbstruct(new SPHDBGOutText(t));
}
struct SPHDBGTextSetColor : public SPHDBGDrawAbsract
{
    u32 color;

    SPHDBGTextSetColor(u32 c) : color(c) {}
    virtual void render() { UI().Font().pFontStat->SetColor(color); }
};

void DBG_TextSetColor(u32 color) { DBG_DrawPHAbstruct(new SPHDBGTextSetColor(color)); }
struct SPHDBGTextOutSet : public SPHDBGDrawAbsract
{
    float x, y;

    SPHDBGTextOutSet(float _x, float _y) : x(_x), y(_y) {}
    virtual void render() { UI().Font().pFontStat->OutSet(x, y); }
};

void DBG_TextOutSet(float x, float y) { DBG_DrawPHAbstruct(new SPHDBGTextOutSet(x, y)); }
void DBG_OpenCashedDraw() { dbg_ph_draw_mode = dmCashed; }
void DBG_ClosedCashedDraw(u32 remove_time)
{
    dbg_ph_draw_mode = dmSecondaryThread;
    cash_draw_remove_time = remove_time + Device.dwTimeGlobal;
}

IC void push(PHABS_DBG_V& v, SPHDBGDrawAbsract* a)
{
    // if( v.size() < 1500 )
    v.push_back(a);
}
void DBG_DrawPHAbstruct(SPHDBGDrawAbsract* a)
{
    if (dbg_ph_draw_mode != dmCashed && dbg_ph_draw_mode != dmCashedSecondary)
    {
        if (physics_world()->Processing())
            dbg_ph_draw_mode = dmSecondaryThread;
        else
            dbg_ph_draw_mode = dmSimple;
    }
    else
    {
        if (physics_world()->Processing())
            dbg_ph_draw_mode = dmCashedSecondary;
        else
            dbg_ph_draw_mode = dmCashed;
    }
    switch (dbg_ph_draw_mode)
    {
    case dmSecondaryThread:
        if (draw_frame)
        {
            push(dbg_draw_abstruct0, a);
        }
        else
        {
            push(dbg_draw_abstruct1, a);
        };
        break;
    case dmCashed: push(dbg_draw_cashed, a); break;
    case dmCashedSecondary: push(dbg_draw_cashed_secondary, a); break;
    case dmSimple: push(dbg_draw_simple, a); break;
    }
}

void DBG_PHAbstruactStartFrame(bool dr_frame)
{
    PHABS_DBG_I i, e;
    if (dr_frame)
    {
        i = dbg_draw_abstruct0.begin();
        e = dbg_draw_abstruct0.end();
    }
    else
    {
        i = dbg_draw_abstruct1.begin();
        e = dbg_draw_abstruct1.end();
    }
    for (; e != i; ++i)
    {
        xr_delete(*i);
    }
    if (dr_frame)
    {
        dbg_draw_abstruct0.clear();
    }
    else
    {
        dbg_draw_abstruct1.clear();
    }
}
void capped_cylinder_ray_collision_test();
void DBG_PHAbstructRender()
{
    PHABS_DBG_I i, e;
    if (!draw_frame)
    {
        i = dbg_draw_abstruct0.begin();
        e = dbg_draw_abstruct0.end();
    }
    else
    {
        i = dbg_draw_abstruct1.begin();
        e = dbg_draw_abstruct1.end();
    }

    for (; e != i; ++i)
    {
        if (!(*i))
        {
        }
        else
            (*i)->render();
    }
    if (dbg_ph_draw_mode != dmCashed)
    {
        PHABS_DBG_I i, e;
        i = dbg_draw_cashed.begin();
        e = dbg_draw_cashed.end();
        for (; e != i; ++i)
        {
            (*i)->render();
        }
        if (cash_draw_remove_time < Device.dwTimeGlobal)
        {
            clear_vector(dbg_draw_cashed);
        }
    }
    {
        PHABS_DBG_I i, e;
        i = dbg_draw_simple.begin();
        e = dbg_draw_simple.end();
        for (; e != i; ++i)
        {
            (*i)->render();
        }
        // clear_vector(dbg_draw_simple);
    }
    // capped_cylinder_ray_collision_test();
}
static void DBG_DrawTarckObj();
static u32 previous_frame = u32(-1);
void DBG_RenderUpdate()
{
    if (Device.Paused() || Device.dwFrame == previous_frame || !(Device.fTimeDelta > EPS_S))
        return;
    draw_frame = !draw_frame;
    clear_vector(dbg_draw_simple);
    previous_frame = Device.dwFrame;
    dbg_draw_cashed.insert(dbg_draw_cashed.end(), dbg_draw_cashed_secondary.begin(), dbg_draw_cashed_secondary.end());
    dbg_draw_cashed_secondary.clear();
    DBG_DrawTarckObj();
}
void DBG_PHAbstructClear()
{
    DBG_PHAbstruactStartFrame(true);
    DBG_PHAbstruactStartFrame(false);
    clear_vector(dbg_draw_cashed);
    clear_vector(dbg_draw_cashed_secondary);
    clear_vector(dbg_draw_simple);
}

void DBG_CashedClear()
{
    clear_vector(dbg_draw_cashed);
    clear_vector(dbg_draw_cashed_secondary);
}

void DBG_DrawFrameStart()
{
    DBG_PHAbstruactStartFrame(draw_frame);

    dbg_tries_num = 0;
    dbg_saved_tries_for_active_objects = 0;
}

void PH_DBG_Clear()
{
    DBG_PHAbstructClear();
    dbg_text_current_height_scale = 1.f;
}

void PH_DBG_Render()
{
    if (ph_dbg_draw_mask.test(phDbgDrawZDisable))
        GEnv.DRender->ZEnable(false);
    // CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE,0));
    UI().Font().pFontStat->OutSet(550, 250);
    DBG_PHAbstructRender();

    if (ph_dbg_draw_mask.test(phDbgDrawZDisable))
        GEnv.DRender->ZEnable(true);
    // CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE,1));

    // draw_frame=!draw_frame;
}

void DBG_DrawStatBeforeFrameStep()
{
    if (ph_dbg_draw_mask.test(phDbgDrawObjectStatistics))
    {
        static float obj_count = 0.f;
        static float update_obj_count = 0.f;
        obj_count = obj_count * 0.9f + float(physics_world()->ObjectsNumber()) * 0.1f;
        update_obj_count = update_obj_count * 0.9f + float(physics_world()->UpdateObjectsNumber()) * 0.1f;
        DBG_OutText("Active Phys Objects %3.0f", obj_count);
        DBG_OutText("Active Phys Update Objects %3.0f", update_obj_count);
    }
}

void DBG_DrawStatAfterFrameStep()
{
    if (ph_dbg_draw_mask.test(phDbgDrawObjectStatistics))
    {
        DBG_OutText("------------------------------");
        static float fdbg_bodies_num = 0.f;
        static float fdbg_joints_num = 0.f;
        static float fdbg_islands_num = 0.f;
        static float fdbg_contacts_num = 0.f;
        static float fdbg_tries_num = 0.f;
        fdbg_islands_num = 0.9f * fdbg_islands_num + 0.1f * float(dbg_islands_num);
        fdbg_bodies_num = 0.9f * fdbg_bodies_num + 0.1f * float(dbg_bodies_num);
        fdbg_joints_num = 0.9f * fdbg_joints_num + 0.1f * float(dbg_joints_num);
        fdbg_contacts_num = 0.9f * fdbg_contacts_num + 0.1f * float(dbg_contacts_num);
        fdbg_tries_num = 0.9f * fdbg_tries_num + 0.1f * float(dbg_tries_num);
        DBG_OutText("Ph Number of active islands %3.0f", fdbg_islands_num);
        DBG_OutText("Ph Number of active bodies %3.0f", fdbg_bodies_num);
        DBG_OutText("Ph Number of active joints %4.0f", fdbg_joints_num);
        DBG_OutText("Ph Number of contacts %4.0f", fdbg_contacts_num);
        DBG_OutText("Ph Number of tries %5.0f", fdbg_tries_num);
        DBG_OutText("------------------------------");
    }
    if (ph_dbg_draw_mask.test(phDbgDrawCashedTriesStat))
    {
        DBG_OutText("------------------------------");
        static float fdbg_saved_tries_for_active_objects = 0;
        static float fdbg_total_saved_tries = 0;

        fdbg_saved_tries_for_active_objects =
            0.9f * fdbg_saved_tries_for_active_objects + 0.1f * float(dbg_saved_tries_for_active_objects);
        fdbg_total_saved_tries = 0.9f * fdbg_total_saved_tries + 0.1f * float(dbg_total_saved_tries);
        DBG_OutText("Ph Number of cashed tries in active objects %5.0f", fdbg_saved_tries_for_active_objects);
        DBG_OutText("Ph Total number cashed %5.0f", fdbg_total_saved_tries);

        static SInertVal fdbg_reused_queries_per_step(0.9f);
        static SInertVal fdbg_new_queries_per_step(0.9f);
        fdbg_reused_queries_per_step.new_val(float(dbg_reused_queries_per_step));
        fdbg_new_queries_per_step.new_val(float(dbg_new_queries_per_step));
        DBG_OutText("Ph tri_queries_per_step %5.2f", fdbg_new_queries_per_step.val);
        DBG_OutText("Ph reused_tri_queries_per_step %5.2f", fdbg_reused_queries_per_step.val);
        DBG_OutText("------------------------------");
    }
    // draw_frame=!draw_frame;
}

CFunctionGraph::CFunctionGraph()
{
    m_stat_graph = NULL;
    m_function.clear();
}
CFunctionGraph::~CFunctionGraph()
{
    xr_delete(m_stat_graph);
    m_function.clear();
}
void CFunctionGraph::Init(type_function fun, float x0, float x1, int l, int t, int w, int h, int points_num /*=500*/,
    u32 color /*=*/, u32 bk_color)
{
    x_min = x0;
    x_max = x1;
    m_stat_graph = new CStatGraph();
    m_function = fun;
    R_ASSERT(!m_function.empty() && m_stat_graph);
    R_ASSERT(x1 > x0);
    s = (x_max - x_min) / points_num;
    R_ASSERT(s > 0.f);
    m_stat_graph->SetRect(l, t, w, h, bk_color, bk_color);
    float min = phInfinity;
    float max = -phInfinity;
    for (float x = x_min; x < x_max; x += s)
    {
        float val = m_function(x);

        save_min(min, val);
        save_max(max, val);
    }

    R_ASSERT(min < phInfinity && max > -phInfinity && min <= max);
    m_stat_graph->SetMinMax(min, max, points_num);

    for (float x = x_min; x < x_max; x += s)
    {
        float val = m_function(x);
        m_stat_graph->AppendItem(val, color);
    }
    // m_stat_graph->AddMarker(CStatGraph::stVert, 0, color_xrgb(255, 0, 0));
    // m_stat_graph->AddMarker(CStatGraph::stHor, 0, color_xrgb(255, 0, 0));
}

void CFunctionGraph::AddMarker(CStatGraph::EStyle Style, float pos, u32 Color)
{
    VERIFY(IsActive());
    ScaleMarkerPos(Style, pos);
    m_stat_graph->AddMarker(Style, pos, Color);
}
void CFunctionGraph::UpdateMarker(u32 ID, float M)
{
    VERIFY(IsActive());
    ScaleMarkerPos(ID, M);
    m_stat_graph->UpdateMarkerPos(ID, M);
}
void CFunctionGraph::ScaleMarkerPos(u32 ID, float& p)
{
    VERIFY(IsActive());
    ScaleMarkerPos(m_stat_graph->Marker(ID).m_eStyle, p);
}
void CFunctionGraph::ScaleMarkerPos(CStatGraph::EStyle Style, float& p)
{
    VERIFY(IsActive());
    if (Style == CStatGraph::stVert)
        p = ScaleX(p);
}
void CFunctionGraph::Clear()
{
    xr_delete(m_stat_graph);
    m_function.clear();
}

bool CFunctionGraph::IsActive()
{
    VERIFY((m_stat_graph == 0) == m_function.empty());
    return !!m_stat_graph;
}

LPCSTR PH_DBG_ObjectTrackName() { return s_dbg_trace_obj_name; }
// extern ENGINE_API	IGame_Level*	g_pGameLevel;
void PH_DBG_SetTrackObject()
{
    //	xr_strcpy( s_dbg_trace_obj_name,obj);
    //	dbg_trace_object_name=s_dbg_trace_obj_name;
    if (g_pGameLevel)
        trace_object = Level().Objects.FindObjectByName(PH_DBG_ObjectTrackName());
}

static LPCSTR name_bool(BOOL v)
{
    static const xr_token token_bool[] = {{"false", 0}, {"true", 1}};
    return get_token_name(token_bool, v);
}

static LPCSTR name_blend_type(CBlend::ECurvature blend)
{
    static xr_token token_blend[] = {{"eFREE_SLOT", int(CBlend::eFREE_SLOT)}, {"eAccrue", int(CBlend::eAccrue)},
        {"eFalloff", int(CBlend::eFalloff)}, {"eFORCEDWORD", int(CBlend::eFORCEDWORD)}};
    return get_token_name(token_blend, blend);
}
/*
enum
{
    dbg_track_obj_blends_bp_0			= 1<< 0,
    dbg_track_obj_blends_bp_1			= 1<< 1,
    dbg_track_obj_blends_bp_2			= 1<< 2,
    dbg_track_obj_blends_bp_3			= 1<< 3,
    dbg_track_obj_blends_motion_name	= 1<< 4,
    dbg_track_obj_blends_time			= 1<< 5,
    dbg_track_obj_blends_ammount		= 1<< 6,
    dbg_track_obj_blends_mix_params		= 1<< 7
    dbg_track_obj_blends_flags			= 1<< 8,
    dbg_track_obj_blends_state			= 1<< 9,
    dbg_track_obj_blends_dump			= 1<< 10
};
*/
Flags32 dbg_track_obj_flags = {u32(-1) & ~dbg_track_obj_blends_dump};
void DBG_AnimBlend(IKinematicsAnimated& ka, const CBlend& B)
{
    // UI().Font().pFontStat->SetHeight	(20.0f);

    DBG_OutText("-------------------------------------");
    if (dbg_track_obj_flags.test(dbg_track_obj_blends_motion_name))
    {
        std::pair<LPCSTR, LPCSTR> motion = ka.LL_MotionDefName_dbg(B.motionID);
        DBG_OutText("motion : name %s, set: %s ", motion.first, motion.second);
    }
    if (dbg_track_obj_flags.test(dbg_track_obj_blends_time))
        DBG_OutText("time current: %f, time total: %f, frame %d ", B.timeCurrent, B.timeTotal, B.dwFrame);

    if (dbg_track_obj_flags.test(dbg_track_obj_blends_ammount))
        DBG_OutText("ammount: %f, power: %f  ", B.blendAmount, B.blendPower);

    if (dbg_track_obj_flags.test(dbg_track_obj_blends_mix_params))
        DBG_OutText("accrue: %f, fallof: %f, speed: %f ", B.blendAccrue, B.blendFalloff, B.speed);

    if (dbg_track_obj_flags.test(dbg_track_obj_blends_flags))
        DBG_OutText("bonepart: %d, channel: %d, stop_at_end: %s, fall_at_end: %s ", B.bone_or_part, B.channel,
            name_bool(B.stop_at_end), name_bool(B.fall_at_end));
    if (dbg_track_obj_flags.test(dbg_track_obj_blends_state))
        DBG_OutText("state: %s, playing: %s, stop_at_end_callback: %s ", name_blend_type(B.blend_state()),
            name_bool(B.playing), name_bool(B.stop_at_end_callback));
    // DBG_OutText( "callback: %p callback param: %p", B.Callback, B.CallbackParam );
}

void DBG_AnimPartState(IKinematicsAnimated& ka, u16 part)
{
    if (!dbg_track_obj_flags.test(1 << part))
        return;
    DBG_OutText("=======================================");
    const u16 n = (u16)ka.LL_PartBlendsCount(part);
    DBG_OutText("bone part : %d num blends: %d", part, n);

    for (u16 i = 0; i < n; ++i)
        DBG_AnimBlend(ka, *ka.LL_PartBlend(part, i));
}
void DBG_AnimState(IKinematicsAnimated& ka)
{
    if (dbg_track_obj_flags.test(dbg_track_obj_blends_dump))
    {
        ka.LL_DumpBlends_dbg();
        dbg_track_obj_flags.set(dbg_track_obj_blends_dump, FALSE);
    }
    for (u16 i = 0; i < MAX_PARTS; ++i)
        DBG_AnimPartState(ka, i);
}

static void DBG_DrawTarckObj()
{
    if (!ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject))
    {
        trace_object = 0;
        return;
    }
    DBG_TextOutSet(450, 150);
    DBG_OutText("obj name: %s", PH_DBG_ObjectTrackName());

    if (!trace_object)
    {
        // trace_object= Level().Objects.FindObjectByName( PH_DBG_ObjectTrackName() );
        PH_DBG_SetTrackObject();
        if (!trace_object)
        {
            DBG_OutText("obj name: %s not found", PH_DBG_ObjectTrackName());
            return;
        }
    }

    IRenderVisual* v = trace_object->Visual();
    if (!v)
        return;
    DBG_OutText("visual name: %s ", *trace_object->cNameVisual());
    IKinematics* k = smart_cast<IKinematics*>(v);
    if (!k)
        return;
    IKinematicsAnimated* ka = smart_cast<IKinematicsAnimated*>(k);
    if (!ka)
        return;
    DBG_AnimState(*ka);
}

void DBG_DrawBones(const Fmatrix& xform, IKinematics* K)
{
    u16 nbb = K->LL_BoneCount();
    for (u16 i = 0; i < nbb; ++i)
    {
        CBoneInstance& bi = K->LL_GetBoneInstance(i);
        CBoneData& bd = K->LL_GetData(i);

        Fmatrix bone_pos = bi.mTransform;
        // K->Bone_GetAnimPos( bone_pos, i, u8(-1), false );

        DBG_DrawMatrix(Fmatrix().mul_43(xform, bone_pos), 0.1);

        u16 bp = bd.GetParentID();
        if (BI_NONE != bp)
        {
            CBoneInstance& pbi = K->LL_GetBoneInstance(bp);
            DBG_DrawLine(Fmatrix().mul_43(xform, bone_pos).c, Fmatrix().mul_43(xform, pbi.mTransform).c,
                color_xrgb(255, 255, 0));
        }
    }
    DBG_DrawMatrix(xform, 1);
    DBG_DrawPoint(xform.c, 0.1, color_xrgb(255, 125, 125));
}
void DBG_DrawBones(IGameObject& O)
{
    IKinematics* K = smart_cast<IKinematics*>(O.Visual());

    // K->CalculateBones_Invalidate();
    // K->CalculateBones();

    VERIFY(K);
    DBG_DrawBones(O.XFORM(), K);
}
void DBG_PhysBones(IGameObject& O)
{
    CPhysicsShellHolder* sh = smart_cast<CPhysicsShellHolder*>(&O);
    VERIFY(sh);
    CPhysicsShell* shell = sh->PPhysicsShell();
    if (!shell)
        return;
    u16 nb_elements = shell->get_ElementsNumber();
    for (u16 i = 0; i < nb_elements; ++i)
    {
        CPhysicsElement* e = shell->get_ElementByStoreOrder(i);

        DBG_DrawMatrix(e->XFORM(), 0.1f); // Fmatrix().mul_43( O.XFORM(),e->XFORM())
        CPhysicsElement* pE = (e)->parent_element();
        if (pE)
            DBG_DrawLine(e->XFORM().c, pE->XFORM().c, color_xrgb(255, 100, 0));
    }
}

void DBG_DrawBind(IGameObject& O)
{
    IKinematics* K = smart_cast<IKinematics*>(O.Visual());

    u16 nbb = K->LL_BoneCount();
    xr_vector<Fmatrix> binds;
    K->LL_GetBindTransform(binds);

    for (u16 i = 0; i < nbb; ++i)
    {
        CBoneData& bd = K->LL_GetData(i);

        DBG_DrawMatrix(Fmatrix().mul_43(O.XFORM(), binds[i]), 0.1, 100);
        u16 bp = bd.GetParentID();
        if (BI_NONE != bp)
        {
            DBG_DrawLine(Fmatrix().mul_43(O.XFORM(), binds[i]).c, Fmatrix().mul_43(O.XFORM(), binds[bp]).c,
                color_xrgb(0, 255, 255));
        }
    }

    u16 bip01_bi = K->LL_BoneID("bip01");
    if (bip01_bi != BI_NONE)
        DBG_DrawPoint(Fmatrix().mul_43(O.XFORM(), binds[bip01_bi]).c, 0.1, color_xrgb(255, 255, 255));
}

class cphdebug_impl : public IPhDebugRender
{
    void open_cashed_draw() { DBG_OpenCashedDraw(); }
    void close_cashed_draw(u32 remove_time) { DBG_ClosedCashedDraw(remove_time); }
    void draw_tri(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 c, bool solid)
    {
        DBG_DrawTri(v0, v1, v2, c, solid);
    }

public:
    cphdebug_impl() { ph_debug_render = this; }
} ph_debug_render_impl;

void DBG_PH_NetRelcase(IGameObject* obj)
{
    if (trace_object == obj)
        trace_object = NULL;
}

bool is_trace_obj(CPHObject* obj)
{
    return trace_object && smart_cast<IGameObject*>(obj->ref_object()) == trace_object;
}

void DBG_ObjBeforeCollision(CPHObject* obj)
{
    if (is_trace_obj(obj))
        DBG_OpenCashedDraw();
}
void DBG_ObjAfterCollision(CPHObject* obj)
{
    if (is_trace_obj(obj))
        DBG_ClosedCashedDraw(50000);
}

void DBG_ObjBeforePhTune(CPHObject* obj) {}
void DBG_ObjeAfterPhTune(CPHObject* obj) {}
Fvector dbg_trace_prev_pos = {0, 0, 0};
void DBG_ObjBeforeStep(CPHObject* obj)
{
    if (is_trace_obj(obj))
    {
        DBG_OpenCashedDraw();
        // dbg_draw_velocity	( 0.1f, color_xrgb( 255, 0, 0 ) );
        if (obj->ref_object()->ObjectPPhysicsShell())
        {
            obj->ref_object()->ObjectPPhysicsShell()->dbg_draw_force(0.1f, color_xrgb(0, 0, 255));
            Fmatrix form = Fidentity;
            obj->ref_object()->ObjectPPhysicsShell()->GetGlobalTransformDynamic(&form);
            dbg_trace_prev_pos.set(form.c);
        }
    }
}

void DBG_ObjAfterStep(CPHObject* obj)
{
    if (is_trace_obj(obj))
    {
        if (obj->ref_object()->ObjectPPhysicsShell())
        {
            obj->ref_object()->ObjectPPhysicsShell()->dbg_draw_velocity(0.1f, color_xrgb(255, 0, 0));
        }
        // dbg_draw_force		( 0.1f, color_xrgb( 0, 0, 255 ) );
    }
}

void DBG_ObjBeforePhDataUpdate(CPHObject* obj) {}
void DBG_ObjAfterPhDataUpdate(CPHObject* obj)
{
    if (!is_trace_obj(obj))
        return;
    if (obj->ref_object()->ObjectPPhysicsShell())
    {
        Fmatrix form = Fidentity;
        obj->ref_object()->ObjectPPhysicsShell()->GetGlobalTransformDynamic(&form);
        DBG_DrawLine(dbg_trace_prev_pos, form.c, color_xrgb(255, 0, 0));
    }
    DBG_ClosedCashedDraw(50000);
}

class CPHDebugOutput : public IDebugOutput
{
    virtual const Flags32& ph_dbg_draw_mask() const { return ::ph_dbg_draw_mask; }
    virtual const Flags32& ph_dbg_draw_mask1() const { return ::ph_dbg_draw_mask1; }
    virtual void DBG_DrawStatBeforeFrameStep() { ::DBG_DrawStatBeforeFrameStep(); }
    virtual void DBG_DrawStatAfterFrameStep() { ::DBG_DrawStatAfterFrameStep(); }
    // virtual	void DBG_RenderUpdate( )												=0;
    virtual void DBG_OpenCashedDraw() { ::DBG_OpenCashedDraw(); }
    virtual void DBG_ClosedCashedDraw(u32 remove_time) { ::DBG_ClosedCashedDraw(remove_time); }
    // virtual	void DBG_DrawPHAbstruct( SPHDBGDrawAbsract*	a )							=0;
    virtual void DBG_DrawPHObject(const CPHObject* obj) { ::DBG_DrawPHObject(obj); }
    virtual void DBG_DrawContact(const dContact& c) { ::DBG_DrawContact(c); }
    virtual void DBG_DrawTri(CDB::RESULT* T, u32 c) { ::DBG_DrawTri(T, c); }
    virtual void DBG_DrawTri(CDB::TRI* T, const Fvector* V_verts, u32 c) { ::DBG_DrawTri(T, V_verts, c); }
    virtual void DBG_DrawLine(const Fvector& p0, const Fvector& p1, u32 c) { ::DBG_DrawLine(p0, p1, c); }
    virtual void DBG_DrawAABB(const Fvector& center, const Fvector& AABB, u32 c) { ::DBG_DrawAABB(center, AABB, c); }
    virtual void DBG_DrawOBB(const Fmatrix& m, const Fvector h, u32 c) { ::DBG_DrawOBB(m, h, c); }
    virtual void DBG_DrawPoint(const Fvector& p, float size, u32 c) { ::DBG_DrawPoint(p, size, c); }
    virtual void DBG_DrawMatrix(const Fmatrix& m, float size, u8 a = 255) { ::DBG_DrawMatrix(m, size, a); }
    // virtual	void DBG_DrawRotationX( const Fmatrix &m, float ang0, float ang1, float size, u32 ac, bool solid =
    // false,
    // u32 tessel = 7 ) = 0;
    // virtual	void DBG_DrawRotationY( const Fmatrix &m, float ang0, float ang1, float size, u32 ac, bool solid =
    // false,
    // u32 tessel = 7 ) = 0;
    // virtual	void DBG_DrawRotationZ( const Fmatrix &m, float ang0, float ang1, float size, u32 ac, bool solid =
    // false,
    // u32 tessel = 7 ) = 0;
    virtual void _cdecl DBG_OutText(LPCSTR s, ...)
    {
        string1024 t;
        va_list marker;
        va_start(marker, s);
        vsprintf(t, s, marker);
        va_end(marker);
        DBG_DrawPHAbstruct(new SPHDBGOutText(t));
    }
    // virtual	void DBG_TextOutSet( float x, float y )									=0;
    // virtual	void DBG_TextSetColor( u32 color )										=0;
    // virtual	void DBG_DrawBind( IGameObject &O )											=0;
    // virtual	void DBG_PhysBones( IGameObject &O )										=0;
    // virtual	void DBG_DrawBones( IGameObject &O )										=0;
    virtual void DBG_DrawFrameStart() { ::DBG_DrawFrameStart(); }
    virtual void PH_DBG_Render() { ::PH_DBG_Render(); }
    virtual void PH_DBG_Clear() { ::PH_DBG_Clear(); }
    virtual LPCSTR PH_DBG_ObjectTrackName() { return ::PH_DBG_ObjectTrackName(); }
    // virtual	bool			draw_frame								()=0;
    virtual u32& dbg_tries_num()
    {
        return ::dbg_tries_num;
        //	make_string( "%s, _14_=%f \n", dump_string( make_string( "%s.i, ", name ).c_str(), form.i ).c_str( ) ,
        // form._14_ )	+
        //	make_string( "%s, _24_=%f \n", dump_string( make_string( "%s.j, ", name ).c_str(), form.j ).c_str( ) ,
        // form._24_ )	+
        //	make_string( "%s, _34_=%f \n", dump_string( make_string( "%s.k, ", name ).c_str(), form.k ).c_str( ) ,
        // form._34_  ) +
        //	make_string( "%s, _44_=%f \n", dump_string( make_string( "%s.c, ", name ).c_str(), form.c ).c_str( ) ,
        // form._44_ );
    }
    virtual u32& dbg_saved_tries_for_active_objects() { return ::dbg_saved_tries_for_active_objects; }
    virtual u32& dbg_total_saved_tries() { return ::dbg_total_saved_tries; }
    virtual u32& dbg_reused_queries_per_step() { return ::dbg_reused_queries_per_step; }
    virtual u32& dbg_new_queries_per_step() { return ::dbg_new_queries_per_step; }
    virtual u32& dbg_bodies_num() { return ::dbg_bodies_num; }
    virtual u32& dbg_joints_num() { return ::dbg_joints_num; }
    virtual u32& dbg_islands_num() { return ::dbg_islands_num; }
    virtual u32& dbg_contacts_num() { return ::dbg_contacts_num; }
    virtual float dbg_vel_collid_damage_to_display()
    {
        return ::dbg_vel_collid_damage_to_display;
        // Msg( "%s, _14_=%f ", dump_string( make_string( "%s.i, ", name ).c_str(), form.i ).c_str( ) , form._14_ );
        // Msg( "%s, _24_=%f ", dump_string( make_string( "%s.j, ", name ).c_str(), form.j ).c_str( ) , form._24_ );
        // Msg( "%s, _34_=%f ", dump_string( make_string( "%s.k, ", name ).c_str(), form.k ).c_str( ) , form._34_  );
        // Msg( "%s, _44_=%f ", dump_string( make_string( "%s.c, ", name ).c_str(), form.c ).c_str( ) , form._44_ );
    }

    virtual void DBG_ObjAfterPhDataUpdate(CPHObject* obj) { ::DBG_ObjAfterPhDataUpdate(obj); }
    virtual void DBG_ObjBeforePhDataUpdate(CPHObject* obj) { ::DBG_ObjBeforePhDataUpdate(obj); }
    virtual void DBG_ObjAfterStep(CPHObject* obj) { ::DBG_ObjAfterStep(obj); }
    virtual void DBG_ObjBeforeStep(CPHObject* obj) { ::DBG_ObjBeforeStep(obj); }
    virtual void DBG_ObjeAfterPhTune(CPHObject* obj) { ::DBG_ObjeAfterPhTune(obj); }
    virtual void DBG_ObjBeforePhTune(CPHObject* obj) { ::DBG_ObjBeforePhTune(obj); }
    virtual void DBG_ObjAfterCollision(CPHObject* obj) { ::DBG_ObjAfterCollision(obj); }
    virtual void DBG_ObjBeforeCollision(CPHObject* obj) { ::DBG_ObjBeforeCollision(obj); }
public:
    CPHDebugOutput() { ph_debug_output = this; }
} ph_debug_output_impl;

#endif
