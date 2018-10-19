#include "StdAfx.h"
#include "Geometry.h"
#include "PHDynamicData.h"
#include "ExtendedGeom.h"
#include "dcylinder/dCylinder.h"

#include "xrCore/Animation/Bone.hpp"

// global
#ifdef DEBUG
#include "debug_output.h"
#endif // #ifdef DEBUG

static void computeFinalTx(dGeomID geom_transform, dReal* final_pos, dReal* final_R)
{
    R_ASSERT2(dGeomGetClass(geom_transform) == dGeomTransformClass, "is not a geom transform");
    dGeomID obj = dGeomTransformGetGeom(geom_transform);
    const dReal* R = dGeomGetRotation(geom_transform);
    const dReal* pos = dGeomGetPosition(geom_transform);
    dMULTIPLY0_331(final_pos, R, dGeomGetPosition(obj));
    final_pos[0] += pos[0];
    final_pos[1] += pos[1];
    final_pos[2] += pos[2];
    dMULTIPLY0_333(final_R, R, dGeomGetRotation(obj));
}

void GetBoxExtensions(
    dGeomID box, const dReal* axis, const dReal* pos, const dReal* rot, float center_prg, dReal* lo_ext, dReal* hi_ext)
{
    R_ASSERT2(dGeomGetClass(box) == dBoxClass, "is not a box");
    dVector3 length;
    dGeomBoxGetLengths(box, length);
    dReal dif = dDOT(pos, axis) - center_prg;
    dReal ful_ext = dFabs(dDOT14(axis, rot + 0)) * length[0] + dFabs(dDOT14(axis, rot + 1)) * length[1] +
        dFabs(dDOT14(axis, rot + 2)) * length[2];
    ful_ext /= 2.f;
    *lo_ext = -ful_ext + dif;
    *hi_ext = ful_ext + dif;
}

void GetCylinderExtensions(
    dGeomID cyl, const dReal* axis, const dReal* pos, const dReal* rot, float center_prg, dReal* lo_ext, dReal* hi_ext)
{
    R_ASSERT2(dGeomGetClass(cyl) == dCylinderClassUser, "is not a cylinder");
    dReal radius, length;
    dGeomCylinderGetParams(cyl, &radius, &length);
    dReal dif = dDOT(pos, axis) - center_prg;
    dReal _cos = dFabs(dDOT14(axis, rot + 1));
    dReal cos1 = dDOT14(axis, rot + 0);
    dReal cos3 = dDOT14(axis, rot + 2);
    dReal _sin = _sqrt(cos1 * cos1 + cos3 * cos3);
    length /= 2.f;
    dReal ful_ext = _cos * length + _sin * radius;
    *lo_ext = -ful_ext + dif;
    *hi_ext = ful_ext + dif;
}

void GetSphereExtensions(
    dGeomID sphere, const dReal* axis, const dReal* pos, float center_prg, dReal* lo_ext, dReal* hi_ext)
{
    R_ASSERT2(dGeomGetClass(sphere) == dSphereClass, "is not a sphere");
    dReal radius = dGeomSphereGetRadius(sphere);
    dReal dif = dDOT(pos, axis) - center_prg;
    *lo_ext = -radius + dif;
    *hi_ext = radius + dif;
}

void TransformedGeometryExtensionLocalParams(
    dGeomID geom_transform, const dReal* axis, float center_prg, dReal* local_axis, dReal& local_center_prg)
{
    R_ASSERT2(dGeomGetClass(geom_transform) == dGeomTransformClass, "is not a geom transform");
    const dReal* rot = dGeomGetRotation(geom_transform);
    const dReal* pos = dGeomGetPosition(geom_transform);
    dVector3 local_pos;

    dMULTIPLY1_331(local_axis, rot, axis);
    dMULTIPLY1_331(local_pos, rot, pos);
    local_center_prg = center_prg - dDOT(local_pos, local_axis);
}

CODEGeom::CODEGeom()
{
    m_geom_transform = NULL;
    m_bone_id = u16(-1);
}

CODEGeom::~CODEGeom()
{
    if (m_geom_transform)
        destroy();
}

void CODEGeom::get_mass(dMass& m, const Fvector& ref_point, float density)
{
    get_mass(m);
    dMassAdjust(&m, density * volume());
    Fvector l;
    l.sub(local_center(), ref_point);
    dMassTranslate(&m, l.x, l.y, l.z);
}

void CODEGeom::get_mass(dMass& m, const Fvector& ref_point)
{
    get_mass(m);
    Fvector l;
    l.sub(local_center(), ref_point);
    dMassTranslate(&m, l.x, l.y, l.z);
}

void CODEGeom::add_self_mass(dMass& m, const Fvector& ref_point, float density)
{
    dMass self_mass;
    get_mass(self_mass, ref_point, density);
    dMassAdd(&m, &self_mass);
}

void CODEGeom::add_self_mass(dMass& m, const Fvector& ref_point)
{
    dMass self_mass;
    get_mass(self_mass, ref_point);
    dMassAdd(&m, &self_mass);
}

void CODEGeom::get_local_center_bt(Fvector& center)
{
    if (!m_geom_transform)
        return;
    if (!geom()) // geom is not transformed
    {
        center.set(0.f, 0.f, 0.f);
    }
    center.set(*((const Fvector*)dGeomGetPosition(geom())));
}
void CODEGeom::get_local_form_bt(Fmatrix& form)
{
    PHDynamicData::DMXPStoFMX(dGeomGetRotation(geom()), dGeomGetPosition(geom()), form);
}
void CODEGeom::get_global_center_bt(Fvector& center)
{
    center.set(*((const Fvector*)dGeomGetPosition(m_geom_transform)));
    dVector3 add;
    dMULTIPLY0_331(add, dGeomGetRotation(m_geom_transform), dGeomGetPosition(geom()));
    center.x += add[0];
    center.y += add[1];
    center.z += add[2];
}
void CODEGeom::get_xform(Fmatrix& form) const
{
    VERIFY(m_geom_transform);
    const dReal* rot = NULL;
    const dReal* pos = NULL;
    dVector3 p;
    dMatrix3 r;
    get_final_tx_bt(pos, rot, p, r);

    PHDynamicData::DMXPStoFMX(rot, pos, form);
}

bool CODEGeom::collide_fluids() const { return !m_flags.test(SBoneShape::sfNoFogCollider); }
void CODEGeom::get_Box(Fmatrix& form, Fvector& sz) const
{
    get_xform(form);
    Fvector c;
    t_get_box(this, form, sz, c);
    form.c = c;
}
/*
void CODEGeom::get_global_form_bt(Fmatrix& form)
{
    dMULTIPLY0_331 ((dReal*)(&form.c),dGeomGetRotation(m_geom_transform),dGeomGetPosition(geom()));
    form.c.add(*((const Fvector*)dGeomGetPosition(m_geom_transform)));
    dMULTIPLY3_333 ((dReal*)(&form),dGeomGetRotation(m_geom_transform),dGeomGetRotation(geom()));
    //PHDynamicData::DMXtoFMX((dReal*)(&form),form);
}
*/
void CODEGeom::set_static_ref_form(const Fmatrix& form)
{
    dGeomSetPosition(geometry_transform(), form.c.x, form.c.y, form.c.z);
    Fmatrix33 m33;
    m33.set(form);
    dMatrix3 R;
    PHDynamicData::FMX33toDMX(m33, R);
    dGeomSetRotation(geometry_transform(), R);
}
void CODEGeom::clear_motion_history(bool set_unspecified)
{
    dGeomUserDataResetLastPos(geom());
    if (set_unspecified)
        return;
    get_global_center_bt(cast_fv(dGeomGetUserData(geom())->last_pos));
#ifdef DEBUG
    Fmatrix m;
    get_xform(m);
    if (Fvector().sub(m.c, cast_fv(dGeomGetUserData(geom())->last_pos)).magnitude() > EPS)
        Msg("! WRONG THING");
#endif
}

void CODEGeom::set_build_position(const Fvector& /*ref_point*/) { clear_motion_history(true); }
void CODEGeom::set_body(dBodyID body)
{
    if (m_geom_transform)
        dGeomSetBody(m_geom_transform, body);
}

void CODEGeom::add_to_space(dSpaceID space)
{
    if (m_geom_transform)
        dSpaceAdd(space, m_geom_transform);
}
void CODEGeom::remove_from_space(dSpaceID space)
{
    if (m_geom_transform)
        dSpaceRemove(space, m_geom_transform);
}
void CODEGeom::clear_cashed_tries()
{
    if (!m_geom_transform)
        return;
    dGeomID g = geom();
    if (g)
    {
        VERIFY(dGeomGetUserData(g));
        dGeomUserDataClearCashedTries(g);
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        dGeomUserDataClearCashedTries(m_geom_transform);
    }
}
void CODEGeom::set_material(u16 ul_material)
{
    if (!m_geom_transform)
        return;
    if (geom())
    {
        VERIFY(dGeomGetUserData(geom()));
        dGeomGetUserData(geom())->material = ul_material;
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        dGeomGetUserData(m_geom_transform)->material = ul_material;
    }
}

void CODEGeom::set_contact_cb(ContactCallbackFun* ccb)
{
    if (!m_geom_transform)
        return;
    if (geom())
    {
        VERIFY(dGeomGetUserData(geom()));
        dGeomUserDataSetContactCallback(geom(), ccb);
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        dGeomUserDataSetContactCallback(m_geom_transform, ccb);
    }
}

void CODEGeom::set_obj_contact_cb(ObjectContactCallbackFun* occb)
{
    if (!m_geom_transform)
        return;
    if (geom())
    {
        VERIFY(dGeomGetUserData(geom()));
        dGeomUserDataSetObjectContactCallback(geom(), occb);
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        dGeomUserDataSetObjectContactCallback(m_geom_transform, occb);
    }
}
void CODEGeom::add_obj_contact_cb(ObjectContactCallbackFun* occb)
{
    if (!m_geom_transform)
        return;
    if (geom())
    {
        VERIFY(dGeomGetUserData(geom()));
        dGeomUserDataAddObjectContactCallback(geom(), occb);
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        dGeomUserDataAddObjectContactCallback(m_geom_transform, occb);
    }
}
void CODEGeom::remove_obj_contact_cb(ObjectContactCallbackFun* occb)
{
    if (!m_geom_transform)
        return;
    if (geom())
    {
        VERIFY(dGeomGetUserData(geom()));
        dGeomUserDataRemoveObjectContactCallback(geom(), occb);
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        dGeomUserDataRemoveObjectContactCallback(m_geom_transform, occb);
    }
}
void CODEGeom::set_callback_data(void* cd)
{
    if (!m_geom_transform)
        return;
    if (geom())
    {
        VERIFY(dGeomGetUserData(geom()));
        dGeomUserDataSetCallbackData(geom(), cd);
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        dGeomUserDataSetCallbackData(m_geom_transform, cd);
    }
}
void* CODEGeom::get_callback_data()
{
    if (!m_geom_transform)
        return NULL;
    if (geom())
    {
        VERIFY(dGeomGetUserData(geom()));
        return dGeomGetUserData(geom())->callback_data;
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        return dGeomGetUserData(m_geom_transform)->callback_data;
    }
}
void CODEGeom::set_ref_object(IPhysicsShellHolder* ro)
{
    if (!m_geom_transform)
        return;
    if (geom())
    {
        VERIFY(dGeomGetUserData(geom()));
        dGeomUserDataSetPhysicsRefObject(geom(), ro);
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        dGeomUserDataSetPhysicsRefObject(m_geom_transform, ro);
    }
}

void CODEGeom::set_ph_object(CPHObject* o)
{
    if (!m_geom_transform)
        return;
    if (geom())
    {
        VERIFY(dGeomGetUserData(geom()));
        dGeomGetUserData(geom())->ph_object = o;
    }
    else
    {
        VERIFY(dGeomGetUserData(m_geom_transform));
        dGeomGetUserData(m_geom_transform)->ph_object = o;
    }
}
void CODEGeom::move_local_basis(const Fmatrix& inv_new_mul_old)
{
    Fmatrix new_form;
    get_local_form(new_form);
    new_form.mulA_43(inv_new_mul_old);
    set_local_form(new_form);
}
void CODEGeom::build(const Fvector& ref_point)
{
    init();
    set_build_position(ref_point);
}
void CODEGeom::init()
{
    dGeomID geom = create();
    m_geom_transform = dCreateGeomTransform(0);
    dGeomTransformSetCleanup(m_geom_transform, 0);
    dGeomSetData(m_geom_transform, 0);
    dGeomTransformSetGeom(m_geom_transform, geom);
    dGeomTransformSetInfo(m_geom_transform, 1);
    dGeomCreateUserData(geom);
    dGeomUserDataSetBoneId(geom, m_bone_id);
}
void CODEGeom::destroy()
{
    if (!m_geom_transform)
        return;
    if (geom())
    {
        dGeomDestroyUserData(geom());
        dGeomDestroy(geom());
        dGeomTransformSetGeom(m_geom_transform, 0);
    }
    dGeomDestroyUserData(m_geom_transform);
    dGeomDestroy(m_geom_transform);
    m_geom_transform = NULL;
}

CBoxGeom::CBoxGeom(const Fobb& box) { m_box = box; }
void CBoxGeom::get_mass(dMass& m)
{
    Fvector& hside = m_box.m_halfsize;
    dMassSetBox(&m, 1.f, hside.x * 2.f, hside.y * 2.f, hside.z * 2.f);
    dMatrix3 DMatx;
    PHDynamicData::FMX33toDMX(m_box.m_rotate, DMatx);
    dMassRotate(&m, DMatx);
}

float CBoxGeom::volume() { return m_box.m_halfsize.x * m_box.m_halfsize.y * m_box.m_halfsize.z * 8.f; }
float CBoxGeom::radius() { return m_box.m_halfsize.x; }
void CODEGeom::get_final_tx_bt(const dReal*& p, const dReal*& R, dReal* bufV, dReal* bufM) const
{
    VERIFY(m_geom_transform);
    // dGeomID		g		=	geometry_bt()						;
    get_final_tx(m_geom_transform, p, R, bufV, bufM);
}
void CODEGeom::get_final_tx(dGeomID g, const dReal*& p, const dReal*& R, dReal* bufV, dReal* bufM)
{
    if (is_transform(g))
    {
        computeFinalTx(g, bufV, bufM);
        R = bufM;
        p = bufV;
    }
    else
    {
        R = dGeomGetRotation(g);
        p = dGeomGetPosition(g);
    }
}

void CODEGeom::set_local_form_bt(const Fmatrix& xform)
{
    dMatrix3 R;
    PHDynamicData::FMXtoDMX(xform, R);
    dGeomSetRotation(geom(), R);
    dGeomSetPosition(geom(), xform.c.x, xform.c.y, xform.c.z);
}

void CBoxGeom::get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const
{
    VERIFY(m_geom_transform);
    const dReal* rot = NULL;
    const dReal* pos = NULL;
    dVector3 p;
    dMatrix3 r;
    dGeomID g = geometry_bt();
    get_final_tx_bt(pos, rot, p, r);
    GetBoxExtensions(g, cast_fp(axis), pos, rot, center_prg, &lo_ext, &hi_ext);
}

void CBoxGeom::get_max_area_dir_bt(Fvector& dir)
{
    dVector3 length, ddir;
    dGeomBoxGetLengths(geometry(), length);
    dReal S1 = length[0] * length[1], S2 = length[0] * length[2], S3 = length[1] * length[2];
    const dReal* R = dGeomGetRotation(geometry());
    if (S1 > S2)
    {
        if (S1 > S3)
        {
            ddir[0] = R[2];
            ddir[1] = R[6];
            ddir[2] = R[10]; // S1
        }
        else
        {
            ddir[0] = R[0];
            ddir[1] = R[4];
            ddir[2] = R[8]; // S3
        }
    }
    else
    {
        if (S2 > S3)
        {
            ddir[0] = R[1];
            ddir[1] = R[5];
            ddir[2] = R[9]; // S2
        }
        else
        {
            ddir[0] = R[0];
            ddir[1] = R[4];
            ddir[2] = R[8]; // S3
        }
    }

    if (geom())
    {
        const dReal* TR = dGeomGetRotation(geometry_transform());
        dir.x = dDOT(ddir, TR);
        dir.y = dDOT(ddir, TR + 4);
        dir.z = dDOT(ddir, TR + 8);
    }
    else
    {
        dir.x = ddir[0];
        dir.y = ddir[1];
        dir.z = ddir[2];
    }
}

const Fvector& CBoxGeom::local_center() { return m_box.m_translate; }
void CBoxGeom::get_local_form(Fmatrix& form)
{
    form._14 = 0;
    form._24 = 0;
    form._34 = 0;
    form._44 = 1;
    form.i.set(m_box.m_rotate.i);
    form.j.set(m_box.m_rotate.j);
    form.k.set(m_box.m_rotate.k);
    form.c.set(m_box.m_translate);
}
void CBoxGeom::set_local_form(const Fmatrix& form)
{
    m_box.m_rotate.i.set(form.i);
    m_box.m_rotate.j.set(form.j);
    m_box.m_rotate.k.set(form.k);
    m_box.m_translate.set(form.c);
}

dGeomID CBoxGeom::create()
{
    return dCreateBox(0, m_box.m_halfsize.x * 2.f, m_box.m_halfsize.y * 2.f, m_box.m_halfsize.z * 2.f);
}
void CBoxGeom::set_size(const Fvector& half_size)
{
    m_box.m_halfsize.set(half_size);
    VERIFY(geom());
    dGeomBoxSetLengths(geom(), m_box.m_halfsize.x * 2.f, m_box.m_halfsize.y * 2.f, m_box.m_halfsize.z * 2.f);
}

void CBoxGeom::get_size(Fvector& half_size) const
{
    VERIFY(geom());
    dGeomBoxGetLengths(geom(), cast_fp(half_size));
    half_size.mul(0.5f);
}
void CBoxGeom::set_build_position(const Fvector& ref_point)
{
    inherited::set_build_position(ref_point);

    dVector3 local_position = {
        m_box.m_translate.x - ref_point.x, m_box.m_translate.y - ref_point.y, m_box.m_translate.z - ref_point.z};
    dGeomSetPosition(geom(), local_position[0], local_position[1], local_position[2]);
    dMatrix3 R;
    PHDynamicData::FMX33toDMX(m_box.m_rotate, R);
    dGeomSetRotation(geom(), R);
}

CSphereGeom::CSphereGeom(const Fsphere& sphere) { m_sphere = sphere; }
void CSphereGeom::get_mass(dMass& m) { dMassSetSphere(&m, 1.f, m_sphere.R); }
float CSphereGeom::volume() { return 4.f * M_PI * m_sphere.R * m_sphere.R * m_sphere.R / 3.f; }
float CSphereGeom::radius() { return m_sphere.R; }
void CSphereGeom::get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const
{
    VERIFY(m_geom_transform);
    const dReal* rot = NULL;
    const dReal* pos = NULL;
    dVector3 p;
    dMatrix3 r;
    dGeomID g = geometry_bt();
    get_final_tx_bt(pos, rot, p, r);
    GetSphereExtensions(g, cast_fp(axis), pos, center_prg, &lo_ext, &hi_ext);
}
const Fvector& CSphereGeom::local_center() { return m_sphere.P; }
void CSphereGeom::get_local_form(Fmatrix& form)
{
    form.identity();
    form.c.set(m_sphere.P);
}
void CSphereGeom::set_local_form(const Fmatrix& form) { m_sphere.P.set(form.c); }
dGeomID CSphereGeom::create() { return dCreateSphere(0, m_sphere.R); }
void CSphereGeom::set_build_position(const Fvector& ref_point)
{
    inherited::set_build_position(ref_point);
    dVector3 local_position = {m_sphere.P.x - ref_point.x, m_sphere.P.y - ref_point.y, m_sphere.P.z - ref_point.z};

    dGeomSetPosition(geom(), local_position[0], local_position[1], local_position[2]);
}

CCylinderGeom::CCylinderGeom(const Fcylinder& cyl) { m_cylinder = cyl; }
void CCylinderGeom::get_mass(dMass& m)
{
    dMassSetCylinder(&m, 1.f, 2, m_cylinder.m_radius, m_cylinder.m_height);
    dMatrix3 DMatx;
    Fmatrix33 m33;
    m33.j.set(m_cylinder.m_direction);
    Fvector::generate_orthonormal_basis(m33.j, m33.k, m33.i);
    PHDynamicData::FMX33toDMX(m33, DMatx);
    dMassRotate(&m, DMatx);
}

float CCylinderGeom::volume() { return M_PI * m_cylinder.m_radius * m_cylinder.m_radius * m_cylinder.m_height; }
float CCylinderGeom::radius() { return m_cylinder.m_radius; }
void CCylinderGeom::get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const
{
    VERIFY(m_geom_transform);
    const dReal* rot = NULL;
    const dReal* pos = NULL;
    dVector3 p;
    dMatrix3 r;
    dGeomID g = geometry_bt();
    get_final_tx_bt(pos, rot, p, r);
    GetCylinderExtensions(g, cast_fp(axis), pos, rot, center_prg, &lo_ext, &hi_ext);
}

void CCylinderGeom::set_radius(float r)
{
    m_cylinder.m_radius = r;
    VERIFY(geom());
    dGeomCylinderSetParams(geom(), m_cylinder.m_radius, m_cylinder.m_height);
}

const Fvector& CCylinderGeom::local_center() { return m_cylinder.m_center; }
void CCylinderGeom::get_local_form(Fmatrix& form)
{
    form._14 = 0;
    form._24 = 0;
    form._34 = 0;
    form._44 = 1;
    form.j.set(m_cylinder.m_direction);
    Fvector::generate_orthonormal_basis(form.j, form.k, form.i);
    form.c.set(m_cylinder.m_center);
}
void CCylinderGeom::set_local_form(const Fmatrix& form)
{
    m_cylinder.m_center.set(form.c);
    m_cylinder.m_direction.set(form.j);
}
dGeomID CCylinderGeom::create() { return dCreateCylinder(0, m_cylinder.m_radius, m_cylinder.m_height); }
void CCylinderGeom::set_build_position(const Fvector& ref_point)
{
    inherited::set_build_position(ref_point);
    dVector3 local_position = {
        m_cylinder.m_center.x - ref_point.x, m_cylinder.m_center.y - ref_point.y, m_cylinder.m_center.z - ref_point.z};

    dGeomSetPosition(geom(), local_position[0], local_position[1], local_position[2]);
    dMatrix3 R;
    Fmatrix33 m33;
    m33.j.set(m_cylinder.m_direction);
    Fvector::generate_orthonormal_basis(m33.j, m33.k, m33.i);
    PHDynamicData::FMX33toDMX(m33, R);
    dGeomSetRotation(geom(), R);
}

#ifdef DEBUG
void CODEGeom::dbg_draw(float scale, u32 color, Flags32 flags) const
{
    Fmatrix m;
    get_xform(m);
    debug_output().DBG_DrawMatrix(m, 0.02f);
    debug_output().DBG_DrawPoint(m.c, 0.001f, color_xrgb(0, 255, 255));
}

void CBoxGeom::dbg_draw(float scale, u32 color, Flags32 flags) const
{
    inherited::dbg_draw(scale, color, flags);

    dGeomID g = geom();
    VERIFY(g);

    Fmatrix m;
    get_xform(m);

    dVector3 l;
    dGeomBoxGetLengths(g, l);

    debug_output().DBG_DrawOBB(m, cast_fv(l).mul(0.5f), color);
}

void CSphereGeom::dbg_draw(float scale, u32 color, Flags32 flags) const
{
    inherited::dbg_draw(scale, color, flags);

    Fmatrix m;
    get_xform(m);

    dGeomID g = geom();
    VERIFY(g);
    dGeomSphereGetRadius(g);

    debug_output().DBG_DrawPoint(m.c, dGeomSphereGetRadius(g), color);
}

void CCylinderGeom::dbg_draw(float scale, u32 color, Flags32 flags) const
{
    inherited::dbg_draw(scale, color, flags);

    dGeomID g = geom();
    VERIFY(g);

    Fmatrix m;
    get_xform(m);

    float r = 1, h = 1;
    dGeomCylinderGetParams(g, &r, &h);
    Fvector ext(Fvector().set(r, h * 0.5f, r));
    debug_output().DBG_DrawOBB(m, ext, color);
}
#endif
