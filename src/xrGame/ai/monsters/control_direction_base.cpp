#include "stdafx.h"
#include "control_direction_base.h"
#include "basemonster/base_monster.h"
#include "../../detail_path_manager.h"

void CControlDirectionBase::reinit()
{
	inherited::reinit	();

	m_delay				= 0;
	m_time_last_faced	= 0;

	m_heading.init		();
	m_heading.target	= m_man->path_builder().m_body.target.yaw;

	m_pitch.init		();	
	m_pitch.target		= m_man->path_builder().m_body.target.pitch;

	m_man->capture		(this, ControlCom::eControlDir);
}

void CControlDirectionBase::face_target(const Fvector &position, u32 delay, float add_yaw)
{
	if (m_time_last_faced + delay > Device.dwTimeGlobal) return;

	m_delay = delay;

	float	yaw, pitch;
	Fvector dir;

	dir.sub		(position, m_object->Position());
	dir.getHP	(yaw,pitch);
	yaw			*= -1;

	yaw			+= (m_man->direction().is_from_right(position)) ? add_yaw : -add_yaw;
	yaw			= angle_normalize(yaw);

	m_heading.target	= yaw;

	m_time_last_faced	= Device.dwTimeGlobal;
}
void CControlDirectionBase::face_target(const CObject *obj,	u32 delay, float add_yaw) 
{
	face_target	(obj->Position(), delay, add_yaw);
}

void CControlDirectionBase::use_path_direction(bool reversed)
{
	float yaw,pitch;
	m_man->path_builder().detail().direction().getHP	(yaw,pitch);

	if (fsimilar(yaw,0.f,EPS_S)) return;

	m_heading.target = angle_normalize((reversed) ? (-yaw + PI) : (-yaw));
}

void CControlDirectionBase::set_heading_speed(float value, bool force) 
{
	m_heading.speed_target = value;
}

void CControlDirectionBase::set_heading(float value, bool force) 
{
	m_heading.target = value;
}
//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
void CControlDirectionBase::update_frame()
{
	SControlDirectionData	*ctrl_data = (SControlDirectionData *)m_man->data(this, ControlCom::eControlDir);
	if (!ctrl_data) return;

	ctrl_data->heading.target_angle	= m_heading.target;
	ctrl_data->heading.target_speed = m_heading.speed_target;
}
