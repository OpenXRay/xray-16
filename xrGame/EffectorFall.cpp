#include "stdafx.h"
#include "EffectorFall.h"
#include "CameraEffector.h"
#include "GamePersistent.h"

#define FALL_SPEED 3.5f
#define FALL_MAXDIST 0.15f

CEffectorFall::CEffectorFall(float power,float life_time) : CEffectorCam(eCEFall, life_time)
{
	SetHudAffect			(false);
	fPower					= (power>1)?1:((power<0)?0:power*power);
	fPhase					= 0;
}


BOOL CEffectorFall::ProcessCam(SCamEffectorInfo& info)
{
	fPhase+=FALL_SPEED*Device.fTimeDelta;
	if (fPhase<1)	
		info.p.y-=FALL_MAXDIST*fPower*_sin(M_PI*fPhase+M_PI);
	else			
		fLifeTime=-1;
	return TRUE;
}

CEffectorDOF::CEffectorDOF(const Fvector4& dof)
:CEffectorCam(eCEDOF, 100000)
{
	GamePersistent().SetEffectorDOF	(Fvector().set(dof.x,dof.y,dof.z));
	m_fPhase						= Device.fTimeGlobal + dof.w;
}

BOOL CEffectorDOF::ProcessCam(SCamEffectorInfo& info)
{
	if (m_fPhase<Device.fTimeGlobal)
	{
		GamePersistent().RestoreEffectorDOF();
		fLifeTime=-1;
	}
	return				TRUE;
}
