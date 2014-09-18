#pragma once

IC	void CControlPathBuilderBase::set_cover_params(float min, float max, float dev, float radius)
{
	m_cover_info.min_dist		= min;
	m_cover_info.max_dist		= max;
	m_cover_info.deviation		= dev;
	m_cover_info.radius			= radius;
}

IC void CControlPathBuilderBase::set_use_covers(bool val)
{
	m_cover_info.use_covers	= val;	
}

IC void CControlPathBuilderBase::set_rebuild_time(u32 time) 
{
	m_time	= time;
}

IC void	CControlPathBuilderBase::set_distance_to_end(float dist)
{
	m_distance_to_path_end = dist;
}

IC void CControlPathBuilderBase::set_generic_parameters()
{
	CControlPathBuilderBase::set_rebuild_time			(5000);
	CControlPathBuilderBase::set_distance_to_end		(3.f);
	CControlPathBuilderBase::set_use_covers				();
	CControlPathBuilderBase::set_cover_params			(5.f, 30.f, 1.f, 30.f);
}
