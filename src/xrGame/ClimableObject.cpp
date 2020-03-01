#include "StdAfx.h"
#include "ClimableObject.h"
#include "xrPhysics/IPHStaticGeomShell.h"
#include "xrServer_Objects_ALife.h"
#include "xrPhysics/PHCharacter.h"
#include "xrPhysics/MathUtils.h"
#include "xrPhysics/ExtendedGeom.h"
#include "xrEngine/GameMtlLib.h"
#ifdef DEBUG
#include "debug_renderer.h"
#include "Level.h"
#include "PHDebug.h"
#endif

#ifdef DEBUG
#include "debug_renderer.h"
#endif

static const float down_leader_extension_tolerance = 0.2f;
static const float up_leader_extension_tolerance = 0.0f;

IC void OrientToNorm(const Fvector& normal, Fmatrix& form, Fobb& box)
{
    Fvector* ax_pointer = (Fvector*)&form;
    float* s_pointer = (float*)&(box.m_halfsize);
    float max_dot = abs(ax_pointer[0].dotproduct(normal));
    float min_size = box.m_halfsize.x;
    int max_ax_i = 0, min_size_i = 0;
    for (int i = 1; 3 > i; ++i)
    {
        float dot_pr = abs(ax_pointer[i].dotproduct(normal));
        if (max_dot < dot_pr)
        {
            max_ax_i = i;
            max_dot = dot_pr;
        }
        if (min_size > s_pointer[i])
        {
            min_size_i = i;
            min_size = s_pointer[i];
        }
    }
    VERIFY(min_size_i == max_ax_i);
    if (ax_pointer[max_ax_i].dotproduct(normal) < 0.f)
    {
        ax_pointer[max_ax_i].invert();
        ax_pointer[(max_ax_i + 1) % 3].invert();
    }
}

CClimableObject::CClimableObject() : m_pStaticShell(nullptr), m_radius(0), m_material(u16(-1)) {}
CClimableObject::~CClimableObject() {}
void CClimableObject::Load(LPCSTR section) { inherited::Load(section); }
BOOL CClimableObject::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeObjectClimable* CLB = smart_cast<CSE_ALifeObjectClimable*>(e);
    R_ASSERT(CLB);
    m_material = GMLib.GetMaterialIdx(CLB->material.c_str());
    const Fmatrix& b = CLB->shapes[0].data.box;
    m_box.m_halfsize.set(b._11, b._22, b._33);
    m_radius = _max(_max(m_box.m_halfsize.x, m_box.m_halfsize.y), m_box.m_halfsize.z);

    // m_box.m_halfsize.set(1.f,1.f,1.f);
    BOOL ret = inherited::net_Spawn(DC);

    spatial.type &= ~STYPE_VISIBLEFORAI;

    const float f_min_width = 0.2f;
    Fvector shift;
    shift.set(0.f, 0.f, 0.f);
    SORT(
        b._11, m_axis.set(XFORM().i); m_axis.mul(m_box.m_halfsize.x), m_side.set(XFORM().i);
        m_side.mul(m_box.m_halfsize.x), m_norm.set(XFORM().i); if (m_box.m_halfsize.x < f_min_width) {
            m_box.m_halfsize.x = f_min_width;
            shift.set(1.f, 0.f, 0.f);
        };
        m_norm.mul(m_box.m_halfsize.x), b._22, m_axis.set(XFORM().j);
        m_axis.mul(m_box.m_halfsize.y), m_side.set(XFORM().j); m_side.mul(m_box.m_halfsize.y), m_norm.set(XFORM().j);
        if (m_box.m_halfsize.y < f_min_width) {
            m_box.m_halfsize.y = f_min_width;
            shift.set(0.f, 1.f, 0.f);
        };
        m_norm.mul(m_box.m_halfsize.y), b._33, m_axis.set(XFORM().k);
        m_axis.mul(m_box.m_halfsize.z), m_side.set(XFORM().k); m_side.mul(m_box.m_halfsize.z), m_norm.set(XFORM().k);
        if (m_box.m_halfsize.z < f_min_width) {
            m_box.m_halfsize.z = f_min_width;
            shift.set(0.f, 0.f, 1.f);
        };
        m_norm.mul(m_box.m_halfsize.z));
    shift.mul(f_min_width);

    XFORM().transform_dir(shift);
    Position().sub(shift);
    m_box.xform_set(Fidentity);

    m_pStaticShell = P_BuildLeaderGeomShell(this, ObjectContactCallback, m_box);

    if (m_axis.y < 0.f)
    {
        m_axis.invert();
        m_side.invert();
    }
    //	shedule_unregister();
    processing_deactivate();
    // m_pStaticShell->set_ObjectContactCallback(ObjectContactCallback);
    return ret;
}
void CClimableObject::net_Destroy()
{
    inherited::net_Destroy();
    DestroyStaticGeomShell(m_pStaticShell);

    // m_pStaticShell->Deactivate();
    // xr_delete(m_pStaticShell);
}
void CClimableObject::shedule_Update(u32 dt) // Called by shedule
{
    inherited::shedule_Update(dt);
}
void CClimableObject::UpdateCL() // Called each frame, so no need for d
{
    inherited::UpdateCL();
}

void CClimableObject::Center(Fvector& C) const { C.set(XFORM().c); }
float CClimableObject::Radius() const { return m_radius; }
float CClimableObject::DDLowerP(CPHCharacter* actor, Fvector& out_dir) const
{
    VERIFY(actor);
    Fvector pos;
    LowerPoint(out_dir);
    actor->GetFootCenter(pos);
    out_dir.sub(pos);
    return to_mag_and_dir(out_dir);
}
float CClimableObject::DDUpperP(CPHCharacter* actor, Fvector& out_dir) const
{
    VERIFY(actor);
    Fvector pos;
    UpperPoint(out_dir);
    actor->GetFootCenter(pos);
    out_dir.sub(pos);
    return to_mag_and_dir(out_dir);
}

void CClimableObject::DefineClimbState(CPHCharacter* actor) const {}
float CClimableObject::DDAxis(Fvector& dir) const
{
    dir.set(m_axis);
    return to_mag_and_dir(dir);
}

float CClimableObject::DDSide(Fvector& dir) const
{
    dir.set(m_side);
    return to_mag_and_dir(dir);
}
float CClimableObject::DDNorm(Fvector& dir) const
{
    dir.set(m_norm);
    return to_mag_and_dir(dir);
}
float CClimableObject::DDToAxis(CPHCharacter* actor, Fvector& out_dir) const
{
    VERIFY(actor);
    DToAxis(actor, out_dir);
    return to_mag_and_dir(out_dir);
}

void CClimableObject::POnAxis(CPHCharacter* actor, Fvector& P) const
{
    VERIFY(actor);
    actor->GetFootCenter(P);
    prg_pos_on_axis(Position(), m_axis, P);
}
void CClimableObject::LowerPoint(Fvector& P) const
{
    P.sub(XFORM().c, m_axis);
    P.add(m_norm);
}

void CClimableObject::UpperPoint(Fvector& P) const
{
    P.add(XFORM().c, m_axis);
    P.add(m_norm);
}

void CClimableObject::DToAxis(CPHCharacter* actor, Fvector& dir) const
{
    VERIFY(actor);
    POnAxis(actor, dir);
    Fvector pos;
    actor->GetFootCenter(pos);
    dir.sub(pos);
}
void CClimableObject::DSideToAxis(CPHCharacter* actor, Fvector& dir) const
{
    VERIFY(actor);
    DToAxis(actor, dir);
    Fvector side;
    side.set(m_side);
    to_mag_and_dir(side);
    side.mul(side.dotproduct(dir));
    dir.set(side);
}

float CClimableObject::DDSideToAxis(CPHCharacter* actor, Fvector& dir) const
{
    VERIFY(actor);
    DToAxis(actor, dir);
    Fvector side;
    side.set(m_side);
    to_mag_and_dir(side);
    float dot = side.dotproduct(dir);
    if (dot > 0.f)
    {
        dir.set(side);
        return dot;
    }
    else
    {
        dir.set(side);
        dir.invert();
        return -dot;
    }
}

void CClimableObject::DToPlain(CPHCharacter* actor, Fvector& dist) const
{
    VERIFY(actor);
    DToAxis(actor, dist);
    Fvector norm;
    norm.set(m_norm);
    to_mag_and_dir(norm);
    float dot = norm.dotproduct(dist);
    norm.mul(dot);
    dist.set(norm);
}

float CClimableObject::DDToPlain(CPHCharacter* actor, Fvector& dir) const
{
    VERIFY(actor);
    DToPlain(actor, dir);
    return to_mag_and_dir(dir);
}

bool CClimableObject::InTouch(CPHCharacter* actor) const
{
    VERIFY(actor);
    Fvector dir;
    const float normal_tolerance = 0.05f;
    float foot_radius = actor->FootRadius();
    return (DDToPlain(actor, dir) < foot_radius + m_norm.magnitude() + normal_tolerance &&
               DDSideToAxis(actor, dir) < m_side.magnitude()) &&
        InRange(actor);
}

float CClimableObject::AxDistToUpperP(CPHCharacter* actor) const
{
    VERIFY(actor);
    Fvector v1, v2;
    actor->GetFootCenter(v1);
    UpperPoint(v2);
    v2.sub(v1);
    v1.set(m_axis);
    to_mag_and_dir(v1);
    return v1.dotproduct(v2);
}

float CClimableObject::AxDistToLowerP(CPHCharacter* actor) const
{
    VERIFY(actor);
    Fvector v1, v2;
    actor->GetFootCenter(v1);
    LowerPoint(v2);
    v2.sub(v1);
    v1.set(m_axis);
    to_mag_and_dir(v1);
    return -v1.dotproduct(v2);
}
bool CClimableObject::InRange(CPHCharacter* actor) const
{
    VERIFY(actor);
    return AxDistToLowerP(actor) > -down_leader_extension_tolerance &&
        AxDistToUpperP(actor) + actor->FootRadius() > -up_leader_extension_tolerance;
}

bool CClimableObject::BeforeLadder(CPHCharacter* actor, float tolerance /*=0.f*/) const
{
    VERIFY(actor);
    Fvector d;
    DToAxis(actor, d);
    Fvector n;
    n.set(Norm());
    float width = to_mag_and_dir(n);
    return d.dotproduct(n) < -(width + actor->FootRadius() / 2.f + tolerance);
}

BOOL CClimableObject::UsedAI_Locations() { return FALSE; }
void CClimableObject::ObjectContactCallback(
    bool& do_colide, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/)
{
    dxGeomUserData* usr_data_1 = PHRetrieveGeomUserData(c.geom.g1);
    dxGeomUserData* usr_data_2 = PHRetrieveGeomUserData(c.geom.g2);
    dxGeomUserData* usr_data_ch = NULL;
    dxGeomUserData* usr_data_lad = NULL;
    CClimableObject* this_object = NULL;
    CPHCharacter* ch = NULL;
    float norm_sign = 0.f;
    if (bo1)
    {
        usr_data_ch = usr_data_2;
        usr_data_lad = usr_data_1;
        norm_sign = -1.f;
    }
    else
    {
        norm_sign = 1.f;
        usr_data_ch = usr_data_1;
        usr_data_lad = usr_data_2;
    }

    if (usr_data_ch && usr_data_ch->ph_object && usr_data_ch->ph_object->CastType() == CPHObject::tpCharacter)
        ch = static_cast<CPHCharacter*>(usr_data_ch->ph_object);
    else
    {
        do_colide = false;
        return;
    }
    VERIFY(ch);
    VERIFY(usr_data_lad);
    this_object = static_cast<CClimableObject*>(usr_data_lad->ph_ref_object);
    VERIFY(this_object);
    if (!this_object->BeforeLadder(ch, -0.1f))
        do_colide = false;
}
#ifdef DEBUG
extern Flags32 dbg_net_Draw_Flags;
void CClimableObject::OnRender()
{
    if (!dbg_net_Draw_Flags.test(dbg_draw_climbable) && !ph_dbg_draw_mask.test(phDbgLadder))
        return;

    Fmatrix form;
    m_box.xform_get(form);
    // form.mulA(XFORM());
    Level().debug_renderer().draw_obb(XFORM(), m_box.m_halfsize, color_xrgb(0, 0, 255));
    Fvector p1, p2, d;
    d.set(m_axis);
    p1.add(XFORM().c, d);
    p2.sub(XFORM().c, d);
    Level().debug_renderer().draw_line(Fidentity, p1, p2, color_xrgb(255, 0, 0));

    d.set(m_side);
    p1.add(XFORM().c, d);
    p2.sub(XFORM().c, d);
    Level().debug_renderer().draw_line(Fidentity, p1, p2, color_xrgb(255, 0, 0));

    d.set(m_norm);
    d.mul(10.f);
    p1.add(XFORM().c, d);
    p2.set(XFORM().c);
    Level().debug_renderer().draw_line(Fidentity, p1, p2, color_xrgb(0, 255, 0));
}
#endif
