// EffectorShot.cpp: implementation of the CCameraShotEffector class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "EffectorShot.h"
#include "Weapon.h"

//-----------------------------------------------------------------------------
// Weapon shot effector
//-----------------------------------------------------------------------------
CWeaponShotEffector::CWeaponShotEffector()
{
    Reset();
    //	m_first_shot_pos = 0.0f;
}

void CWeaponShotEffector::Initialize(const CameraRecoil& cam_recoil)
{
    m_cam_recoil.Clone(cam_recoil);
    Reset();
}

void CWeaponShotEffector::Reset()
{
    m_angle_vert = 0.0f;
    m_angle_horz = 0.0f;

    m_prev_angle_vert = 0.0f;
    m_prev_angle_horz = 0.0f;

    m_delta_vert = 0.0f;
    m_delta_horz = 0.0f;

    m_LastSeed = 0;
    m_single_shot = false;
    m_first_shot = false;
    m_actived = false;
    m_shot_end = true;
}

void CWeaponShotEffector::Shot(CWeapon* weapon)
{
    R_ASSERT(weapon);
    m_shot_numer = weapon->ShotsFired() - 1;
    if (m_shot_numer <= 0)
    {
        m_shot_numer = 0;
        Reset();
    }
    m_single_shot = (weapon->GetCurrentFireMode() == 1);

    float angle = m_cam_recoil.Dispersion * weapon->cur_silencer_koef.cam_dispersion;
    angle += m_cam_recoil.DispersionInc * weapon->cur_silencer_koef.cam_disper_inc * (float)m_shot_numer;
    Shot2(angle);
}

void CWeaponShotEffector::Shot2(float angle)
{
    m_angle_vert +=
        angle * (m_cam_recoil.DispersionFrac + m_Random.randF(-1.0f, 1.0f) * (1.0f - m_cam_recoil.DispersionFrac));

    clamp(m_angle_vert, -m_cam_recoil.MaxAngleVert, m_cam_recoil.MaxAngleVert);
    if (fis_zero(m_angle_vert - m_cam_recoil.MaxAngleVert))
    {
        m_angle_vert *= m_Random.randF(0.96f, 1.04f);
    }

    float rdm = m_Random.randF(-1.0f, 1.0f);
    m_angle_horz += (m_angle_vert / m_cam_recoil.MaxAngleVert) * rdm * m_cam_recoil.StepAngleHorz;

    clamp(m_angle_horz, -m_cam_recoil.MaxAngleHorz, m_cam_recoil.MaxAngleHorz);

    m_first_shot = true;
    m_actived = true;
    m_shot_end = false;
}

void CWeaponShotEffector::Relax()
{
    float time_to_relax = _abs(m_angle_vert) / m_cam_recoil.RelaxSpeed;
    float relax_speed_horz = (fis_zero(time_to_relax)) ? 0.0f : _abs(m_angle_horz) / time_to_relax;

    float dt = Device.fTimeDelta;

    if (m_angle_horz >= 0.0f) // h
    {
        m_angle_horz -= relax_speed_horz * dt;
    }
    else
    {
        m_angle_horz += relax_speed_horz * dt;
    }

    if (m_angle_vert >= 0.0f) // v
    {
        m_angle_vert -= m_cam_recoil.RelaxSpeed * dt;
        if (m_angle_vert < 0.0f)
        {
            m_angle_vert = 0.0f;
            m_actived = false;
        }
    }
    else
    {
        m_angle_vert += m_cam_recoil.RelaxSpeed * dt;
        if (m_angle_vert > 0.0f)
        {
            m_angle_vert = 0.0f;
            m_actived = false;
        }
    }
}

void CWeaponShotEffector::Update()
{
    if (m_actived && m_cam_recoil.ReturnMode /*|| m_single_shot*/)
    {
        Relax();
    }

    if (!m_cam_recoil.ReturnMode && m_shot_end && !m_single_shot)
    {
        m_actived = false;
    }

    m_delta_vert = m_angle_vert - m_prev_angle_vert;
    m_delta_horz = m_angle_horz - m_prev_angle_horz;

    m_prev_angle_vert = m_angle_vert;
    m_prev_angle_horz = m_angle_horz;

    //	Msg( " <<[%d]  v=%.4f  dv=%.4f   a=%d s=%d  fr=%d", m_shot_numer, m_angle_vert, m_delta_vert, m_actived,
    // m_first_shot, Device.dwFrame );
}

void CWeaponShotEffector::GetDeltaAngle(Fvector& angle)
{
    angle.x = -m_angle_vert;
    angle.y = -m_angle_horz;
    angle.z = 0.0f;
}

void CWeaponShotEffector::GetLastDelta(Fvector& delta_angle)
{
    delta_angle.x = -m_delta_vert;
    delta_angle.y = -m_delta_horz;
    delta_angle.z = 0.0f;
}

void CWeaponShotEffector::SetRndSeed(s32 Seed)
{
    if (m_LastSeed == 0)
    {
        m_LastSeed = Seed;
        //		m_Random.seed		(Seed);
        m_Random.seed(Device.dwFrame);
    }
}

void CWeaponShotEffector::ChangeHP(float* pitch, float* yaw)
{
    *pitch -= m_delta_vert; // y = pitch = p = vert
    *yaw -= m_delta_horz; // x = yaw   = h = horz

    //	if ( m_first_shot )
    //	{
    //		m_first_shot_pos = *pitch;
    //		m_first_shot = false;
    //	}

    //	if ( m_cam_recoil.ReturnMode && m_cam_recoil.StopReturn && (*pitch > m_first_shot_pos + 0.1f) )
    //	{
    //		m_actived = false;
    //	}
    //	Msg( "[%d]  pitch = %.4f   yaw = %.4f    fs=%d    a=%d  fr=%d", m_shot_numer, *pitch, *yaw, m_first_shot,
    // m_actived, Device.dwFrame );
}

//-----------------------------------------------------------------------------
// Camera shot effector
//-----------------------------------------------------------------------------

CCameraShotEffector::CCameraShotEffector(const CameraRecoil& cam_recoil) : CEffectorCam(eCEShot, 100000.0f)
{
    CWeaponShotEffector::Initialize(cam_recoil);
    m_pActor = NULL;
}

CCameraShotEffector::~CCameraShotEffector() {}
BOOL CCameraShotEffector::ProcessCam(SCamEffectorInfo& info)
{
    Update();
    return TRUE;
}
