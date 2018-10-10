#include "StdAfx.h"
#include "ShootingObject.h"
#include "GameObject.h"

void CShootingObject::DumpActiveParams(shared_str const& section_name, CInifile& dst_ini) const
{
    dst_ini.w_fvector4(section_name.c_str(), "hit_power", fvHitPower);
    dst_ini.w_float(section_name.c_str(), "hit_impulse", fHitImpulse);
    dst_ini.w_float(section_name.c_str(), "bullet_speed", m_fStartBulletSpeed);
    dst_ini.w_float(section_name.c_str(), "max_distance", fireDistance);
    dst_ini.w_float(section_name.c_str(), "disp_base", fireDispersionBase);
    // dst_ini.w_float		(section_name.c_str(), "shot_time_counter", 	fShotTimeCounter);

    dst_ini.w_float(section_name.c_str(), "sil_hit_power", m_silencer_koef.hit_power);
    dst_ini.w_float(section_name.c_str(), "sil_hit_impulse", m_silencer_koef.hit_impulse);
    dst_ini.w_float(section_name.c_str(), "sil_bullet_speed", m_silencer_koef.bullet_speed);
    dst_ini.w_float(section_name.c_str(), "sil_disp_base", m_silencer_koef.fire_dispersion);
}
