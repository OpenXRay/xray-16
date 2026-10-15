#pragma once

#include "Geometry.h"
#include "PHDefs.h"
#include "PhysicsCommon.h"
#include "PHDisabling.h"
#include "PHGeometryOwner.h"
#include "PHInterpolation.h"
#include "PHFracture.h"
#include "xrPhysicsCore/IPhysicsCore.h"

#include "xrServerEntities/PHSynchronize.h"

struct SPHImpact;
class CPHShell;
class CPHFracturesHolder;

class CPHElement : public CPhysicsElement,
                   public CPHSynchronize,
                   public CPHDisablingFull,
                   public CPHGeometryOwner
{
    friend class CPHFracturesHolder;

    float m_mass;
    PhysicsMassProperties m_native_mass;
    bool m_custom_inertia = false;
    bool m_bone_masses = false;
    CharacterVirtualHandle m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;

    float m_l_scale;
    float m_w_scale;

    CPHElement* m_parent_element;
    CPHShell* m_shell;
    CPHInterpolation m_char_handle_interpolation;

    float m_w_limit;
    float m_l_limit;

    float k_w;
    float k_l;

    Flags8 m_flags;
    enum
    {
        flActive = 1 << 0,
        flActivating = 1 << 1,
        flUpdate = 1 << 2,
        flWasEnabledBeforeFreeze = 1 << 3,
        flEnabledOnStep = 1 << 4,
        flFixed = 1 << 5,
        flAnimated = 1 << 6
    };
public:
    bool WantsSleep() const { return m_disabled; }
    CPHFracturesHolder* m_fratures_holder;
    PhysicsMassProperties NativeMassProperties();
    PhysicsMassProperties CalculateMassProperties(u16 from, u16 to, bool inside, Fvector& center);
    void SetNativeMassProperties(const PhysicsMassProperties& properties);
private:
    ////////////////////////////////////////////Interpolation/////////////////////////////////////////////////////////////////////////////////////
    void FillInterpolation()
    {
        m_char_handle_interpolation.ResetPositions();
        m_char_handle_interpolation.ResetRotations();
        m_flags.set(flUpdate, TRUE);
    }
    IC void UpdateInterpolation()
    {
        m_char_handle_interpolation.UpdatePositions();
        m_char_handle_interpolation.UpdateRotations();
        m_flags.set(flUpdate, TRUE);
    }

public:
    ////////////////////////////////////////////////Geometry/////////////////////////////////////////////////////////////////////////////////////////////////
    virtual void add_Sphere(const Fsphere& V);
    virtual void add_Box(const Fobb& V);
    virtual void add_Cylinder(const Fcylinder& V);
    virtual void add_Shape(const SBoneShape& shape);
    virtual void add_Shape(const SBoneShape& shape, const Fmatrix& offset);
    virtual CPhysicsGeom* last_geom() { return CPHGeometryOwner::last_geom(); }
    virtual CPhysicsGeom* geometry(u16 i) { return CPHGeometryOwner::Geom(i); }
    virtual const IPhysicsGeometry* geometry(u16 i) const { return CPHGeometryOwner::Geom(i); };
    virtual void add_geom(CPhysicsGeom* g);
    virtual void remove_geom(CPhysicsGeom* g);
    virtual bool has_geoms() { return CPHGeometryOwner::has_geoms(); }
    virtual void set_ContactCallback(ObjectContactCallbackFun* callback);
    virtual void set_ObjectContactCallback(ObjectContactCallbackFun* callback);
    virtual void add_ObjectContactCallback(ObjectContactCallbackFun* callback);
    virtual void remove_ObjectContactCallback(ObjectContactCallbackFun* callback);
    virtual void set_CallbackData(void* cd);
    virtual void* get_CallbackData();
    virtual ObjectContactCallbackFun* get_ObjectContactCallback();
    virtual void set_PhysicsRefObject(IPhysicsShellHolder* ref_object);
    virtual IPhysicsShellHolder* PhysicsRefObject() { return m_phys_ref_object; }
    CPHShell* ph_shell() { return m_shell; }
    virtual void SetMaterial(u16 m);
    virtual void SetMaterial(LPCSTR m) { CPHGeometryOwner::SetMaterial(m); }
    virtual u16 numberOfGeoms() const;
    virtual const Fvector& local_mass_Center() { return CPHGeometryOwner::local_mass_Center(); }
    virtual float getVolume() { return CPHGeometryOwner::get_volume(); }
    virtual void get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const;
    virtual void get_MaxAreaDir(Fvector& dir) { CPHGeometryOwner::get_MaxAreaDir(dir); }
    virtual float getRadius();
    virtual void GetPointVel(Fvector& res_vel, const Fvector& point) const;
    void SetCollideWithStatics(bool collide);
    ////////////////////////////////////////////////////Mass/////////////////////////////////////////////////////////////////////////////////////////////////
private:
    void calculate_it_data(const Fvector& mc, float mass);
    void calculate_it_data_use_density(const Fvector& mc, float density);
    void calc_it_fract_data_use_density(const Fvector& mc, float density);

    // Заменили dMass на float
public:
    virtual const Fvector& mass_Center() const;
    virtual void setDensity(float M);
    virtual float getDensity() { return m_mass / m_volume; }
    virtual void setMassMC(float M, const Fvector& mass_center);
    virtual void setDensityMC(float M, const Fvector& mass_center);
    virtual void set_local_mass_center(const Fvector& mc);
    virtual void setInertia(float M);
    virtual void addInertia(float M);
    virtual void add_Mass(const SBoneShape& shape, const Fmatrix& offset, const Fvector& mass_center, float mass, CPHFracture* fracture = NULL);
    virtual void set_BoxMass(const Fobb& box, float mass);
    virtual void setMass(float M);
    virtual float getMass() { return m_mass; }
    virtual PhysicsMassProperties getMassTensor();
    void CutVelocity(float l_limit, float a_limit);

    //////////////////////////////////////////////Disable/////////////////////////////////////////////////////////////////////////////////////////////////
    virtual void Disable();
    virtual void ReEnable();
    void Enable();

    // Заменили dBodyIsEnabled на запросы к Jolt
    virtual bool isEnabled() const { return isActive() && (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE && GetPhysicsCore()->IsBodyActive(m_char_handle)); }
    virtual bool isFullActive() const { return isActive() && !m_flags.test(flActivating); }
    virtual bool isActive() const { return !!m_flags.test(flActive); }
    virtual void Freeze();
    virtual void UnFreeze();
    virtual bool EnabledStateOnStep() { return (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE && GetPhysicsCore()->IsBodyActive(m_char_handle)) || m_flags.test(flEnabledOnStep); }

    ////////////////////////////////////////////////Updates///////////////////////////////////////////////////////////////////////////////////////////////
    bool AnimToVel(float dt, float l_limit, float a_limit);
    void BoneGlPos(Fmatrix& m, const Fmatrix& BoneTransform) const;
    void ToBonePos(const CBoneInstance* B, motion_history_state history_state);
    void ToBonePos(const Fmatrix& BoneTransform, motion_history_state history_state);
    IC void ActivatingPos(const Fmatrix& BoneTransform);
    IC void CalculateBoneTransform(Fmatrix& bone_transform) const;

#ifdef DEBUG
    virtual void dbg_draw_velocity(float scale, u32 color);
    virtual void dbg_draw_force(float scale, u32 color);
    virtual void dbg_draw_geometry(float scale, u32 color, Flags32 flags = Flags32().assign(0)) const;
#endif
    void SetBoneCallbackOverwrite(bool v);
    void BonesCallBack(CBoneInstance* B);
    void StataticRootBonesCallBack(CBoneInstance* B);
    void PhDataUpdate(float step);
    void PhTune(float step);
    virtual void Update();

    //////////////////////////////////////////////////Dynamics////////////////////////////////////////////////////////////////////////////////////////////
    virtual void SetAirResistance(float linear = default_k_l, float angular = default_k_w)
    {
        k_w = angular;
        k_l = linear;
    }
    virtual void GetAirResistance(float& linear, float& angular)
    {
        linear = k_l;
        angular = k_w;
    }
    virtual void applyImpact(const SPHImpact& impact);
    virtual void applyImpulseTrace(const Fvector& pos, const Fvector& dir, float val, const u16 id);
    virtual void set_DisableParams(const SAllDDOParams& params);
    virtual void set_DynamicLimits(float l_limit = default_l_limit, float w_limit = default_w_limit);
    virtual void set_DynamicScales(float l_scale = default_l_scale, float w_scale = default_w_scale);
    virtual void Fix();
    virtual void SetAnimated(bool v);
    virtual void ReleaseFixed();
    virtual bool isFixed() { return !!(m_flags.test(flFixed)); }
    virtual void applyForce(const Fvector& dir, float val);
    virtual void applyForce(float x, float y, float z);
    virtual void applyImpulse(const Fvector& dir, float val);
    virtual void applyImpulseVsMC(const Fvector& pos, const Fvector& dir, float val);
    virtual void applyImpulseVsGF(const Fvector& pos, const Fvector& dir, float val);
    virtual void applyGravityAccel(const Fvector& accel);
    virtual void getForce(Fvector& force);
    virtual void getTorque(Fvector& torque);
    virtual void get_LinearVel(Fvector& velocity) const;
    virtual void get_AngularVel(Fvector& velocity) const;
    virtual void set_LinearVel(const Fvector& velocity);
    virtual void set_AngularVel(const Fvector& velocity);
    virtual void setForce(const Fvector& force);
    virtual void setTorque(const Fvector& torque);
    virtual void set_ApplyByGravity(bool flag);
    virtual bool get_ApplyByGravity();

    ///////////////////////////////////////////////////Net////////////////////////////////////////////////////////////////////////////////////////
    virtual void get_State(SPHNetState& state);
    virtual void set_State(const SPHNetState& state);
    virtual void net_Import(NET_Packet& P);
    virtual void net_Export(NET_Packet& P);

    ///////////////////////////////////////////////////Position///////////////////////////////////////////////////////////////////////////////////
    virtual void SetTransform(const Fmatrix& m0, motion_history_state history_state);
    virtual void TransformPosition(const Fmatrix& form, motion_history_state history_state);
    virtual void getQuaternion(Fquaternion& quaternion);
    virtual void setQuaternion(const Fquaternion& quaternion);
    virtual void SetGlobalPositionDynamic(const Fvector& position);
    virtual void GetGlobalPositionDynamic(Fvector* v);
    virtual void cv2obj_Xfrom(const Fquaternion& q, const Fvector& pos, Fmatrix& xform);
    virtual void cv2bone_Xfrom(const Fquaternion& q, const Fvector& pos, Fmatrix& xform);
    virtual void InterpolateGlobalTransform(Fmatrix* m);
    virtual void InterpolateGlobalPosition(Fvector* v);
    virtual void GetGlobalTransformDynamic(Fmatrix* m) const;
    IC void InverceLocalForm(Fmatrix&);
    IC void MulB43InverceLocalForm(Fmatrix&) const;

    ////////////////////////////////////////////////////Structure/////////////////////////////////////////////////////////////////////////////////
    virtual CPhysicsShell* PhysicsShell();
    CPHShell* PHShell();
    virtual void set_ParentElement(CPhysicsElement* p) { m_parent_element = (CPHElement*)p; }
#ifdef DEBUG
    CPhysicsElement* parent_element() { return m_parent_element; }
#endif
    void SetShell(CPHShell* p);
    virtual CharacterVirtualHandle get_body() { return m_char_handle; }
    virtual const CharacterVirtualHandle get_bodyConst() const { return m_char_handle; }

    //////////////////////////////////////////////////////Breakable//////////////////////////////////////////////////////////////////////////////////
    IC CPHFracturesHolder* FracturesHolder() { return m_fratures_holder; }
    IC const CPHFracturesHolder* constFracturesHolder() const { return m_fratures_holder; }
    void DeleteFracturesHolder();
    virtual bool isBreakable();
    virtual u16 setGeomFracturable(CPHFracture& fracture);
    virtual CPHFracture& Fracture(u16 num);
    void SplitProcess(ELEMENT_PAIR_VECTOR& new_elements);
    void PassEndGeoms(u16 from, u16 to, CPHElement* dest);

    ////////////////////////////////////////////////////Build/Activate////////////////////////////////////////////////////////////////////////////
    virtual void Activate(const Fmatrix& m0, float dt01, const Fmatrix& m2, bool disable = false);
    virtual void Activate(const Fmatrix& transform, const Fvector& lin_vel, const Fvector& ang_vel, bool disable = false);
    virtual void Activate(bool disable = false, bool not_set_bone_callbacks = false);
    virtual void Activate(const Fmatrix& start_from, bool disable = false);
    virtual void Deactivate();
    void SetBoneCallback();
    void ClearBoneCallback();
    void CreateSimulBase();
    void PresetActive();
    void build();
    void build(bool disable);
    void destroy();
    void Start();
    void RunSimulation();
    void RunSimulation(const Fmatrix& start_from);
    void ClearDestroyInfo();
    void GetAnimBonePos(Fmatrix& bp);
    CPHElement();
    virtual ~CPHElement();
};

IC CPHElement* cast_PHElement(CPhysicsElement* e) { return static_cast<CPHElement*>(static_cast<CPhysicsElement*>(e)); }
IC CPHElement* cast_PHElement(void* e) { return static_cast<CPHElement*>(static_cast<CPhysicsElement*>(e)); }
IC CPhysicsElement* cast_PhysicsElement(CPHElement* e)
{
    return static_cast<CPhysicsElement*>(static_cast<CPHElement*>(e));
}
