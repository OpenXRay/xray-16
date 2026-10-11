#include "StdAfx.h"
#include "Geometry.h"
#include "PHGeometryOwner.h"
#include "PHDynamicData.h"
#include "ExtendedGeom.h"

#include "xrCore/Animation/Bone.hpp"

#ifdef DEBUG
#include "debug_output.h"
#endif

CPhysicsGeom::CPhysicsGeom() : m_geom_shape(nullptr), m_bone_id(u16(-1)), m_element_position(u16(-1)), m_char_handle(INVALID_CHARACTER_VIRTUAL_HANDLE)
{
}

CPhysicsGeom::~CPhysicsGeom()
{
    xr_delete(object_callbacks);
    destroy();
}

void CPhysicsGeom::get_mass(float& m, const Fvector& ref_point, float density)
{
    get_mass(m);
    m *= density;
}

void CPhysicsGeom::get_mass(float& m, const Fvector& ref_point)
{
    get_mass(m);
}

void CPhysicsGeom::add_self_mass(float& m, const Fvector& ref_point, float density)
{
    float self_mass;
    get_mass(self_mass, ref_point, density);
    m += self_mass;
}

void CPhysicsGeom::add_self_mass(float& m, const Fvector& ref_point)
{
    float self_mass;
    get_mass(self_mass, ref_point);
    m += self_mass;
}

void CPhysicsGeom::get_local_center_bt(Fvector& center)
{
    center = local_center();
}

void CPhysicsGeom::get_local_form_bt(Fmatrix& form)
{
    if (has_runtime_local) form = runtime_local;
    else { get_local_form(form); form.c.sub(reference_center); }
}

void CPhysicsGeom::get_global_center_bt(Fvector& center)
{
    Fmatrix form;
    get_xform(form);
    center = form.c;
}

void CPhysicsGeom::get_xform(Fmatrix& form) const
{
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        Fmatrix body, local;
        GetPhysicsCore()->GetBodyTransform(m_char_handle, body);
        const_cast<CPhysicsGeom*>(this)->get_local_form_bt(local);
        form.mul_43(body, local);
    }
    else
    {
        // Fallback, если тело еще не создано
        const_cast<CPhysicsGeom*>(this)->get_local_form(form);
    }
}

bool CPhysicsGeom::collide_fluids() const
{
    return !m_flags.test(SBoneShape::sfNoFogCollider);
}

void CPhysicsGeom::get_Box(Fmatrix& form, Fvector& sz) const
{
    get_xform(form);
    Fvector c;
    t_get_box(this, form, sz, c);
    form.c = c;
}

void CPhysicsGeom::set_static_ref_form(const Fmatrix& form)
{
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->SetBodyTransform(m_char_handle, form);
    }
}

void CPhysicsGeom::clear_motion_history(bool set_unspecified)
{
    // Оставлено пустым, так как Jolt Physics сам управляет интерполяцией
}

void CPhysicsGeom::set_build_position(const Fvector& ref_point)
{
    reference_center = ref_point;
    clear_motion_history(true);
}

void CPhysicsGeom::set_body(CharacterVirtualHandle body)
{
    m_char_handle = body;
}

void CPhysicsGeom::clear_cashed_tries()
{
    // В Jolt Physics кэширование коллизий идет на уровне ядра (NarrowPhase),
    // ручная очистка со стороны геометрии больше не требуется.
}

void CPhysicsGeom::set_material(u16 ul_material) {
    material = ul_material;
    if (m_geom_shape) GetPhysicsCore()->SetShapeMaterial(m_geom_shape, material);
}
void CPhysicsGeom::set_contact_cb(ObjectContactCallbackFun* ccb) { contact_callback = ccb; }
void CPhysicsGeom::set_obj_contact_cb(ObjectContactCallbackFun* occb) { dGeomUserDataSetObjectContactCallback(this, occb); }
void CPhysicsGeom::add_obj_contact_cb(ObjectContactCallbackFun* occb) { dGeomUserDataAddObjectContactCallback(this, occb); }
void CPhysicsGeom::remove_obj_contact_cb(ObjectContactCallbackFun* occb) { dGeomUserDataRemoveObjectContactCallback(this, occb); }
void CPhysicsGeom::set_callback_data(void* cd) { callback_data = cd; }
void* CPhysicsGeom::get_callback_data() { return callback_data; }
void CPhysicsGeom::set_ref_object(IPhysicsShellHolder* ro) { ph_ref_object = ro; }
void CPhysicsGeom::set_ph_object(CPHObject* o) { ph_object = o; }

void CPhysicsGeom::move_local_basis(const Fmatrix& inv_new_mul_old)
{
    inv_new_mul_old.transform_tiny(native_mass_center);
    Fmatrix new_form;
    get_local_form(new_form);
    new_form.mulA_43(inv_new_mul_old);
    set_local_form(new_form);
}

void CPhysicsGeom::build(const Fvector& ref_point)
{
    init();
    set_build_position(ref_point);
}

void CPhysicsGeom::init()
{
    if (!m_geom_shape)
        m_geom_shape = create();
    if (m_geom_shape) GetPhysicsCore()->SetShapeMaterial(m_geom_shape, material);
}

void CPhysicsGeom::destroy()
{
    if (m_geom_shape)
    {
        GetPhysicsCore()->DestroyCDBModel(m_geom_shape);
        m_geom_shape = nullptr;
    }
}

void CPhysicsGeom::set_local_form_bt(const Fmatrix& xform)
{
    runtime_local = xform;
    has_runtime_local = true;
    if (owner) owner->RebuildNativeShape();
    // В Jolt Physics геометрия добавляется в составную форму (CompoundShape)
    // с нужным оффсетом изначально при создании тела. Нам не нужно
    // и нельзя менять m_box (исходные данные кости), как это делалось в ODE.
}

// ============================================================================
// CBoxGeom
// ============================================================================

CBoxGeom::CBoxGeom(const Fobb& box) : m_box(box) { }

void CBoxGeom::get_mass(float& m)
{
    m = volume();
}

float CBoxGeom::volume()
{
    return m_box.m_halfsize.x * m_box.m_halfsize.y * m_box.m_halfsize.z * 8.f;
}

float CBoxGeom::radius()
{
    return _max(m_box.m_halfsize.x, _max(m_box.m_halfsize.y, m_box.m_halfsize.z));
}

void CBoxGeom::get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const
{
    Fmatrix m;
    get_xform(m);
    Fvector ext = m_box.m_halfsize;
    float dif = m.c.dotproduct(axis) - center_prg;

    float ful_ext = _abs(axis.dotproduct(m.i)) * ext.x +
                    _abs(axis.dotproduct(m.j)) * ext.y +
                    _abs(axis.dotproduct(m.k)) * ext.z;

    lo_ext = -ful_ext + dif;
    hi_ext = ful_ext + dif;
}

void CBoxGeom::get_max_area_dir_bt(Fvector& dir)
{
    Fvector ext = m_box.m_halfsize;
    float S1 = ext.x * ext.y, S2 = ext.x * ext.z, S3 = ext.y * ext.z;

    Fmatrix m;
    get_xform(m);

    if (S1 > S2)
    {
        if (S1 > S3) dir = m.k;
        else         dir = m.i;
    }
    else
    {
        if (S2 > S3) dir = m.j;
        else         dir = m.i;
    }
}

const Fvector& CBoxGeom::local_center() { return m_box.m_translate; }

void CBoxGeom::get_local_form(Fmatrix& form)
{
    form._14 = 0; form._24 = 0; form._34 = 0; form._44 = 1;
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

PhysicsShapeHandle CBoxGeom::create()
{
    return GetPhysicsCore()->CreateBoxShape(m_box.m_halfsize);
}

void CBoxGeom::set_size(const Fvector& half_size)
{
    m_box.m_halfsize.set(half_size);
    destroy();
    init();
    if (owner) owner->RebuildNativeShape();
}

void CBoxGeom::get_size(Fvector& half_size) const
{
    half_size = m_box.m_halfsize;
}

void CBoxGeom::set_build_position(const Fvector& ref_point)
{
    inherited::set_build_position(ref_point);
}

// ============================================================================
// CSphereGeom
// ============================================================================

CSphereGeom::CSphereGeom(const Fsphere& sphere) : m_sphere(sphere) { }

void CSphereGeom::get_mass(float& m)
{
    m = volume();
}

float CSphereGeom::volume()
{
    return 4.f * M_PI * m_sphere.R * m_sphere.R * m_sphere.R / 3.f;
}

float CSphereGeom::radius()
{
    return m_sphere.R;
}

void CSphereGeom::get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const
{
    Fmatrix m;
    get_xform(m);
    float dif = m.c.dotproduct(axis) - center_prg;
    lo_ext = -m_sphere.R + dif;
    hi_ext = m_sphere.R + dif;
}

const Fvector& CSphereGeom::local_center() { return m_sphere.P; }

void CSphereGeom::get_local_form(Fmatrix& form)
{
    form.identity();
    form.c.set(m_sphere.P);
}

void CSphereGeom::set_local_form(const Fmatrix& form)
{
    m_sphere.P.set(form.c);
}

PhysicsShapeHandle CSphereGeom::create() { return GetPhysicsCore()->CreateSphereShape(m_sphere.R); }

void CSphereGeom::set_build_position(const Fvector& ref_point)
{
    inherited::set_build_position(ref_point);
}

// ============================================================================
// CCylinderGeom
// ============================================================================

CCylinderGeom::CCylinderGeom(const Fcylinder& cyl) : m_cylinder(cyl) { }

void CCylinderGeom::get_mass(float& m)
{
    m = volume();
}

float CCylinderGeom::volume()
{
    return M_PI * m_cylinder.m_radius * m_cylinder.m_radius * m_cylinder.m_height;
}

float CCylinderGeom::radius()
{
    return m_cylinder.m_radius;
}

void CCylinderGeom::get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const
{
    Fmatrix m;
    get_xform(m);
    float dif = m.c.dotproduct(axis) - center_prg;
    float _cos = _abs(axis.dotproduct(m.j)); // предполагая, что j это ось цилиндра
    float _sin = _sqrt(1.f - _cos * _cos);

    float ful_ext = _cos * (m_cylinder.m_height * 0.5f) + _sin * m_cylinder.m_radius;
    lo_ext = -ful_ext + dif;
    hi_ext = ful_ext + dif;
}

void CCylinderGeom::set_radius(float r)
{
    m_cylinder.m_radius = r;
    destroy();
    init();
    if (owner) owner->RebuildNativeShape();
}

const Fvector& CCylinderGeom::local_center() { return m_cylinder.m_center; }

void CCylinderGeom::get_local_form(Fmatrix& form)
{
    form._14 = 0; form._24 = 0; form._34 = 0; form._44 = 1;
    form.j.set(m_cylinder.m_direction);
    Fvector::generate_orthonormal_basis(form.j, form.k, form.i);
    form.c.set(m_cylinder.m_center);
}

void CCylinderGeom::set_local_form(const Fmatrix& form)
{
    m_cylinder.m_center.set(form.c);
    m_cylinder.m_direction.set(form.j);
}

PhysicsShapeHandle CCylinderGeom::create() { return GetPhysicsCore()->CreateCylinderShape(m_cylinder.m_radius, m_cylinder.m_height * 0.5f); }

void CCylinderGeom::set_build_position(const Fvector& ref_point)
{
    inherited::set_build_position(ref_point);
}

#ifdef DEBUG
void CPhysicsGeom::dbg_draw(float scale, u32 color, Flags32 flags) const
{
    Fmatrix m;
    get_xform(m);
    debug_output().DBG_DrawMatrix(m, 0.02f);
    debug_output().DBG_DrawPoint(m.c, 0.001f, color_xrgb(0, 255, 255));
}

void CBoxGeom::dbg_draw(float scale, u32 color, Flags32 flags) const
{
    inherited::dbg_draw(scale, color, flags);

    Fmatrix m;
    get_xform(m);
    debug_output().DBG_DrawOBB(m, m_box.m_halfsize, color);
}

void CSphereGeom::dbg_draw(float scale, u32 color, Flags32 flags) const
{
    inherited::dbg_draw(scale, color, flags);

    Fmatrix m;
    get_xform(m);
    debug_output().DBG_DrawPoint(m.c, m_sphere.R, color);
}

void CCylinderGeom::dbg_draw(float scale, u32 color, Flags32 flags) const
{
    inherited::dbg_draw(scale, color, flags);

    Fmatrix m;
    get_xform(m);
    Fvector ext(Fvector().set(m_cylinder.m_radius, m_cylinder.m_height * 0.5f, m_cylinder.m_radius));
    debug_output().DBG_DrawOBB(m, ext, color);
}
#endif
