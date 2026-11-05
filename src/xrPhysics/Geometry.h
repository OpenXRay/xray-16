#pragma once

#include "PhysicsCommon.h"
#include "xrEngine/IPhysicsGeometry.h"
#include "xrCore/_cylinder.h"
#include "xrCore/_sphere.h"
#include "xrPhysicsCore/IPhysicsCore.h"

class CGameObject;
class CPHObject;
class IPhysicsShellHolder;
class CPHGeometryOwner;

class XRPHYSICS_API CPhysicsGeom : public IPhysicsGeometry
{
public:
    PhysicsShapeHandle m_geom_shape;
    u16 m_bone_id;
    u16 m_element_position;
    Flags16 m_flags;

    CharacterVirtualHandle m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;
    IPhysicsShellHolder* ph_ref_object = nullptr;

    CPHObject* ph_object = nullptr;
    void* callback_data = nullptr;
    Fvector reference_center = {0, 0, 0};
    CPHGeometryOwner* owner = nullptr;
    Fmatrix runtime_local = Fidentity;
    bool has_runtime_local = false;
    float native_mass = 0;
    Fvector native_mass_center = {0, 0, 0};
    virtual float volume() = 0;
    virtual void get_mass(float& m) = 0;
    void get_mass(float& m, const Fvector& ref_point, float density);
    void get_mass(float& m, const Fvector& ref_point);
    void add_self_mass(float& m, const Fvector& ref_point);
    void add_self_mass(float& m, const Fvector& ref_point, float density);

    void get_local_center_bt(Fvector& center);
    void get_global_center_bt(Fvector& center);
    void get_local_form_bt(Fmatrix& form);
    virtual void get_xform(Fmatrix& form) const;

    class CObjectContactCallback* object_callbacks = nullptr;
    ObjectContactCallbackFun* contact_callback = nullptr;
    Fvector last_pos = {-INFINITY, -INFINITY, -INFINITY};
    Fvector last_aabb_size = {0, 0, 0};
    Fvector last_aabb_pos = {0, 0, 0};
    bool pushing_neg = false;
    bool pushing_b_neg = false;
    bool b_static_colide = true;

    u16 material = 0;
    u16 tri_material = 0;
    u32 contact_triangle = u32(-1);
    const NativePhysicsContact* contact_response = nullptr;

#ifdef DEBUG
    virtual void dbg_draw(float scale, u32 color, Flags32 flags) const;
#endif

    virtual void get_Box(Fmatrix& form, Fvector& sz) const;
    virtual bool collide_fluids() const;
    void set_static_ref_form(const Fmatrix& form);
    virtual void get_max_area_dir_bt(Fvector& dir) = 0;
    virtual float radius() = 0;
    virtual void get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const = 0;

    void clear_cashed_tries();

    IC PhysicsShapeHandle geometry() { return m_geom_shape; }
    IC const PhysicsShapeHandle geometry() const { return m_geom_shape; }

    virtual const Fvector& local_center() = 0;
    virtual void get_local_form(Fmatrix& form) = 0;
    virtual void set_local_form(const Fmatrix& form) = 0;
    void set_local_form_bt(const Fmatrix& xform);

    // set
    void set_body(CharacterVirtualHandle body);
    CharacterVirtualHandle get_body() const { return m_char_handle; }
    void set_bone_id(u16 id) { m_bone_id = id; }
    u16 bone_id() { return m_bone_id; }

    IC u16& element_position() { return m_element_position; }

    void set_shape_flags(const Flags16& _flags) { m_flags = _flags; }

    void add_to_space(void* space) {};
    void remove_from_space(void* space) {};

    void set_material(u16 ul_material);
    void set_contact_cb(ObjectContactCallbackFun* ccb);
    void set_obj_contact_cb(ObjectContactCallbackFun* occb);
    void add_obj_contact_cb(ObjectContactCallbackFun* occb);
    void remove_obj_contact_cb(ObjectContactCallbackFun* occb);
    void set_callback_data(void* cd);
    void* get_callback_data();
    void set_ref_object(IPhysicsShellHolder* ro);
    void set_ph_object(CPHObject* o);

protected:
    virtual PhysicsShapeHandle create() = 0;

public:
    void init();
    void build(const Fvector& ref_point);
    virtual void set_build_position(const Fvector& ref_point);
    void clear_motion_history(bool set_unspecified);
    void move_local_basis(const Fmatrix& inv_new_mul_old);
    void destroy();

    CPhysicsGeom();
    virtual ~CPhysicsGeom();
};

class CBoxGeom : public CPhysicsGeom
{
    typedef CPhysicsGeom inherited;
    Fobb m_box;

public:
    CBoxGeom(const Fobb& box);

    virtual float volume();
    virtual float radius();
    virtual void get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const;
    virtual void get_max_area_dir_bt(Fvector& dir);
    virtual void get_mass(float& m);
    virtual const Fvector& local_center();
    virtual void get_local_form(Fmatrix& form);
    virtual void set_local_form(const Fmatrix& form);
    virtual PhysicsShapeHandle create();
    virtual void set_build_position(const Fvector& ref_point);
    void set_size(const Fvector& half_size);
    void get_size(Fvector& half_size) const;

private:
#ifdef DEBUG
    virtual void dbg_draw(float scale, u32 color, Flags32 flags) const;
#endif
};

class CSphereGeom : public CPhysicsGeom
{
    typedef CPhysicsGeom inherited;
    Fsphere m_sphere;

public:
    CSphereGeom(const Fsphere& sphere);

    virtual float volume();
    virtual float radius();
    virtual void get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const;
    virtual void get_max_area_dir_bt(Fvector& dir){};
    virtual void get_mass(float& m);
    virtual const Fvector& local_center();
    virtual void get_local_form(Fmatrix& form);
    virtual void set_local_form(const Fmatrix& form);
    virtual PhysicsShapeHandle create();
    virtual void set_build_position(const Fvector& ref_point);

private:
#ifdef DEBUG
    virtual void dbg_draw(float scale, u32 color, Flags32 flags) const;
#endif
};

class CCylinderGeom : public CPhysicsGeom
{
    typedef CPhysicsGeom inherited;
    Fcylinder m_cylinder;

public:
    CCylinderGeom(const Fcylinder& cyl);

    virtual float volume();
    virtual float radius();
    virtual void get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const;
    virtual void get_max_area_dir_bt(Fvector& dir){};
    virtual const Fvector& local_center();
    virtual void get_mass(float& m);
    virtual void get_local_form(Fmatrix& form);
    virtual void set_local_form(const Fmatrix& form);
    virtual PhysicsShapeHandle create();
    virtual void set_build_position(const Fvector& ref_point);
    void set_radius(float r);

private:
#ifdef DEBUG
    virtual void dbg_draw(float scale, u32 color, Flags32 flags) const;
#endif
};
