////////////////////////////////////////////////////////////////////////////
//	Module 		: profiler_inline.h
//	Created 	: 23.07.2004
//  Modified 	: 23.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Profiler inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CProfilePortion::CProfilePortion	(LPCSTR timer_id)
{
	if (!psAI_Flags.test(aiStats))
		return;

	if (!psDeviceFlags.test(rsStatistic))
		return;

	m_timer_id							= timer_id;
	m_time								= CPU::QPC();
}

IC	CProfilePortion::~CProfilePortion	()
{
	if (!psAI_Flags.test(aiStats))
		return;

	if (!psDeviceFlags.test(rsStatistic))
		return;

	u64									temp = CPU::QPC();
	m_time								= temp - m_time;
	profiler().add_profile_portion		(*this);
}

IC	CProfiler&	profiler				()
{
	return			(*g_profiler);
}

IC	CProfileStats::CProfileStats		()
{
	m_update_time	= 0;
	m_name			= shared_str("");
	m_time			= 0.f;
	m_min_time		= 0.f;
	m_max_time		= 0.f;
	m_total_time	= 0.f;
	m_count			= 0;
	m_call_count	= 0;
}
