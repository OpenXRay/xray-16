#pragma once

#include "Geometry.h"
#include "xrPhysicsCore/IPhysicsCore.h"

using GEOM_STORAGE = xr_vector<CPhysicsGeom*>;
using GEOM_I = GEOM_STORAGE::iterator;
using GEOM_CI = GEOM_STORAGE::const_iterator;

struct SBoneShape;
class IKinematics;

class CPHGeometryOwner
{
protected:
    GEOM_STORAGE m_geoms;
    bool b_builded;
    BodyHandle m_native_body_handle = INVALID_BODY_HANDLE;
    u32 m_native_shape_revision = 0;

protected:
    Fvector m_mass_center;
    IPhysicsShellHolder* m_phys_ref_object;
    float m_volume;
    u16 ul_material;
    ObjectContactCallbackFun* contact_callback;
    ObjectContactCallbackFun* object_contact_callback;

public:
    void add_Sphere(const Fsphere& V);
    void RebuildNativeShape();
    u32 NativeShapeRevision() const { return m_native_shape_revision; }
    void add_Box(const Fobb& V);
    void add_Cylinder(const Fcylinder& V);
    void add_Shape(const SBoneShape& shape);
    void add_Shape(const SBoneShape& shape, const Fmatrix& offset);
    CPhysicsGeom* last_geom()
    {
        if (m_geoms.empty())
            return nullptr;
        return m_geoms.back();
    }
    bool has_geoms() { return !m_geoms.empty(); }
    void add_geom(CPhysicsGeom* g);
    void remove_geom(CPhysicsGeom* g);

public:
    void set_ContactCallback(ObjectContactCallbackFun* callback);
    void set_ObjectContactCallback(ObjectContactCallbackFun* callback);
    void add_ObjectContactCallback(ObjectContactCallbackFun* callback);
    void remove_ObjectContactCallback(ObjectContactCallbackFun* callback);

    void set_CallbackData(void* cd);
    void* get_CallbackData();
    ObjectContactCallbackFun* get_ObjectContactCallback();
    void set_PhysicsRefObject(IPhysicsShellHolder* ref_object);
    IPhysicsShellHolder* PhysicsRefObject() { return m_phys_ref_object; }
    void SetPhObjectInGeomData(CPHObject* O);
#ifdef DEBUG
    void dbg_draw(float scale, u32 color, Flags32 flags) const;
#endif
    void SetMaterial(u16 m);
    void SetMaterial(LPCSTR m) { SetMaterial(GMLib.GetMaterialIdx(m)); }
    IC u16 Material() const { return ul_material; }
    IC CPhysicsGeom* Geom(u16 num)
    {
        R_ASSERT2(num < m_geoms.size(), "out of range");
        return m_geoms[num];
    }
    IC const CPhysicsGeom* Geom(u16 num) const
    {
        R_ASSERT2(num < m_geoms.size(), "out of range");
        return m_geoms[num];
    }
    CPhysicsGeom* GeomByBoneID(u16 bone_id);
    u16 numberOfGeoms() const;

public:
    Fvector get_mc_data();
    Fvector get_mc_geoms();
    void get_mc_kinematics(IKinematics* K, Fvector& mc, float& mass);
    void calc_volume_data();
    const Fvector& local_mass_Center() { return m_mass_center; }
    float get_volume()
    {
        calc_volume_data();
        return m_volume;
    };
    void get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const;
    void get_MaxAreaDir(Fvector& dir);
    float getRadius();
    void setStaticForm(const Fmatrix& form);
    void setPosition(const Fvector& pos);
    void clear_cashed_tries();
    void clear_motion_history(bool set_unspecified);
    void get_mc_vs_transform(Fvector& mc, const Fmatrix& m);

protected:
    void build();
    void destroy();
    void build_Geom(CPhysicsGeom& V);
    void build_Geom(u16 i);
    void set_body(CharacterVirtualHandle body);

    CPHGeometryOwner();
    virtual ~CPHGeometryOwner();
};

template <typename geometry_type>
void t_get_extensions(
    const xr_vector<geometry_type*>& geoms, const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext)
{
    lo_ext = FLT_MAX;
    hi_ext = -FLT_MAX;
    auto i = geoms.begin(), e = geoms.end();
    for (; i != e; ++i)
    {
        float temp_lo_ext, temp_hi_ext;
        (*i)->get_Extensions(axis, center_prg, temp_lo_ext, temp_hi_ext);
        if (lo_ext > temp_lo_ext)
            lo_ext = temp_lo_ext;
        if (hi_ext < temp_hi_ext)
            hi_ext = temp_hi_ext;
    }
}
