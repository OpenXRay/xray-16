#include "stdafx.h"
#include "spectator_camera_first_eye.h"
#include "xr_level_controller.h"
#include "../xrEngine/xr_object.h"

CSpectrCameraFirstEye::CSpectrCameraFirstEye(float const & fTimeDelta,
											 CObject* p,
											 u32 flags) :
	inherited(p, flags),
	m_fTimeDelta(fTimeDelta)
{
};

CSpectrCameraFirstEye::~CSpectrCameraFirstEye()
{
}

void CSpectrCameraFirstEye::Move( int cmd, float val, float factor )
{
	//Msg("Current ftimedelta = %0.4f", m_fTimeDelta);
	if (bClampPitch)
	{
		while (pitch < lim_pitch[0])
			pitch += PI_MUL_2;
		while (pitch > lim_pitch[1])
			pitch -= PI_MUL_2;
	};
	switch (cmd){
	case kDOWN:		pitch	-= val?val:(rot_speed.y*m_fTimeDelta/factor);	break;
	case kUP:		pitch	+= val?val:(rot_speed.y*m_fTimeDelta/factor);	break;
	case kLEFT:		yaw		-= val?val:(rot_speed.x*m_fTimeDelta/factor);	break;
	case kRIGHT:	yaw		+= val?val:(rot_speed.x*m_fTimeDelta/factor);	break;
	}
	if (bClampYaw)		clamp(yaw,lim_yaw[0],lim_yaw[1]);
	if (bClampPitch)	clamp(pitch,lim_pitch[0],lim_pitch[1]);
}