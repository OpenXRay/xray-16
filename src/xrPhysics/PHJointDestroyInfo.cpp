#include "StdAfx.h"
#include "PHJointDestroyInfo.h"
#include "PhysicsCommon.h"
#include "console_vars.h"

CPHJointDestroyInfo::CPHJointDestroyInfo(float break_force, float break_torque)
{
    m_sq_break_force = break_force * break_force;
    m_sq_break_torque = break_torque * break_torque;

    m_breaked = false;
}

bool CPHJointDestroyInfo::Update()
{
    float sq_break_force = m_sq_break_force / ph_console::phBreakCommonFactor;
    float sq_break_torque = m_sq_break_torque / ph_console::phBreakCommonFactor; // Исправлен баг оригинала X-Ray

    if (m_joint_feedback.f1.square_magnitude() > sq_break_force)
    {
        m_breaked = true;
        return true;
    }
    if (m_joint_feedback.f2.square_magnitude() > sq_break_force)
    {
        m_breaked = true;
        return true;
    }
    if (m_joint_feedback.t1.square_magnitude() > sq_break_torque)
    {
        m_breaked = true;
        return true;
    }
    if (m_joint_feedback.t2.square_magnitude() > sq_break_torque)
    {
        m_breaked = true;
        return true;
    }

    return false;
}
