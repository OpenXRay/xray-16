#pragma once

#include "Geometry.h"
#include "xrEngine/GameMtlLib.h"

using GEOM_STORAGE = xr_vector<CODEGeom*>;
using GEOM_I = GEOM_STORAGE::iterator;
using GEOM_CI = GEOM_STORAGE::const_iterator;

struct SBoneShape;
class IKinematics;

class CPHGeometryOwner
{
protected:
    GEOM_STORAGE m_geoms; // e
    // bl
    bool b_builded;

private:
    dSpaceID m_group; // e					//bl
protected:
    Fvector m_mass_center; // e ??				//bl
    IPhysicsShellHolder* m_phys_ref_object; //->to shell ??		//bl
    float m_volume; // e ??				//bl
    u16 ul_material; // e ??				//bl
    ContactCallbackFun* contact_callback; //->to shell ??		//bt
    ObjectContactCallbackFun* object_contact_callback; //->to shell ??		//st
public:
    ///
    void add_Sphere(const Fsphere& V); // aux
    void add_Box(const Fobb& V); // aux
    void add_Cylinder(const Fcylinder& V); // aux
    void add_Shape(const SBoneShape& shape); // aux
    void add_Shape(const SBoneShape& shape, const Fmatrix& offset); // aux
    CODEGeom* last_geom()
    {
        if (m_geoms.empty())
            return NULL;
        return m_geoms.back();
    } // aux
    bool has_geoms() { return !m_geoms.empty(); }
    void add_geom(CODEGeom* g);
    void remove_geom(CODEGeom* g);

protected:
    void group_add(CODEGeom& g);
    void group_remove(CODEGeom& g);

public:
    void set_ContactCallback(ContactCallbackFun* callback); // aux (may not be)
    void set_ObjectContactCallback(ObjectContactCallbackFun* callback); // called anywhere ph state influent
    void add_ObjectContactCallback(ObjectContactCallbackFun* callback); // called anywhere ph state influent
    void remove_ObjectContactCallback(ObjectContactCallbackFun* callback); // called anywhere ph state influent
    void set_CallbackData(void* cd);
    void* get_CallbackData();
    ObjectContactCallbackFun* get_ObjectContactCallback();
    void set_PhysicsRefObject(IPhysicsShellHolder* ref_object); // aux
    IPhysicsShellHolder* PhysicsRefObject() { return m_phys_ref_object; } // aux
    void SetPhObjectInGeomData(CPHObject* O);
#ifdef DEBUG
    void dbg_draw(float scale, u32 color, Flags32 flags) const;
#endif
    void SetMaterial(u16 m);
    void SetMaterial(LPCSTR m) { SetMaterial(GMLibrary().GetMaterialIdx(m)); } // aux
    IC CODEGeom* Geom(u16 num)
    {
        R_ASSERT2(num < m_geoms.size(), "out of range");
        return m_geoms[num];
    }
    IC const CODEGeom* Geom(u16 num) const
    {
        R_ASSERT2(num < m_geoms.size(), "out of range");
        return m_geoms[num];
    }
    CODEGeom* GeomByBoneID(u16 bone_id);
    u16 numberOfGeoms() const; // aux
    dGeomID dSpacedGeometry();

protected:
    IC dSpaceID group_space() { return m_group; }
public:
    Fvector get_mc_data(); // aux
    Fvector get_mc_geoms(); // aux
    void get_mc_kinematics(IKinematics* K, Fvector& mc, float& mass);
    void calc_volume_data(); // aux
    const Fvector& local_mass_Center() { return m_mass_center; } // aux
    float get_volume()
    {
        calc_volume_data();
        return m_volume;
    }; // aux
    void get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const; // aux
    void get_MaxAreaDir(Fvector& dir);
    float getRadius();
    void setStaticForm(const Fmatrix& form);
    void setPosition(const Fvector& pos);
    void clear_cashed_tries();
    void clear_motion_history(bool set_unspecified);
    void get_mc_vs_transform(Fvector& mc, const Fmatrix& m);

protected:
    void build();
    void CreateGroupSpace();
    void DestroyGroupSpace();
    void destroy();
    void build_Geom(CODEGeom& V); // aux
    void build_Geom(u16 i);
    void set_body(dBodyID body);

    CPHGeometryOwner();
    virtual ~CPHGeometryOwner();

private:
};

template <typename geometry_type>
void t_get_extensions(
    const xr_vector<geometry_type*>& geoms, const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext)
{
    lo_ext = dInfinity;
    hi_ext = -dInfinity;
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
