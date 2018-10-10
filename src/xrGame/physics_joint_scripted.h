#pragma once

#include "xrPhysics/iphysics_scripted.h"
#include "xrPhysics/PhysicsShell.h"

class cphysics_element_scripted;
class CPhysicsJoint;

class cphysics_joint_scripted : public cphysics_game_scripted<CPhysicsJoint>
{
public:
    cphysics_joint_scripted(CPhysicsJoint* imp) : cphysics_game_scripted<CPhysicsJoint>(imp) {}
    u16 BoneID() { return physics_impl().BoneID(); }
    cphysics_element_scripted* PFirst_element();
    cphysics_element_scripted* PSecond_element();

    u16 GetAxesNumber() { return physics_impl().GetAxesNumber(); }
    void SetAxisSDfactors(float spring_factor, float damping_factor, int axis_num)
    {
        physics_impl().SetAxisSDfactors(spring_factor, damping_factor, axis_num);
    }
    void SetJointSDfactors(float spring_factor, float damping_factor)
    {
        physics_impl().SetJointSDfactors(spring_factor, damping_factor);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetAnchor(const float x, const float y, const float z) { physics_impl().SetAnchor(x, y, z); }
    void SetAnchorVsFirstElement(const float x, const float y, const float z)
    {
        physics_impl().SetAnchorVsFirstElement(x, y, z);
    }
    void SetAnchorVsSecondElement(const float x, const float y, const float z)
    {
        physics_impl().SetAnchorVsSecondElement(x, y, z);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetAxisDir(const float x, const float y, const float z, const int axis_num)
    {
        physics_impl().SetAxisDir(x, y, z, axis_num);
    }
    void SetAxisDirVsFirstElement(const float x, const float y, const float z, const int axis_num)
    {
        physics_impl().SetAxisDirVsFirstElement(x, y, z, axis_num);
    }
    void SetAxisDirVsSecondElement(const float x, const float y, const float z, const int axis_num)
    {
        physics_impl().SetAxisDirVsSecondElement(x, y, z, axis_num);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetLimits(const float low, const float high, const int axis_num)
    {
        physics_impl().SetLimits(low, high, axis_num);
    }

    void SetForceAndVelocity(const float force, const float velocity = 0.f, const int axis_num = -1)
    {
        physics_impl().SetForceAndVelocity(force, velocity, axis_num);
    }
    void GetMaxForceAndVelocity(float& force, float& velocity, int axis_num)
    {
        physics_impl().GetMaxForceAndVelocity(force, velocity, axis_num);
    }
    float GetAxisAngle(int axis_num) { return physics_impl().GetAxisAngle(axis_num); }
    void GetLimits(float& lo_limit, float& hi_limit, int axis_num)
    {
        physics_impl().GetLimits(lo_limit, hi_limit, axis_num);
    }
    void GetAxisDirDynamic(int num, Fvector& axis) { physics_impl().GetAxisDirDynamic(num, axis); }
    void GetAnchorDynamic(Fvector& anchor) { physics_impl().GetAnchorDynamic(anchor); }
    bool isBreakable() { return physics_impl().isBreakable(); }
};
