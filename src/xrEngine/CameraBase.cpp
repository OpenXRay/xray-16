// CameraBase.cpp: implementation of the CCameraBase class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IGame_Level.h"

#include "CameraBase.h"

CCameraBase::CCameraBase(IGameObject* p, u32 flags)
{
    m_Flags.assign(flags);
    vPosition.set(0, 0, 0);
    vDirection.set(0, 0, 1);
    vNormal.set(0, 1, 0);
    yaw = 0;
    pitch = 0;
    roll = 0;
    lim_yaw.set(0.f, 0.f);
    lim_pitch.set(0.f, 0.f);
    lim_roll.set(0.f, 0.f);
    bClampYaw = false;
    bClampPitch = false;
    bClampRoll = false;
    SetParent(p);
    f_fov = 90;
    f_aspect = 1.f;
    tag = 0;
    m_bInputDisabled = false; //--#SM+#--
}

CCameraBase::~CCameraBase() {}
void CCameraBase::Load(LPCSTR section)
{
    rot_speed = pSettings->r_fvector3(section, "rot_speed");

    lim_yaw = pSettings->r_fvector2(section, "lim_yaw");
    lim_pitch = pSettings->r_fvector2(section, "lim_pitch");

    bClampPitch = (0 != lim_pitch[0]) || (0 != lim_pitch[1]);
    bClampYaw = (0 != lim_yaw[0]) || (0 != lim_yaw[1]);

    if (bClampPitch)
        pitch = (lim_pitch[0] + lim_pitch[1]) * 0.5f;
    if (bClampYaw)
        yaw = (lim_yaw[0] + lim_yaw[1]) * 0.5f;
}

IC float AClamp(Fvector2& l, float v) { return (2 * v - l[0] - l[1]) / (l[1] - l[0]); }
float CCameraBase::CheckLimYaw()
{
    if (bClampYaw)
    {
        return AClamp(lim_yaw, yaw);
    }
    else
        return 0;
}

float CCameraBase::CheckLimPitch()
{
    if (bClampYaw)
    {
        return AClamp(lim_pitch, pitch);
    }
    else
        return 0;
}

float CCameraBase::CheckLimRoll()
{
    if (bClampYaw)
    {
        return AClamp(lim_roll, roll);
    }
    else
        return 0;
}
SCamEffectorInfo::SCamEffectorInfo()
{
    p.set(0, 0, 0);
    d.set(0, 0, 1);
    n.set(0, 1, 0);

    fFov = 90.0f;
    fNear = VIEWPORT_NEAR;
    fFar = 100.0f;
    fAspect = 1.f;
    offsetX = 0.f;
    offsetY = 0.f;
    dont_apply = false;
    affected_on_hud = true;
}
