#pragma once

#include "xrPhysics/iphysics_scripted.h"
#include "xrPhysics/PhysicsShell.h"

class CPhysicsShell;
class cphysics_element_scripted;
class cphysics_joint_scripted;

class cphysics_shell_scripted : public cphysics_game_scripted<CPhysicsShell>
{
public:
    cphysics_shell_scripted(CPhysicsShell* imp) : cphysics_game_scripted<CPhysicsShell>(imp) {}
    void applyForce(float x, float y, float z) { physics_impl().applyForce(x, y, z); }
    cphysics_element_scripted* get_Element(LPCSTR bone_name);
    cphysics_element_scripted* get_Element(u16 bone_id);
    cphysics_element_scripted* get_ElementByStoreOrder(u16 idx);

    u16 get_ElementsNumber() { return physics_impl().get_ElementsNumber(); }
    cphysics_joint_scripted* get_Joint(LPCSTR bone_name);
    cphysics_joint_scripted* get_Joint(u16 bone_id);
    cphysics_joint_scripted* get_JointByStoreOrder(u16 idx);

    //&cphysics_shell_scripted::get_JointByStoreOrder)

    u16 get_JointsNumber() { return physics_impl().get_JointsNumber(); }
    void BlockBreaking() { physics_impl().BlockBreaking(); }
    void UnblockBreaking() { physics_impl().UnblockBreaking(); }
    bool IsBreakingBlocked() { return physics_impl().IsBreakingBlocked(); }
    bool isBreakable() { return physics_impl().isBreakable(); }
    void get_LinearVel(Fvector& velocity) const { physics_impl().get_LinearVel(velocity); }
    void get_AngularVel(Fvector& velocity) const { physics_impl().get_AngularVel(velocity); }
};
