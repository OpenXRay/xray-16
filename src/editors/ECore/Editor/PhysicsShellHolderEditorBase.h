#ifndef _PHYSICS_SHELL_HOLDER_EDITOR_BASE_
#define _PHYSICS_SHELL_HOLDER_EDITOR_BASE_

#include "xrPhysics/IPhysicsShellHolder.h"

class IGameObject;

class ECORE_API CPhysicsShellHolderEditorBase : public IPhysicsShellHolder
{
public:
    void CreatePhysicsShell(Fmatrix* obj_xform);
    void DeletePhysicsShell();
    void UpdateObjectXform(Fmatrix& obj_xform);
    void ApplyDragForce(const Fvector& force);

protected:
    CPhysicsShellHolderEditorBase() : m_physics_shell(0), m_object_xform(Fidentity) {}
    ~CPhysicsShellHolderEditorBase() { /*DeletePhysicsShell	();*/}

protected:
    CPhysicsShell* m_physics_shell;
    Fmatrix m_object_xform;

private:
    virtual LPCSTR ObjectName() const { return "EditorActor"; }
    virtual LPCSTR ObjectNameVisual() const { return "unknown"; }
    virtual LPCSTR ObjectNameSect() const { return "unknown"; }
    virtual bool ObjectGetDestroy() const { return false; };
    virtual ICollisionHitCallback* ObjectGetCollisionHitCallback() { return 0; }
    virtual u16 ObjectID() const { return u16(-1); }
    virtual IGameObject* IObject() { return 0; } //--#SM+#--
    virtual ICollisionForm* ObjectCollisionModel()
    {
        VERIFY(false);
        return 0;
    }
    //	virtual	IRenderVisual*				_BCL	ObjectVisual						()				 { return
    // m_pVisual;}
    virtual IDamageSource* ObjectCastIDamageSource() { return 0; }
    virtual void ObjectProcessingDeactivate() { ; }
    virtual void ObjectProcessingActivate() {}
    virtual void ObjectSpatialMove() {}
    virtual CPhysicsShell*& ObjectPPhysicsShell() { return m_physics_shell; }
    virtual void enable_notificate() {}
    virtual bool has_parent_object() { return false; }
    virtual void on_physics_disable() {}
    virtual IPHCapture* PHCapture() { return 0; }
    virtual bool IsInventoryItem() { return false; }
    virtual bool IsActor() { return false; }
    virtual bool IsStalker() { return false; }
    virtual bool IsCollideWithBullets() { return false; } //--#SM+#--
    virtual bool IsCollideWithActorCamera() { return false; } //--#SM+#--
    // virtual	void						SetWeaponHideState					( u16 State, bool bSet )=0;
    virtual void HideAllWeapons(bool v) {} //(SetWeaponHideState(INV_STATE_BLOCK_ALL,true))
    virtual void MovementCollisionEnable(bool enable) {}
    virtual CPHSoundPlayer* ObjectPhSoundPlayer() { return 0; }
    virtual ICollisionDamageReceiver* ObjectPhCollisionDamageReceiver() { return 0; }
    virtual void BonceDamagerCallback(float& damage_factor) {}

public:
    virtual Fmatrix& ObjectXFORM() { return m_object_xform; }

private:
    virtual Fvector& ObjectPosition() { return m_object_xform.c; }
#ifdef DEBUG
    virtual std::string dump(EDumpType type) const
    {
        VERIFY(false);
        return std::string("ActorEditor!");
    }
#endif
};

#endif
