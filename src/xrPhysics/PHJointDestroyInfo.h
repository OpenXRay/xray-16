#pragma once

#include "xrCore/_vector3d.h"
#include "xrCore/xr_types.h"

struct SPhysicsJointFeedback
{
    Fvector f1;
    Fvector t1;
    Fvector f2;
    Fvector t2;

    SPhysicsJointFeedback()
    {
        f1.set(0,0,0); t1.set(0,0,0);
        f2.set(0,0,0); t2.set(0,0,0);
    }
};

class CPHJointDestroyInfo
{
    friend class CPHShellSplitterHolder;
    friend class CPHShell;
    friend class CPHFracturesHolder;

    SPhysicsJointFeedback m_joint_feedback;
    float m_sq_break_force;
    float m_sq_break_torque;
    u16 m_end_element;
    u16 m_end_joint;
    bool m_breaked;

public:
    CPHJointDestroyInfo(float break_force, float break_torque);

    IC SPhysicsJointFeedback* JointFeedback() { return &m_joint_feedback; }
    IC bool Breaked() { return m_breaked; };

    bool Update();
};
