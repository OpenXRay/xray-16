////////////////////////////////////////////////////////////////////////////
//	Module 		: profiler_inline.h
//	Created 	: 23.07.2004
//  Modified 	: 23.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Profiler inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

inline CProfilePortion::CProfilePortion(const char* id, bool enableIf)
{
    // XXX: wrap into global xrGame function and pass as enableIf
    //if (!psAI_Flags.test(aiStats))
    //    return;
    if (!enableIf || !psDeviceFlags.test(rsStatistic))
        return;
    enabled = true;
    m_timer_id = id;
    m_time = CPU::QPC();
}

inline CProfilePortion::~CProfilePortion()
{
    if (!enabled)
        return;
    u64 temp = CPU::QPC();
    m_time = temp - m_time;
    profiler().add_profile_portion(*this);
}

inline CProfiler& profiler() { return *g_profiler; }

inline CProfileStats::CProfileStats()
{
    m_update_time = 0;
    m_name = shared_str("");
    m_time = 0.f;
    m_min_time = 0.f;
    m_max_time = 0.f;
    m_total_time = 0.f;
    m_count = 0;
    m_call_count = 0;
}
