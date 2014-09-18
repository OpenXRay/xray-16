// EffectorZoomInertion.cpp: инерция(покачивания) оружия в режиме
//							 приближения
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EffectorZoomInertion.h"


#define EFFECTOR_ZOOM_SECTION "zoom_inertion_effector"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEffectorZoomInertion::CEffectorZoomInertion	() : CEffectorCam(eCEZoom,100000.f)
{
	Load();
	SetRndSeed		(Device.dwTimeContinual);
	m_dwTimePassed	= 0;
}

CEffectorZoomInertion::~CEffectorZoomInertion	()
{

}

void CEffectorZoomInertion::LoadParams			(LPCSTR Section, LPCSTR Prefix)
{
	string256 full_name;
	m_fCameraMoveEpsilon	= READ_IF_EXISTS(pSettings, r_float, Section, strconcat(sizeof(full_name),full_name, Prefix, "camera_move_epsilon"),	pSettings->r_float(EFFECTOR_ZOOM_SECTION, "camera_move_epsilon"));
	m_fDispMin				= READ_IF_EXISTS(pSettings, r_float, Section, strconcat(sizeof(full_name),full_name, Prefix, "disp_min"),				pSettings->r_float(EFFECTOR_ZOOM_SECTION, "disp_min"));
	m_fSpeedMin				= READ_IF_EXISTS(pSettings, r_float, Section, strconcat(sizeof(full_name),full_name, Prefix, "speed_min"),			pSettings->r_float(EFFECTOR_ZOOM_SECTION, "speed_min"));
	m_fZoomAimingDispK		= READ_IF_EXISTS(pSettings, r_float, Section, strconcat(sizeof(full_name),full_name, Prefix, "zoom_aim_disp_k"),		pSettings->r_float(EFFECTOR_ZOOM_SECTION, "zoom_aim_disp_k"));
	m_fZoomAimingSpeedK		= READ_IF_EXISTS(pSettings, r_float, Section, strconcat(sizeof(full_name),full_name, Prefix, "zoom_aim_speed_k"),		pSettings->r_float(EFFECTOR_ZOOM_SECTION, "zoom_aim_speed_k"));
	m_dwDeltaTime			= READ_IF_EXISTS(pSettings, r_u32, Section, strconcat(sizeof(full_name),full_name, Prefix, "delta_time"),			pSettings->r_u32(EFFECTOR_ZOOM_SECTION, "delta_time"));
};

void CEffectorZoomInertion::Load		()
{
	LoadParams(EFFECTOR_ZOOM_SECTION, "");
	
	m_dwTimePassed		= 0;

	m_fFloatSpeed		= m_fSpeedMin;
	m_fDispRadius		= m_fDispMin;

	m_fEpsilon = 2*m_fFloatSpeed;


	m_vTargetVel.set(0.f,0.f,0.f);
	m_vCurrentPoint.set(0.f,0.f,0.f);
	m_vTargetPoint.set(0.f,0.f,0.f);
	m_vLastPoint.set(0.f,0.f,0.f);
}

void	CEffectorZoomInertion::Init				(CWeaponMagazined*	pWeapon)
{
	if (!pWeapon) return;

	LoadParams(*pWeapon->cNameSect(), "ezi_");
};

void CEffectorZoomInertion::SetParams	(float disp)
{
	float old_disp = m_fDispRadius;

	m_fDispRadius = disp*m_fZoomAimingDispK;
	if(m_fDispRadius<m_fDispMin) 
		m_fDispRadius = m_fDispMin;

	m_fFloatSpeed = disp*m_fZoomAimingSpeedK;
	if(m_fFloatSpeed<m_fSpeedMin) 
		m_fFloatSpeed = m_fSpeedMin;

	//для того, чтоб сразу прошел пересчет направления
	//движения прицела
	if(!fis_zero(old_disp-m_fDispRadius,EPS))
		m_fEpsilon = 2*m_fDispRadius;
}


void			CEffectorZoomInertion::CalcNextPoint		()
{
	m_fEpsilon = 2*m_fFloatSpeed;

	float half_disp_radius = m_fDispRadius/2.f;
	m_vTargetPoint.x = m_Random.randF(-half_disp_radius,half_disp_radius);
	m_vTargetPoint.y = m_Random.randF(-half_disp_radius,half_disp_radius);

	m_vTargetVel.sub(m_vTargetPoint, m_vLastPoint);
};

BOOL CEffectorZoomInertion::ProcessCam(SCamEffectorInfo& info)
{
	bool camera_moved = false;

	//определяем двигал ли прицелом актер
	if(!info.d.similar(m_vOldCameraDir, m_fCameraMoveEpsilon))
		camera_moved = true;


	Fvector dir;
	dir.sub(m_vCurrentPoint,m_vTargetPoint);

	if (m_dwTimePassed == 0)
	{
		m_vLastPoint.set(m_vCurrentPoint);
		CalcNextPoint();
	}
	else
	{
		while (m_dwTimePassed > m_dwDeltaTime)
		{
			m_dwTimePassed -= m_dwDeltaTime;

			m_vLastPoint.set(m_vTargetPoint);
			CalcNextPoint();
		};
	}

	m_vCurrentPoint.lerp(m_vLastPoint, m_vTargetPoint, float(m_dwTimePassed)/m_dwDeltaTime);

	m_vOldCameraDir = info.d;

	if(!camera_moved)
		info.d.add(m_vCurrentPoint);

	m_dwTimePassed += Device.dwTimeDelta;

	return TRUE;
}