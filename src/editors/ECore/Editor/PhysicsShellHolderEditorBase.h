#ifndef _PHYSICS_SHELL_HOLDER_EDITOR_BASE_
#define _PHYSICS_SHELL_HOLDER_EDITOR_BASE_

#include "xrPhysics/IPhysicsShellHolder.h"

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
    virtual LPCSTR _BCL ObjectName() const { return "EditorActor"; }
    virtual LPCSTR _BCL ObjectNameVisual() const { return "unknown"; }
    virtual LPCSTR _BCL ObjectNameSect() const { return "unknown"; }
    virtual bool _BCL ObjectGetDestroy() const { return false; };
    virtual ICollisionHitCallback* _BCL ObjectGetCollisionHitCallback() { return 0; }
    virtual u16 _BCL ObjectID() const { return u16(-1); }
    virtual IGameObject* _BCL IObject() { return 0; } //--#SM+#--
    virtual ICollisionForm* _BCL ObjectCollisionModel()
    {
        VERIFY(false);
        return 0;
    }
    //	virtual	IRenderVisual*				_BCL	ObjectVisual						()				 { return
    // m_pVisual;}
    virtual IDamageSource* _BCL ObjectCastIDamageSource() { return 0; }
    virtual void _BCL ObjectProcessingDeactivate() { ; }
    virtual void _BCL ObjectProcessingActivate() {}
    virtual void _BCL ObjectSpatialMove() {}
    virtual CPhysicsShell*& _BCL ObjectPPhysicsShell() { return m_physics_shell; }
    virtual void _BCL enable_notificate() {}
    virtual bool _BCL has_parent_object() { return false; }
    virtual void _BCL on_physics_disable() {}
    virtual IPHCapture* _BCL PHCapture() { return 0; }
    virtual bool _BCL IsInventoryItem() { return false; }
    virtual bool _BCL IsActor() { return false; }
    virtual bool _BCL IsStalker() { return false; }
    virtual bool _BCL IsCollideWithBullets() { return false; } //--#SM+#--
    virtual bool _BCL IsCollideWithActorCamera() { return false; } //--#SM+#--
    // virtual	void						SetWeaponHideState					( u16 State, bool bSet )=0;
    virtual void _BCL HideAllWeapons(bool v) {} //(SetWeaponHideState(INV_STATE_BLOCK_ALL,true))
    virtual void _BCL MovementCollisionEnable(bool enable) {}
    virtual CPHSoundPlayer* _BCL ObjectPhSoundPlayer() { return 0; }
    virtual ICollisionDamageReceiver* _BCL ObjectPhCollisionDamageReceiver() { return 0; }
    virtual void _BCL BonceDamagerCallback(float& damage_factor) {}

public:
    virtual Fmatrix& _BCL ObjectXFORM() { return m_object_xform; }

private:
    virtual Fvector& _BCL ObjectPosition() { return m_object_xform.c; }
#ifdef DEBUG
    virtual std::string _BCL dump(EDumpType type) const
    {
        VERIFY(false);
        return std::string("ActorEditor!");
    }
#endif
};

#endif
