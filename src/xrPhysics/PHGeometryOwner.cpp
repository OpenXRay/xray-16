#include "stdafx.h"
#include "PHGeometryOwner.h"
#include "PHWorld.h"

#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"

CPHGeometryOwner::CPHGeometryOwner()
{
    b_builded = false;
    m_mass_center.set(0, 0, 0);
    VERIFY(ph_world);
    // contact_callback=ContactShotMark;//ph_world->default_contact_shotmark();
    contact_callback = ph_world->default_contact_shotmark();
    object_contact_callback = NULL;
    ul_material = GMLibrary().GetMaterialIdx("objects\\small_box");
    m_group = NULL;
    m_phys_ref_object = NULL;
}

CPHGeometryOwner::~CPHGeometryOwner()
{
    GEOM_I i_geom = m_geoms.begin(), e = m_geoms.end();
    for (; i_geom != e; ++i_geom)
        xr_delete(*i_geom);
    m_geoms.clear();
    DestroyGroupSpace();
    // if( b_builded )
    //{
    // VERIFY( m_group );
    // if( m_group )
    //	dSpaceDestroy( m_group );

    //}
}
void CPHGeometryOwner::group_add(CODEGeom& g)
{
    if (!m_group)
    {
        CreateGroupSpace();
    }
    VERIFY(m_group);
    {
        g.add_to_space((dSpaceID)m_group);
    }
}

void CPHGeometryOwner::group_remove(CODEGeom& g)
{
    VERIFY(m_group);
    g.remove_from_space(m_group);
    if (dSpaceGetNumGeoms(m_group) == 0)
        DestroyGroupSpace();
}

void CPHGeometryOwner::build_Geom(CODEGeom& geom)
{
    geom.build(m_mass_center);
    // geom.set_body(m_body);
    geom.set_material(ul_material);
    if (contact_callback)
        geom.set_contact_cb(contact_callback);
    if (object_contact_callback)
        geom.set_obj_contact_cb(object_contact_callback);
    if (m_phys_ref_object)
        geom.set_ref_object(m_phys_ref_object);
    // VERIFY( m_group );
    group_add(geom);
}

void CPHGeometryOwner::build_Geom(u16 i)
{
    CODEGeom& geom = *m_geoms[i];
    build_Geom(geom);
    geom.element_position() = i;
}

void CPHGeometryOwner::build()
{
    if (b_builded)
        return;
    // if(m_geoms.size()>1)//add/remove geom issues
    // VERIFY(!m_group);

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
void CPHGeometryOwner::set_body(dBodyID body)
{
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
        (*i)->set_body(body);
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
    m_mass_center.mul(1.f / m_volume);
    return m_mass_center;
}

Fvector CPHGeometryOwner::get_mc_geoms()
{
    ////////////////////to be implemented
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

dGeomID CPHGeometryOwner::dSpacedGeometry()
{
    if (!b_builded)
        return 0;
    VERIFY(m_group);
    // if(m_group)
    return (dGeomID)group_space(); //(dGeomID)m_group;
    // else return (*m_geoms.begin())->geometry_transform();
}

void CPHGeometryOwner::add_Box(const Fobb& V)
{
    Fobb box;
    box = V;
    if (box.m_halfsize.x < 0.005f)
        box.m_halfsize.x = 0.005f;
    if (box.m_halfsize.y < 0.005f)
        box.m_halfsize.y = 0.005f;
    if (box.m_halfsize.z < 0.005f)
        box.m_halfsize.z = 0.005f;
    m_geoms.push_back(smart_cast<CODEGeom*>(new CBoxGeom(box)));
}

void CPHGeometryOwner::add_Sphere(const Fsphere& V) { m_geoms.push_back(smart_cast<CODEGeom*>(new CSphereGeom(V))); }
void CPHGeometryOwner::add_Cylinder(const Fcylinder& V)
{
    m_geoms.push_back(smart_cast<CODEGeom*>(new CCylinderGeom(V)));
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
        // Fmatrix position;
        // position.set(box.m_rotate);
        // position.c.set(box.m_translate);
        // position.mulA(offset);
        // box.m_rotate.set(position);
        // box.m_translate.set(position.c);
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
    case SBoneShape::stBox:
    {
        add_Box(shape.box);
        break;
    }
    case SBoneShape::stSphere:
    {
        add_Sphere(shape.sphere);
        break;
    }

    case SBoneShape::stCylinder:
    {
        add_Cylinder(shape.cylinder);
        break;
    }

    case SBoneShape::stNone: break;
    default: NODEFAULT;
    }
}

void CPHGeometryOwner::set_ContactCallback(ContactCallbackFun* callback)
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
    {
        GEOM_I i = m_geoms.begin(), e = m_geoms.end();
        for (; i != e; ++i)
            (*i)->add_obj_contact_cb(callback);
    }
}

void CPHGeometryOwner::remove_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    if (object_contact_callback == callback)
    {
        object_contact_callback = NULL;
    }
    if (!b_builded)
        return;
    {
        GEOM_I i = m_geoms.begin(), e = m_geoms.end();
        for (; i != e; ++i)
            (*i)->remove_obj_contact_cb(callback);
    }
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
    /*
    lo_ext=dInfinity;hi_ext=-dInfinity;
    GEOM_CI i=m_geoms.begin(),e=m_geoms.end();
    for(;i!=e;++i)
    {
        float temp_lo_ext,temp_hi_ext;
        (*i)->get_Extensions(axis,center_prg,temp_lo_ext,temp_hi_ext);
        if(lo_ext>temp_lo_ext)lo_ext=temp_lo_ext;
        if(hi_ext<temp_hi_ext)hi_ext=temp_hi_ext;
    }
    */
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
void CPHGeometryOwner::CreateGroupSpace()
{
    VERIFY(!m_group);
    m_group = dSimpleSpaceCreate(0);
    dSpaceSetCleanup(m_group, 0);
}
void CPHGeometryOwner::DestroyGroupSpace()
{
    if (m_group)
    {
        dGeomDestroy((dGeomID)m_group);
        m_group = NULL;
    }
}
struct SFindPred
{
    u16 m_val;
    SFindPred(u16 val) { m_val = val; }
    bool operator()(CODEGeom* g) { return g->bone_id() == m_val; }
};
CODEGeom* CPHGeometryOwner::GeomByBoneID(u16 bone_id)
{
    GEOM_I g = std::find_if(m_geoms.begin(), m_geoms.end(), SFindPred(bone_id));
    if (g != m_geoms.end())
    {
        return *g;
    }
    else
    {
        return NULL;
    }
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

void CPHGeometryOwner::add_geom(CODEGeom* g)
{
    VERIFY(b_builded);
    VERIFY(m_group);
    m_geoms.push_back(g);
    group_add(*g);
    // g->add_to_space( m_group );
}

void CPHGeometryOwner::remove_geom(CODEGeom* g)
{
    VERIFY(b_builded);
    VERIFY(m_group);
    GEOM_I gi = std::find(m_geoms.begin(), m_geoms.end(), g);
    VERIFY(gi != m_geoms.end());
    //(*gi)->remove_from_space( m_group );
    group_remove(*g);
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
