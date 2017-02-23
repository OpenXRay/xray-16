////////////////////////////////////////////////////////////////////////////
//	Module 		: profiler.cpp
//	Created 	: 23.07.2004
//  Modified 	: 23.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Profiler
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
// XXX nitrocaster PROFILER: temporarily disabled due to linkage issues
#if 0
#include "profiler.h"
#include "xrEngine/GameFont.h"

#ifdef CONFIG_PROFILE_LOCKS
static volatile LONG					critical_section_counter = 0;

void add_profile_portion				(LPCSTR id, const u64 &time)
{
	if (!*id)
		return;	
	if (!psDeviceFlags.test(rsStatistic))
		return;

	CProfileResultPortion			temp;
	temp.m_timer_id					= id;
	temp.m_time						= time;
	
	profiler().add_profile_portion	(temp);
}
#endif // CONFIG_PROFILE_LOCKS

CProfiler	*g_profiler			= 0;
LPCSTR		indent				= "  ";
char		white_character		= '.';

struct CProfilePortionPredicate {
	IC		bool operator()			(const CProfileResultPortion &_1, const CProfileResultPortion &_2) const
	{
		return					(xr_strcmp(_1.m_timer_id,_2.m_timer_id) < 0);
	}
};

CProfiler::CProfiler				()
#ifdef CONFIG_PROFILE_LOCKS
	:m_section("")
#endif // CONFIG_PROFILE_LOCKS
{
	m_actual							= true;
}

CProfiler::~CProfiler				()
{
#ifdef CONFIG_PROFILE_LOCKS
	set_add_profile_portion		(0);
#endif // CONFIG_PROFILE_LOCKS
}

IC	u32 compute_string_length		(LPCSTR str)
{
	LPCSTR						i, j = str;
	u32							count = 0;
	while ((i = strchr(j,'/')) != 0) {
		j = i					= i + 1;
		++count;
	}
	return						(count*xr_strlen(indent) + xr_strlen(j));
}

IC	void CProfiler::convert_string	(LPCSTR str, shared_str &out, u32 max_string_size)
{
	string256					m_temp;
	LPCSTR						i, j = str;
	u32							count = 0;
	while ((i = strchr(j,'/')) != 0) {
		j = i					= i + 1;
		++count;
	}
	xr_strcpy						(m_temp,"");
	for (u32 k = 0; k<count; ++k)
		xr_strcat					(m_temp,indent);
	xr_strcat						(m_temp,j);
	count						= xr_strlen(m_temp);
	for ( ; count < max_string_size; ++count)
		m_temp[count]			= white_character;
	m_temp[max_string_size]		= 0;
	out							= m_temp;
}

void CProfiler::setup_timer			(LPCSTR timer_id, const u64 &timer_time, const u32 &call_count)
{
	string256					m_temp;
	float						_time = float(timer_time)*1000.f/CPU::qpc_freq;
	TIMERS::iterator			i = m_timers.find(timer_id);
	if (i == m_timers.end()) {
		xr_strcpy					(m_temp,timer_id);
		LPSTR					j,k = m_temp;
		while ((j = strchr(k,'/')) != 0) {
			*j					= 0;
			TIMERS::iterator	m = m_timers.find(m_temp);
			if (m == m_timers.end())
				m_timers.insert	(std::make_pair(shared_str(m_temp),CProfileStats()));
			*j					= '/';
			k					= j + 1;
		}
		i						= m_timers.insert(std::make_pair(shared_str(timer_id),CProfileStats())).first;

		CProfileStats			&current = (*i).second;
		current.m_min_time		= _time;
		current.m_max_time		= _time;
		current.m_total_time	= _time;
		current.m_count			= 1;
		current.m_call_count	= call_count;
		m_actual				= false;
	}
	else {
		CProfileStats			&current = (*i).second;
		current.m_min_time		= _min(current.m_min_time,_time);
		current.m_max_time		= _max(current.m_max_time,_time);
		current.m_total_time	+= _time;
		++current.m_count;
		current.m_call_count	+= call_count;
	}

	if (_time > (*i).second.m_time)
		(*i).second.m_time		= _time;
	else
		(*i).second.m_time		= .01f*_time + .99f*(*i).second.m_time;

	(*i).second.m_update_time	= Device.dwTimeGlobal;
}

void CProfiler::clear				()
{
#ifdef CONFIG_PROFILE_LOCKS
	while (InterlockedExchange(&critical_section_counter,1))
		Sleep					(0);
#endif // CONFIG_PROFILE_LOCKS

	m_section.Enter				();
	m_portions.clear			();
	m_timers.clear				();
	m_section.Leave				();

	m_call_count				= 0;

#ifdef CONFIG_PROFILE_LOCKS
	InterlockedExchange			(&critical_section_counter,0);
#endif // CONFIG_PROFILE_LOCKS
}

void CProfiler::show_stats			(IGameFont &font, bool show)
{
	if (!show) {
#ifdef CONFIG_PROFILE_LOCKS
		set_add_profile_portion	(0);
#endif // CONFIG_PROFILE_LOCKS
		clear					();
		return;
	}

#ifdef CONFIG_PROFILE_LOCKS
	set_add_profile_portion		(&::add_profile_portion);
#endif // CONFIG_PROFILE_LOCKS

	++m_call_count;

#ifdef CONFIG_PROFILE_LOCKS
	while (InterlockedExchange(&critical_section_counter,1))
		Sleep					(0);
#endif // CONFIG_PROFILE_LOCKS

	m_section.Enter				();

	if (!m_portions.empty()) {
		std::sort				(m_portions.begin(),m_portions.end(),CProfilePortionPredicate());
		u64						timer_time = 0;
		u32						call_count = 0;

		PORTIONS::const_iterator	I = m_portions.begin(), J = I;
		PORTIONS::const_iterator	E = m_portions.end();
		for ( ; I != E; ++I) {
			if (xr_strcmp((*I).m_timer_id,(*J).m_timer_id)) {
				setup_timer		((*J).m_timer_id,timer_time,call_count);
				timer_time		= 0;
				call_count		= 0;
				J				= I;
			}

			++call_count;
			timer_time			+= (*I).m_time;
		}
		setup_timer				((*J).m_timer_id,timer_time,call_count);

		m_portions.clear		();

		m_section.Leave			();

		if (!m_actual) {
			u32					max_string_size = 0;
			TIMERS::iterator	I = m_timers.begin();
			TIMERS::iterator	E = m_timers.end();
			for ( ; I != E; ++I)
				max_string_size	= _max(max_string_size,compute_string_length(*(*I).first));

			I					= m_timers.begin();
			for ( ; I != E; ++I)
				convert_string	(*(*I).first,(*I).second.m_name,max_string_size);

			m_actual			= true;
		}
	}
	else
		m_section.Leave			();

#ifdef CONFIG_PROFILE_LOCKS
	InterlockedExchange			(&critical_section_counter,0);
#endif // CONFIG_PROFILE_LOCKS

	TIMERS::iterator			I = m_timers.begin();
	TIMERS::iterator			E = m_timers.end();
	for ( ; I != E; ++I) {
		if ((*I).second.m_update_time != Device.dwTimeGlobal)
			(*I).second.m_time	*= .99f;

		float					average = (*I).second.m_count ? (*I).second.m_total_time/float((*I).second.m_count) : 0.f;
		if (average >= (*I).second.m_time)
			font.SetColor	(color_xrgb(127,127,127));
		else
			font.SetColor	(color_xrgb(255,255,255));

		font.OutNext		(
//			"%s.. %8.3f %8.3f %8.3f %8.3f %8.3f %8d %12.3f",
			"%s%c%c %8.3f %8.3f %8.3f %6.1f %8d %12.3f",
			*(*I).second.m_name,
			white_character,
			white_character,
			(*I).second.m_time,
			average,
			(*I).second.m_max_time,
			float((*I).second.m_call_count)/m_call_count,//float((*I).second.m_count),
//			(*I).second.m_min_time,
			(*I).second.m_call_count,
			(*I).second.m_total_time
		);
	}

	font.SetColor			(color_xrgb(255,255,255));
}

void CProfiler::add_profile_portion	(const CProfileResultPortion &profile_portion)
{
#ifdef CONFIG_PROFILE_LOCKS
	if (InterlockedExchange(&critical_section_counter,1))
		return;

	do {
		Sleep					(0);
	}
	while (!InterlockedExchange(&critical_section_counter,1));
#endif // CONFIG_PROFILE_LOCKS

	m_section.Enter				();
	m_portions.push_back		(profile_portion);
	m_section.Leave				();

#ifdef CONFIG_PROFILE_LOCKS
	InterlockedExchange			(&critical_section_counter,0);
#endif // CONFIG_PROFILE_LOCKS
}
#endif
