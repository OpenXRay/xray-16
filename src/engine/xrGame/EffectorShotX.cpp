#include "stdafx.h"
#include "EffectorShotX.h"
#include "../xrEngine/CameraBase.h"

/*CCameraShotEffectorX::CCameraShotEffectorX	(float max_angle, float relax_speed,
								 float max_angle_horz, float step_angle_horz, float angle_frac) :
CCameraShotEffector(max_angle, relax_speed, max_angle_horz, step_angle_horz, angle_frac)
{
}
*/
/*
CCameraShotEffectorX::CCameraShotEffectorX( CameraRecoil& cam_recoil ) :
	CCameraShotEffector( cam_recoil )
{
}

CCameraShotEffectorX::~CCameraShotEffectorX	()
{
}

BOOL CCameraShotEffectorX::ProcessCam(SCamEffectorInfo& info)
{
	return TRUE;
};

void CCameraShotEffectorX::GetDeltaAngle	(Fvector& delta_angle)
{
	delta_angle.x = 0.0f;
	delta_angle.y = 0.0f;
	delta_angle.z = 0.0f;
}

void CCameraShotEffectorX::Shot			(float angle)
{
	float fAC_Old = fAngleVert;
	float fAH_Old = fAngleHorz;
	inherited::Shot(angle);
	float dAC = fAngleVert - fAC_Old;
	float dAH = fAngleHorz - fAH_Old;

	UpdateActorCamera(-dAC, -dAH);	
}

void	CCameraShotEffectorX::Clear				()
{
	fAngleVert	= -EPS_S;
	fAngleHorz	= 0.0f;
};

void	CCameraShotEffectorX::UpdateActorCamera		(float dPitch, float dYaw)
{
	if (!m_pActor) return;

	CCameraBase* pACam = m_pActor->cam_FirstEye();
	if (!pACam) return;

	if (pACam->bClampPitch)
	{
		while (pACam->pitch < pACam->lim_pitch[0])
			pACam->pitch += PI_MUL_2;
		while (pACam->pitch > pACam->lim_pitch[1])
			pACam->pitch -= PI_MUL_2;
	};

	pACam->pitch += dPitch;
	pACam->yaw += dYaw;

	if (pACam->bClampYaw)	clamp(pACam->yaw,pACam->lim_yaw[0],pACam->lim_yaw[1]);
	if (pACam->bClampPitch)	clamp(pACam->pitch,pACam->lim_pitch[0],pACam->lim_pitch[1]);

}
*/
