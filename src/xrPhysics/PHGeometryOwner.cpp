#include "StdAfx.h"
#include "PHGeometryOwner.h"
#include "PHWorld.h"

#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"

CPHGeometryOwner::CPHGeometryOwner()
{
    b_builded = false;
    m_mass_center.set(0, 0, 0);
    VERIFY(ph_world);
    contact_callback = ph_world->default_contact_shotmark();
    object_contact_callback = nullptr;
    ul_material = GMLib.GetMaterialIdx("objects" DELIMITER "small_box");
    m_phys_ref_object = nullptr;
}

CPHGeometryOwner::~CPHGeometryOwner()
{
    GEOM_I i_geom = m_geoms.begin(), e = m_geoms.end();
    for (; i_geom != e; ++i_geom)
        xr_delete(*i_geom);
    m_geoms.clear();
}

void CPHGeometryOwner::build_Geom(CPhysicsGeom& geom)
{
    geom.owner = this;
    geom.build(m_mass_center);
    geom.set_material(ul_material);
    if (contact_callback)
        geom.set_contact_cb(contact_callback);
    if (object_contact_callback)
        geom.set_obj_contact_cb(object_contact_callback);
    if (m_phys_ref_object)
        geom.set_ref_object(m_phys_ref_object);
}

void CPHGeometryOwner::build_Geom(u16 i)
{
    CPhysicsGeom& geom = *m_geoms[i];
    build_Geom(geom);
    geom.element_position() = i;
}

void CPHGeometryOwner::build()
{
    if (b_builded)
        return;

    u16 geoms_size = u16(m_geoms.size());
    for (u16 i = 0; i < geoms_size; ++i)
        build_Geom(i);
    b_builded = true;
}

void CPHGeometryOwner::destroy()
{
    if (!b_builded)
        return;
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
    {
        (*i)->destroy();
    }
    b_builded = false;
}

void CPHGeometryOwner::set_body(CharacterVirtualHandle body)
{
    m_native_body_handle = body;
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        { (*i)->owner = this; (*i)->set_body(body); }
}

void CPHGeometryOwner::RebuildNativeShape()
{
    ++m_native_shape_revision;
    if (m_native_body_handle == INVALID_BODY_HANDLE) return;
    if (m_geoms.empty()) {
        GetPhysicsCore()->SetBodyObjectLayer(m_native_body_handle, 4);
        return;
    }
    xr_vector<PhysicsShapeHandle> shapes;
    xr_vector<Fmatrix> transforms;
    for (auto* geom : m_geoms) {
        geom->init();
        geom->owner = this;
        geom->element_position() = static_cast<u16>(shapes.size());
        shapes.push_back(geom->geometry());
        Fmatrix transform;
        geom->get_local_form_bt(transform);
        transforms.push_back(transform);
    }
    auto* core = GetPhysicsCore();
    const auto shape = core->CreateCompoundShape(shapes.data(), transforms.data(), shapes.size());
    R_ASSERT(shape);
    core->SetBodyShape(m_native_body_handle, shape);
    core->DestroyCDBModel(shape);
}

Fvector CPHGeometryOwner::get_mc_data()
{
    Fvector s;
    float pv;
    m_mass_center.set(0, 0, 0);
    m_volume = 0.f;
    GEOM_I i_geom = m_geoms.begin(), e = m_geoms.end();
    for (; i_geom != e; ++i_geom)
    {
        pv = (*i_geom)->volume();
        s.mul((*i_geom)->local_center(), pv);
        m_volume += pv;
        m_mass_center.add(s);
    }
    if (m_volume > EPS_L)
        m_mass_center.mul(1.f / m_volume);
    return m_mass_center;
}

Fvector CPHGeometryOwner::get_mc_geoms()
{
    Fvector mc;
    mc.set(0.f, 0.f, 0.f);
    return mc;
}

void CPHGeometryOwner::get_mc_kinematics(IKinematics* K, Fvector& mc, float& mass)
{
    mc.set(0.f, 0.f, 0.f);
    mass = 0.f;
    m_volume = 0.f;
    GEOM_I i_geom = m_geoms.begin(), e = m_geoms.end();
    for (; i_geom != e; ++i_geom)
    {
        const IBoneData& data = K->GetBoneData((*i_geom)->bone_id());
        Fvector add;
        mass += data.get_mass();
        m_volume += (*i_geom)->volume();
        add.set(data.get_center_of_mass());
        add.mul(data.get_mass());
        mc.add(add);
    }
    if (mass > EPS_L)
        mc.mul(1.f / mass);
}

void CPHGeometryOwner::calc_volume_data()
{
    m_volume = 0.f;
    GEOM_I i_geom = m_geoms.begin(), e = m_geoms.end();
    for (; i_geom != e; ++i_geom)
    {
        m_volume += (*i_geom)->volume();
    }
}

void CPHGeometryOwner::SetMaterial(u16 m)
{
    ul_material = m;
    if (!b_builded)
        return;
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->set_material(m);
}

void CPHGeometryOwner::SetPhObjectInGeomData(CPHObject* O)
{
    if (!b_builded)
        return;
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->set_ph_object(O);
}

void CPHGeometryOwner::add_Box(const Fobb& V)
{
    Fobb box = V;
    if (box.m_halfsize.x < 0.005f) box.m_halfsize.x = 0.005f;
    if (box.m_halfsize.y < 0.005f) box.m_halfsize.y = 0.005f;
    if (box.m_halfsize.z < 0.005f) box.m_halfsize.z = 0.005f;
    m_geoms.push_back(smart_cast<CPhysicsGeom*>(xr_new<CBoxGeom>(box)));
}

void CPHGeometryOwner::add_Sphere(const Fsphere& V)
{
    m_geoms.push_back(smart_cast<CPhysicsGeom*>(xr_new<CSphereGeom>(V)));
}

void CPHGeometryOwner::add_Cylinder(const Fcylinder& V)
{
    m_geoms.push_back(smart_cast<CPhysicsGeom*>(xr_new<CCylinderGeom>(V)));
}

void CPHGeometryOwner::add_Shape(const SBoneShape& shape, const Fmatrix& offset)
{
    switch (shape.type)
    {
    case SBoneShape::stBox:
    {
        Fobb box = shape.box;
        Fmatrix m;
        m.set(offset);
        box.transform(box, m);
        add_Box(box);
        break;
    }
    case SBoneShape::stSphere:
    {
        Fsphere sphere = shape.sphere;
        offset.transform_tiny(sphere.P);
        add_Sphere(sphere);
        break;
    }
    case SBoneShape::stCylinder:
    {
        Fcylinder C = shape.cylinder;
        offset.transform_tiny(C.m_center);
        offset.transform_dir(C.m_direction);
        add_Cylinder(C);
        break;
    }
    case SBoneShape::stNone: break;
    default: NODEFAULT;
    }
}

void CPHGeometryOwner::add_Shape(const SBoneShape& shape)
{
    switch (shape.type)
    {
    case SBoneShape::stBox: add_Box(shape.box); break;
    case SBoneShape::stSphere: add_Sphere(shape.sphere); break;
    case SBoneShape::stCylinder: add_Cylinder(shape.cylinder); break;
    case SBoneShape::stNone: break;
    default: NODEFAULT;
    }
}

void CPHGeometryOwner::set_ContactCallback(ObjectContactCallbackFun* callback)
{
    contact_callback = callback;
    if (!b_builded)
        return;
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->set_contact_cb(callback);
}

void CPHGeometryOwner::set_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    object_contact_callback = callback;
    if (!b_builded)
        return;
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->set_obj_contact_cb(callback);
}

void CPHGeometryOwner::add_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    if (!object_contact_callback)
    {
        object_contact_callback = callback;
    }
    if (!b_builded)
        return;

    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->add_obj_contact_cb(callback);
}

void CPHGeometryOwner::remove_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    if (object_contact_callback == callback)
    {
        object_contact_callback = nullptr;
    }
    if (!b_builded)
        return;

    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->remove_obj_contact_cb(callback);
}

ObjectContactCallbackFun* CPHGeometryOwner::get_ObjectContactCallback() { return object_contact_callback; }

void CPHGeometryOwner::set_CallbackData(void* cd)
{
    VERIFY(b_builded);
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->set_callback_data(cd);
}

void* CPHGeometryOwner::get_CallbackData()
{
    VERIFY(b_builded);
    return (*m_geoms.begin())->get_callback_data();
}

void CPHGeometryOwner::set_PhysicsRefObject(IPhysicsShellHolder* ref_object)
{
    m_phys_ref_object = ref_object;
    if (!b_builded)
        return;
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->set_ref_object(ref_object);
}

u16 CPHGeometryOwner::numberOfGeoms() const { return (u16)m_geoms.size(); }

void CPHGeometryOwner::get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const
{
    t_get_extensions(m_geoms, axis, center_prg, lo_ext, hi_ext);
}

void CPHGeometryOwner::get_MaxAreaDir(Fvector& dir)
{
    if (m_geoms.empty())
        return;
    (*m_geoms.begin())->get_max_area_dir_bt(dir);
}

float CPHGeometryOwner::getRadius()
{
    if (!m_geoms.empty())
        return m_geoms.back()->radius();
    else
        return 0.f;
}

void CPHGeometryOwner::get_mc_vs_transform(Fvector& mc, const Fmatrix& m)
{
    mc.set(m_mass_center);
    m.transform_tiny(mc);
    VERIFY2(_valid(mc), "invalid mc in_set_transform");
}

void CPHGeometryOwner::setStaticForm(const Fmatrix& form)
{
    if (!b_builded)
        return;
    Fmatrix f;
    f.set(form);
    get_mc_vs_transform(f.c, form);
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->set_static_ref_form(f);
}

void CPHGeometryOwner::setPosition(const Fvector& pos)
{
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
    {
        (*i)->set_build_position(pos);
    }
}

CPhysicsGeom* CPHGeometryOwner::GeomByBoneID(u16 bone_id)
{
    GEOM_I g = std::find_if(m_geoms.begin(), m_geoms.end(), [bone_id](CPhysicsGeom* geom)
    {
        return geom->bone_id() == bone_id;
    });

    if (g != m_geoms.end())
    {
        return *g;
    }
    return nullptr;
}

void CPHGeometryOwner::clear_cashed_tries()
{
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
    {
        (*i)->clear_cashed_tries();
    }
}

void CPHGeometryOwner::clear_motion_history(bool set_unspecified)
{
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
    {
        (*i)->clear_motion_history(set_unspecified);
    }
}

void CPHGeometryOwner::add_geom(CPhysicsGeom* g)
{
    VERIFY(b_builded);
    m_geoms.push_back(g);
}

void CPHGeometryOwner::remove_geom(CPhysicsGeom* g)
{
    VERIFY(b_builded);
    GEOM_I gi = std::find(m_geoms.begin(), m_geoms.end(), g);
    VERIFY(gi != m_geoms.end());
    m_geoms.erase(gi);
}

#ifdef DEBUG
void CPHGeometryOwner::dbg_draw(float scale, u32 color, Flags32 flags) const
{
    VERIFY(b_builded);
    GEOM_CI i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
    {
        (*i)->dbg_draw(scale, color, flags);
    }
}
#endif
