#pragma once

#include "PHObject.h"
#include "PhysicsExternalCommon.h"
#include "PHInterpolation.h"
#include "xrServerEntities/PHSynchronize.h"
#include "xrServerEntities/alife_space.h"
#include "PHDisabling.h"

#include "xrEngine/IPhysicsShell.h"
#include "xrPhysicsCore/IPhysicsCore.h"

class IPhysicsShellHolder;
class IClimableObject;
class CGameObject;
class ICollisionDamageInfo;
class CElevatorState;
class CPHActorCharacter;
class CPHAICharacter;
class CPhysicsGeom;

namespace ALife
{
enum EHitType : u32;
}
enum EEnvironment { peOnGround, peAtWall, peInAir };

class CPHCharacter :
    public CPHObject,
    public CPHSynchronize,
    public CPHDisablingTranslational,
    public IPhysicsElement
{
public:
    u64 m_creation_step;
    bool b_exist;

protected:
    CPHInterpolation m_char_handle_interpolation;
    CharacterVirtualHandle m_char_handle;
    IPhysicsShellHolder* m_phys_ref_object;

    float m_mass;
    Fvector m_force = Fvector().set(0, 0, 0);
    bool was_enabled_before_freeze;

    u16* p_lastMaterialIDX;
    u16 lastMaterialIDX;
    u16 injuriousMaterialIDX;

    Fvector m_safe_velocity;
    Fvector m_safe_position;
    float m_mean_y;
    mutable Fvector m_last_velocity_cache; // Для эмуляции mass_Center()

private:
#ifdef DEBUG
    virtual IPhysicsShellHolder* ref_object() { return PhysicsRefObject(); }
#endif

protected:
    ERestrictionType m_new_restriction_type;
    ERestrictionType m_restriction_type;
    bool b_actor_movable;

    bool b_in_touch_resrtrictor;
    float m_current_object_radius;

public:
    virtual ECastType CastType() { return CPHObject::tpCharacter; }
    virtual CPHActorCharacter* CastActorCharacter() { return NULL; }
    virtual CPHAICharacter* CastAICharacter() { return NULL; }
    ERestrictionType RestrictionType() { return m_restriction_type; }
    void SetNewRestrictionType(ERestrictionType rt) { m_new_restriction_type = rt; }
    void SetRestrictionType(ERestrictionType rt) { m_new_restriction_type = m_restriction_type = rt; }
    void SetObjectRadius(float R) { m_current_object_radius = R; }
    float ObjectRadius() { return m_current_object_radius; }
    virtual void ChooseRestrictionType(ERestrictionType my_type, CPHCharacter* ch) {}
    virtual bool UpdateRestrictionType(CPHCharacter* ach) = 0;
    virtual void FreezeContent();
    virtual void UnFreezeContent();
    virtual CharacterVirtualHandle get_body() { return m_char_handle; }
    virtual void fix_body_rotation();
    virtual void get_body_position(Fvector& p);
    virtual void UpdateL1() override;
    virtual void Disable();
    virtual void ReEnable() { ; }
    virtual void Enable();
    virtual void SwitchOFFInitContact() = 0;
    virtual void SwitchInInitContact() = 0;
    virtual bool IsEnabled() = 0;
    bool ActorMovable() { return b_actor_movable; }
    void SetActorMovable(bool v) { b_actor_movable = v; }
    virtual const ICollisionDamageInfo* CollisionDamageInfo() const = 0;
    virtual ICollisionDamageInfo* CollisionDamageInfo() = 0;
    virtual void Reinit() = 0;
    void SetPLastMaterialIDX(u16* p) { p_lastMaterialIDX = p; }
    const u16& LastMaterialIDX() const { return *p_lastMaterialIDX; }
    u16 InjuriousMaterialIDX() const { return injuriousMaterialIDX; }
    virtual void SetHitType(ALife::EHitType type) = 0;
    virtual bool TouchRestrictor(ERestrictionType rttype) = 0;
    virtual void SetElevator(IClimableObject* climable) {};
    u16 m_material{ GAMEMTL_NONE_IDX };
    virtual void SetMaterial(u16 material) { m_material = material; }
    virtual u16 GetMaterial() const { return m_material; }
    virtual void SetMaximumVelocity(float /*vel*/) {}
    virtual float GetMaximumVelocity() { return 0.f; }
    virtual void SetJupmUpVelocity(float /*velocity*/) {}
    virtual void IPosition(Fvector& /*pos*/) {}
    virtual u16 ContactBone() { return 0; }
    virtual void DeathPosition(Fvector& /*deathPos*/) {}
    virtual void ApplyImpulse(const Fvector& /*dir*/, const float /*P*/) {}
    virtual void ApplyForce(const Fvector& force) = 0;
    virtual void ApplyForce(const Fvector& dir, float force) = 0;
    virtual void ApplyForce(float x, float y, float z) = 0;
    virtual void AddControlVel(const Fvector& vel) = 0;
    virtual void Jump(const Fvector& jump_velocity) = 0;
    virtual bool JumpState() = 0;
    virtual EEnvironment CheckInvironment() = 0;
    virtual bool ContactWas() = 0;
    virtual void GroundNormal(Fvector& norm) = 0;
    virtual void Create(Fvector /*sizes*/) = 0;
    virtual void Destroy(void) = 0;
    virtual void SetBox(const Fvector& sizes) = 0;
    virtual void SetAcceleration(Fvector accel) = 0;
    virtual void SetForcedPhysicsControl(bool v) {}
    virtual bool ForcedPhysicsControl() { return false; }
    virtual void SetCamDir(const Fvector& cam_dir) = 0;
    virtual const Fvector& CamDir() const = 0;
    virtual Fvector GetAcceleration() = 0;
    virtual void SetPosition(const Fvector& pos) = 0;

    virtual void SetApplyGravity(BOOL flag)
    {
        if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
            GetPhysicsCore()->SetCharacterVirtualGravityFactor(m_char_handle, flag ? 1.0f : 0.0f);
    }

    virtual void SetObjectContactCallbackData(void* callback) = 0;
    virtual void SetObjectContactCallback(ObjectContactCallbackFun* callback) = 0;
    virtual void SetWheelContactCallback(ObjectContactCallbackFun* callback) = 0;
    virtual void SetStaticContactCallBack(ObjectContactCallbackFun* calback) = 0;
    virtual ObjectContactCallbackFun* ObjectContactCallBack() { return NULL; }
    virtual void DispatchNativeContacts(bool& collide, bool first, CPhysicsGeom* geometry1, CPhysicsGeom* geometry2,
        const Fvector& normal, const Fvector& position, SGameMtl* material1, SGameMtl* material2) {}
    virtual void ProcessNativeContact(const NativePhysicsContact&) {}
    virtual void GetVelocity(Fvector& vvel) const = 0;
    virtual void GetSavedVelocity(Fvector& vvel);
    virtual void GetSmothedVelocity(Fvector& vvel) = 0;
    virtual void SetVelocity(Fvector vel) = 0;
    virtual void SetAirControlFactor(float factor) = 0;
    virtual void GetPosition(Fvector& vpos) = 0;
    virtual void GetBodyPosition(Fvector& vpos) = 0;
    virtual const Fvector& BodyPosition() const = 0;

    virtual void GetFootCenter(Fvector& vpos)
    {
        if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
            GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, vpos);
    }

    virtual void SetMas(float mass) = 0;
    virtual void SetCollisionDamageFactor(float f) = 0;
    virtual float Mass() = 0;
    virtual void SetPhysicsRefObject(IPhysicsShellHolder* ref_object) = 0;
    virtual void SetNonInteractive(bool v) = 0;
    virtual void SetRestrictorRadius(ERestrictionType rtype, float r) {};
    virtual IPhysicsShellHolder* PhysicsRefObject() { return m_phys_ref_object; }

    // AICharacter
    virtual void GetDesiredPosition(Fvector& /*dpos*/) {}
    virtual void SetDesiredPosition(const Fvector& /*pos*/) {}
    virtual void BringToDesired(float /*time*/, float /*velocity*/, float force = 1.f) {}
    virtual bool TryPosition(Fvector /*pos*/, bool) { return false; }
    virtual bool TouchBorder() { return false; }

    virtual void getForce(Fvector& force);
    virtual void setForce(const Fvector& force);
    virtual float FootRadius() = 0;

    virtual void get_State(SPHNetState& state);
    virtual void set_State(const SPHNetState& state);
    virtual void cv2obj_Xfrom(const Fquaternion& q, const Fvector& pos, Fmatrix& xform) { ; }
    virtual void cv2bone_Xfrom(const Fquaternion& q, const Fvector& pos, Fmatrix& xform) { ; }
    virtual const Fvector& ControlAccel() const = 0;
    virtual float& FrictionFactor() = 0;
    virtual void CutVelocity(float l_limit, float a_limit);
    virtual u16 get_elements_number() { return 1; };
    virtual CPHSynchronize* get_element_sync(u16 element)
    {
        VERIFY(element == 0);
        return static_cast<CPHSynchronize*>(this);
    };
    virtual CElevatorState* ElevatorState() = 0;

public:
    virtual void Freeze() = 0;
    virtual void UnFreeze() = 0;
    virtual void step(float dt) = 0;
    virtual void collision_disable() = 0;
    virtual void collision_enable() = 0;
protected:
    virtual const Fmatrix& XFORM() const;
    virtual void get_LinearVel(Fvector& velocity) const;
    virtual void get_AngularVel(Fvector& velocity) const;
    virtual u16 numberOfGeoms() const { return 0; }
    virtual const IPhysicsGeometry* geometry(u16 i) const { return 0; }
    virtual const Fvector& mass_Center() const;

    virtual void get_xform(Fmatrix& form) const { form.set(XFORM()); }
    virtual bool collide_fluids() const { return true; }
public:
    virtual void update_last_material() = 0;
    virtual void NetRelcase(IPhysicsShellHolder* O) {};

public:
    CPHCharacter(void);
    virtual ~CPHCharacter(void);
};

XRPHYSICS_API void virtual_move_collide_callback(
    bool& do_colide, bool bo1,
    CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom,
    const Fvector& contact_normal, const Fvector& contact_pos,
    SGameMtl* material_1, SGameMtl* material_2);
XRPHYSICS_API CPHCharacter* create_ai_character();
XRPHYSICS_API CPHCharacter* create_actor_character(bool single_game);
