#pragma once
#include "PHCharacter.h"
#include "Physics.h"
#include "MathUtils.h"
#include "ElevatorState.h"
#include "IColisiondamageInfo.h"
#include "xrCDB/xr_collide_defs.h"
#include "xrPhysicsCore/IPhysicsCore.h"

class CPhysicsGeom;

namespace ALife
{
enum EHitType : u32;
};
#ifdef DEBUG
#include "debug_output.h"
#endif
class ICollisionHitCallback;

class CPHSimpleCharacter : public CPHCharacter, ICollisionDamageInfo
{
    typedef CPHCharacter inherited;

private:
    collide::rq_results RQR;

protected:
    CElevatorState m_elevator_state;
////////////////////////////damage////////////////////////////////////////
#ifdef DEBUG
public:
#endif
    struct SCollisionDamageInfo
    {
        SCollisionDamageInfo();
        void Construct();
        float ContactVelocity() const;
        void HitDir(Fvector& dir) const;
        IC const Fvector& HitPos() const { return m_damage_pos; }
        void Reinit();

        Fvector m_damage_pos;
        Fvector m_damage_normal;

        ICollisionHitCallback* m_hit_callback;
        u16 m_obj_id;
        float m_dmc_signum;
        enum
        {
            ctStatic,
            ctObject
        } m_dmc_type;
        ALife::EHitType m_hit_type;
        bool is_initiated;
        mutable float m_contact_velocity;
    };
#ifdef DEBUG
    SCollisionDamageInfo& dbg_get_collision_dmg_info() { return m_collision_damage_info; }
#endif
protected:
    SCollisionDamageInfo m_collision_damage_info;
    /////////////////////////// callback
    ObjectContactCallbackFun* m_object_contact_callback;
    xr_vector<ObjectContactCallbackFun*> m_native_callbacks;
    void* m_native_callback_data = nullptr;
    ////////////////////////// geometry
    Fvector m_last_move;

    CPhysicsGeom* m_geom_shell;
    CPhysicsGeom* m_wheel;
    CPhysicsGeom* m_hat;
    CPhysicsGeom* m_cap;

    float m_radius;
    float m_cyl_hight;
    ////////////////////////// movement
    Fvector m_control_force;
    Fvector m_acceleration;
    Fvector m_cam_dir;
    Fvector m_wall_contact_normal;
    Fvector m_ground_contact_normal;
    Fvector m_clamb_depart_position;
    Fvector m_depart_position;
    Fvector m_wall_contact_position;
    Fvector m_ground_contact_position;

    float jump_up_velocity;
    float m_collision_damage_factor;
    float m_max_velocity;
    float m_air_control_factor;

    Fvector m_jump_depart_position;
    Fvector m_death_position;
    Fvector m_jump_accel;

    Fvector m_last_environment_update;
    u16 m_last_picked_material;

    // movement state
    bool is_contact;
    bool was_contact;
    bool b_depart;
    bool b_meet;
    bool b_side_contact;
    bool b_was_side_contact;
    bool b_any_contacts;
    bool b_air_contact_state;

    bool b_valide_ground_contact;
    bool b_valide_wall_contact;
    bool b_on_object;
    bool b_was_on_object;
    bool b_on_ground;
    bool b_lose_ground;
    bool b_collision_restrictor_touch;
    u32 m_air_frames;
    u32 m_contact_count;

    bool is_control;
    bool b_meet_control;
    bool b_lose_control;
    bool was_control;
    bool b_stop_control;
    bool b_depart_control;
    bool b_jump;
    bool b_jumping;
    bool b_clamb_jump;
    bool b_external_impulse;

    u64 m_ext_impuls_stop_step;
    Fvector m_ext_imulse;
    bool b_death_pos;
    bool b_foot_mtl_check;
    float m_friction_factor;
    bool b_non_interactive;
    bool m_is_active;
    ObjectContactCallbackFun* m_static_contact_callback{ nullptr };
    ObjectContactCallbackFun* m_wheel_contact_callback{ nullptr };

public:
    CPHSimpleCharacter();
    virtual ~CPHSimpleCharacter() { Destroy(); }
    /////////////////CPHObject//////////////////////////////////////////////
    virtual void PhDataUpdate(float step);
    virtual void PhTune(float step);
    virtual void InitContact(bool& do_collide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_idx_1, u16 material_idx_2);
    virtual void get_spatial_params();
    /////////////////CPHCharacter////////////////////////////////////////////
public:
    virtual bool ContactWas()
    {
        if (b_meet_control)
        {
            b_meet_control = false;
            return true;
        }
        else
            return false;
    }
    virtual EEnvironment CheckInvironment();
    virtual void GroundNormal(Fvector& norm);
    virtual const ICollisionDamageInfo* CollisionDamageInfo() const { return this; }
    virtual ICollisionDamageInfo* CollisionDamageInfo() { return this; }
private:
    virtual float ContactVelocity() const { return m_collision_damage_info.ContactVelocity(); }
    virtual void HitDir(Fvector& dir) const { return m_collision_damage_info.HitDir(dir); }
    virtual const Fvector& HitPos() const { return m_collision_damage_info.HitPos(); }
    virtual u16 DamageInitiatorID() const;
    virtual IGameObject* DamageInitiator() const;
    virtual ALife::EHitType HitType() const { return m_collision_damage_info.m_hit_type; };
    virtual void SetInitiated();
    virtual bool IsInitiated() const;
    virtual bool GetAndResetInitiated();
    virtual void SetHitType(ALife::EHitType type) { m_collision_damage_info.m_hit_type = type; };
    virtual ICollisionHitCallback* HitCallback() const;
    virtual void Reinit() { m_collision_damage_info.Reinit(); };
public:
    virtual void Create(Fvector sizes);
    virtual void Destroy(void);
    virtual void Disable();
    virtual void EnableObject(CPHObject* obj);
    virtual void Enable();
    virtual void SetBox(const Fvector& sizes);
    virtual bool UpdateRestrictionType(CPHCharacter* ach);
    virtual void SetObjectContactCallback(ObjectContactCallbackFun* callback);
    virtual void SetObjectContactCallbackData(void* data);
    virtual void SetWheelContactCallback(ObjectContactCallbackFun* callback);

private:
    void RemoveObjectContactCallback(ObjectContactCallbackFun* callback);
    void AddObjectContactCallback(ObjectContactCallbackFun* callback);
    static void TestRestrictorContactCallbackFun(bool& do_colide, bool bo1, CPhysicsGeom* geom1, CPhysicsGeom* geom2, const Fvector& contact_normal, const Fvector& contact_pos, SGameMtl* material_1, SGameMtl* material_2);
    static void JoltCharacterContactCallback(void* char_user_data, const Fvector& contact_pos, const Fvector& contact_normal, const Fvector& contact_vel, u32 tri_user_data, BodyHandle other_body_handle, void* other_body_user_data, bool is_sensor);

public:
    virtual ObjectContactCallbackFun* ObjectContactCallBack();
    void DispatchNativeContacts(bool& collide, bool first, CPhysicsGeom* geometry1, CPhysicsGeom* geometry2,
        const Fvector& normal, const Fvector& position, SGameMtl* material1, SGameMtl* material2) override;
    void ProcessNativeContact(const NativePhysicsContact& contact) override;
    virtual void SetStaticContactCallBack(ObjectContactCallbackFun* calback);
    virtual void SwitchOFFInitContact();
    virtual void SwitchInInitContact();
    virtual void SetAcceleration(Fvector accel);
    virtual Fvector GetAcceleration() { return m_acceleration; };
    virtual void SetCamDir(const Fvector& cam_dir);
    virtual const Fvector& CamDir() const { return m_cam_dir; }
    virtual void SetMaterial(u16 material);
    virtual void SetPosition(const Fvector& pos);
    virtual void GetVelocity(Fvector& vvel) const;
    virtual void GetSmothedVelocity(Fvector& vvel);
    virtual void SetVelocity(Fvector vel);
    virtual void SetAirControlFactor(float factor) { m_air_control_factor = factor; }
    virtual void SetElevator(IClimableObject* climable) { m_elevator_state.SetElevator(climable); };
    virtual CElevatorState* ElevatorState();
    virtual void SetCollisionDamageFactor(float f) { m_collision_damage_factor = f; }
    virtual void GetPosition(Fvector& vpos);
    virtual void GetPreviousPosition(Fvector& pos);
    virtual float FootRadius();
    virtual void DeathPosition(Fvector& deathPos);
    virtual void IPosition(Fvector& pos);
    virtual u16 ContactBone();
    virtual void ApplyImpulse(const Fvector& dir, const float P);
    virtual void ApplyForce(const Fvector& force);
    virtual void ApplyForce(const Fvector& dir, float force);
    virtual void ApplyForce(float x, float y, float z);
    virtual void AddControlVel(const Fvector& vel);
    virtual void SetMaximumVelocity(float vel) { m_max_velocity = vel; }
    virtual float GetMaximumVelocity() { return m_max_velocity; }
    virtual void SetJupmUpVelocity(float velocity) { jump_up_velocity = velocity; }
    virtual bool JumpState() { return b_jumping || b_jump; };
    virtual const Fvector& ControlAccel() const { return m_acceleration; }
    virtual bool TouchRestrictor(ERestrictionType rttype);
    virtual float& FrictionFactor() { return m_friction_factor; }
    virtual void SetMas(float mass);
    virtual float Mass() { return m_mass; };
    virtual void SetPhysicsRefObject(IPhysicsShellHolder* ref_object);
    virtual void SetNonInteractive(bool v);
    virtual bool IsEnabled()
    {
        if (!b_exist)
            return false;
        return m_is_active;
    }
    virtual void GetBodyPosition(Fvector& vpos)
    {
        VERIFY(b_exist);
        GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, vpos);
    }
    const Fvector& BodyPosition() const
    {
        VERIFY(b_exist && m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE);
        static Fvector temp;
        GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, temp);
        return temp;
    }

    virtual void get_State(SPHNetState& state);
    virtual void set_State(const SPHNetState& state);
    virtual void ValidateWalkOn();
    bool ValidateWalkOnMesh();
    bool ValidateWalkOnObject();

private:
    void CheckCaptureJoint();
    void ApplyAcceleration();

    u16 RetriveContactBone();
    void SafeAndLimitVelocity();
    virtual void UpdateStaticDamage(const Fvector& normal, const Fvector& pos, SGameMtl* tri_material, bool bo1);
    void UpdateDynamicDamage(const Fvector& normal, const Fvector& pos, CharacterVirtualHandle b2, u16 obj_material_idx, bool bo1);
    IC void FootProcess(const Fvector& normal, const Fvector& pos, bool& do_collide, bool bo, CPhysicsGeom* g);
    IC void foot_material_update(u16 tri_material, u16 foot_material_idx);
    static void TestPathCallback(bool& do_colide, bool bo1, CPhysicsGeom* geom1, CPhysicsGeom* geom2, const Fvector& contact_normal, const Fvector& contact_pos, SGameMtl* material_1, SGameMtl* material_2);
    virtual void Collide();
    void OnStartCollidePhase();

private:
    virtual void Freeze() { CPHObject::Freeze(); }
    virtual void UnFreeze() { CPHObject::UnFreeze(); }
    virtual void step(float dt) { CPHObject::step(dt); }
    virtual void collision_disable() { CPHObject::collision_disable(); }
    virtual void collision_enable() { CPHObject::collision_enable(); }
    virtual void NetRelcase(IPhysicsShellHolder* O);

protected:
    virtual void get_Box(Fvector& sz, Fvector& c) const;
    virtual void update_last_material();

public:
#ifdef DEBUG
    virtual void OnRender();
#endif
};

const float def_spring_rate = 0.5f;
const float def_dumping_rate = 20.1f;

IC bool ignore_material(u16 material_idx)
{
    SGameMtl* material = GMLib.GetMaterialByIdx(material_idx);
    return !!material->Flags.test(SGameMtl::flActorObstacle);
}
