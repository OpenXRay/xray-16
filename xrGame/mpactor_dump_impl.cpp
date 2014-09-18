#include "stdafx.h"
#include "actor_mp_client.h"

void CActorMP::DumpActiveParams(shared_str const & section_name,
								CInifile & dst_ini) const
{
	dst_ini.w_float	(section_name.c_str(), "m_fWalkAccel",		m_fWalkAccel);
	dst_ini.w_float	(section_name.c_str(), "m_fJumpSpeed",		m_fJumpSpeed);
	dst_ini.w_float	(section_name.c_str(), "m_fRunFactor",		m_fRunFactor);
	dst_ini.w_float	(section_name.c_str(), "m_fRunBackFactor",	m_fRunBackFactor);
	dst_ini.w_float	(section_name.c_str(), "m_fWalkBackFactor",	m_fWalkBackFactor);	
	dst_ini.w_float	(section_name.c_str(), "m_fCrouchFactor",	m_fCrouchFactor);
	dst_ini.w_float	(section_name.c_str(), "m_fClimbFactor",	m_fClimbFactor);
	dst_ini.w_float	(section_name.c_str(), "m_fSprintFactor",	m_fSprintFactor);

	dst_ini.w_float	(section_name.c_str(), "m_fWalk_StrafeFactor",	m_fWalk_StrafeFactor);
	dst_ini.w_float	(section_name.c_str(), "m_fRun_StrafeFactor",	m_fRun_StrafeFactor);


	dst_ini.w_float	(section_name.c_str(), "m_fDispBase",		m_fDispBase);
	dst_ini.w_float	(section_name.c_str(), "m_fDispAim",		m_fDispAim);					
	
	dst_ini.w_float	(section_name.c_str(), "m_fDispVelFactor",		m_fDispVelFactor);
	dst_ini.w_float	(section_name.c_str(), "m_fDispAccelFactor",	m_fDispAccelFactor);
	dst_ini.w_float	(section_name.c_str(), "m_fDispCrouchFactor",	m_fDispCrouchFactor);
	dst_ini.w_float	(section_name.c_str(), "m_fDispCrouchNoAccelFactor",m_fDispCrouchNoAccelFactor);
}