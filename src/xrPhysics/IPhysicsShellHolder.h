#pragma once
class ICollisionDamageInfo;
class IDamageSource;
// class IKinematics;
// class IRenderVisual;
class IKinematics;
class CPhysicsShell;
class IPHCapture;
class IPhysicsShellHolder;
class CPHSoundPlayer;
class ICollisionDamageReceiver;
class ICollisionForm;
class IGameObject; //--#SM+#--

class ICollisionHitCallback
{
public:
    virtual void call(
        IPhysicsShellHolder* obj, float min_cs, float max_cs, float& cs, float& hl, ICollisionDamageInfo* di) = 0;
    virtual ~ICollisionHitCallback() {}
};

#ifdef DEBUG
enum EDumpType
{
    base,
    poses,
    vis_geom,
    props,
    full,
    full_capped
};
#endif

class IPhysicsShellHolder
{
public:
    virtual Fmatrix& ObjectXFORM() = 0;
    virtual Fvector& ObjectPosition() = 0;
    virtual LPCSTR ObjectName() const = 0;
    virtual LPCSTR ObjectNameVisual() const = 0;
    virtual LPCSTR ObjectNameSect() const = 0;
    virtual bool ObjectGetDestroy() const = 0;
    virtual ICollisionHitCallback* ObjectGetCollisionHitCallback() = 0;
    virtual u16 ObjectID() const = 0;
    virtual IGameObject* IObject() = 0; //--#SM+#--
    virtual ICollisionForm* ObjectCollisionModel() = 0;
    //	virtual	IRenderVisual*					ObjectVisual						()						=0;
    virtual IKinematics* ObjectKinematics() = 0;
    virtual IDamageSource* ObjectCastIDamageSource() = 0;
    virtual void ObjectProcessingDeactivate() = 0;
    virtual void ObjectProcessingActivate() = 0;
    virtual void ObjectSpatialMove() = 0;
    virtual CPhysicsShell*& ObjectPPhysicsShell() = 0;
    virtual void enable_notificate() = 0;
    virtual bool has_parent_object() = 0;
    virtual void on_physics_disable() = 0;
    virtual IPHCapture* PHCapture() = 0;
    virtual bool IsInventoryItem() = 0;
    virtual bool IsActor() = 0;
    virtual bool IsStalker() = 0;
    virtual bool IsCollideWithBullets() = 0; //--#SM+#--
    virtual bool IsCollideWithActorCamera() = 0; //--#SM+#--
    // virtual	void							SetWeaponHideState					( u16 State, bool bSet )=0;
    virtual void HideAllWeapons(bool v) = 0; //(SetWeaponHideState(INV_STATE_BLOCK_ALL,true))
    virtual void MovementCollisionEnable(bool enable) = 0;
    virtual CPHSoundPlayer* ObjectPhSoundPlayer() = 0;
    virtual ICollisionDamageReceiver* ObjectPhCollisionDamageReceiver() = 0;
    virtual void BonceDamagerCallback(float& damage_factor) = 0;
#ifdef DEBUG
    virtual std::string dump(EDumpType type) const = 0;
#endif
};
