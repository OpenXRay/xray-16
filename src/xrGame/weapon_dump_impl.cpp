#include "StdAfx.h"
#include "Weapon.h"

void CWeapon::DumpActiveParams(shared_str const& section_name, CInifile& dst_ini) const
{
    CShootingObject::DumpActiveParams(section_name, dst_ini);

    dst_ini.w_float(section_name.c_str(), "pdm_disp_base", m_pdm.m_fPDM_disp_base);
    dst_ini.w_float(section_name.c_str(), "pdm_disp_vel_factor", m_pdm.m_fPDM_disp_vel_factor);
    dst_ini.w_float(section_name.c_str(), "pdm_disp_accel_factor", m_pdm.m_fPDM_disp_accel_factor);
    dst_ini.w_float(section_name.c_str(), "pdm_disp_crouch", m_pdm.m_fPDM_disp_crouch);
    dst_ini.w_float(section_name.c_str(), "pdm_disp_crouch_no_acc", m_pdm.m_fPDM_disp_crouch_no_acc);

    dst_ini.w_bool(section_name.c_str(), "cam_return", cam_recoil.ReturnMode);
    dst_ini.w_bool(section_name.c_str(), "cam_return_stop", cam_recoil.StopReturn);

    dst_ini.w_float(section_name.c_str(), "cam_relax_speed", cam_recoil.RelaxSpeed);
    dst_ini.w_float(section_name.c_str(), "cam_max_angle", cam_recoil.MaxAngleVert);
    dst_ini.w_float(section_name.c_str(), "cam_max_angle_horz", cam_recoil.MaxAngleHorz);
    dst_ini.w_float(section_name.c_str(), "cam_step_angle_horz", cam_recoil.StepAngleHorz);
    dst_ini.w_float(section_name.c_str(), "cam_dispersion_frac", cam_recoil.DispersionFrac);

    dst_ini.w_float(section_name.c_str(), "zoom_cam_relax_speed", zoom_cam_recoil.RelaxSpeed);
    dst_ini.w_float(section_name.c_str(), "zoom_cam_max_angle", zoom_cam_recoil.MaxAngleVert);
    dst_ini.w_float(section_name.c_str(), "zoom_cam_max_angle_horz", zoom_cam_recoil.MaxAngleHorz);
    dst_ini.w_float(section_name.c_str(), "zoom_cam_step_angle_horz", zoom_cam_recoil.StepAngleHorz);
    dst_ini.w_float(section_name.c_str(), "zoom_cam_dispersion_frac", zoom_cam_recoil.DispersionFrac);

    dst_ini.w_float(section_name.c_str(), "cam_dispersion", cam_recoil.Dispersion);
    dst_ini.w_float(section_name.c_str(), "cam_dispersion_inc", cam_recoil.DispersionInc);
    dst_ini.w_float(section_name.c_str(), "zoom_cam_dispersion", zoom_cam_recoil.Dispersion);
    dst_ini.w_float(section_name.c_str(), "zoom_cam_dispersion_inc", zoom_cam_recoil.DispersionInc);
}
