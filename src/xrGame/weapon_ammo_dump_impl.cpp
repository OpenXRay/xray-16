#include "stdafx.h"
#include "WeaponAmmo.h"

void CCartridge::DumpActiveParams(shared_str const & section_name,
								   CInifile & dst_ini) const
{
	dst_ini.w_float	(section_name.c_str(), "k_dist",	param_s.kDist);
	dst_ini.w_float	(section_name.c_str(), "k_disp",	param_s.kDisp);
	dst_ini.w_float	(section_name.c_str(), "k_hit",		param_s.kHit);
	dst_ini.w_float	(section_name.c_str(), "k_impulse", param_s.kImpulse);
	dst_ini.w_float	(section_name.c_str(), "k_ap",		param_s.kAP);
	dst_ini.w_float	(section_name.c_str(), "k_airres",	param_s.kAirRes);
	dst_ini.w_s32	(section_name.c_str(), "k_buckshot",param_s.buckShot);
}