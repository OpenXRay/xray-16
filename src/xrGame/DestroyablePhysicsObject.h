#pragma once

class CDestroyablePhysicsObject : public CPhysicObject,
                                  public CPHDestroyable,
                                  public CPHCollisionDamageReceiver,
                                  public CHitImmunity,
                                  public CDamageManager
{
protected:
    using inherited = CPhysicObject;

private:
    float m_fHealth;
    ref_sound m_destroy_sound;
    shared_str m_destroy_particles;

public:
    CDestroyablePhysicsObject();
    virtual ~CDestroyablePhysicsObject();
    virtual CPhysicsShellHolder* PPhysicsShellHolder();
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void Hit(SHit* pHDS);
    virtual void InitServerObject(CSE_Abstract* D);
    virtual ICollisionDamageReceiver* PHCollisionDamageReceiver() { return (this); }
    virtual IFactoryObject* _construct();
    virtual CPhysicsShellHolder* cast_physics_shell_holder() { return this; }
    virtual CParticlesPlayer* cast_particles_player() { return this; }
    virtual CPHDestroyable* ph_destroyable() { return this; }
    virtual void shedule_Update(u32 dt);
    virtual bool CanRemoveObject();
    virtual void OnChangeVisual();

protected:
    void Destroy();

private:
};
