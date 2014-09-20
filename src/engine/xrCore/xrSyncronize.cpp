#include "stdafx.h"

#ifdef PROFILE_CRITICAL_SECTIONS
static add_profile_portion_callback	add_profile_portion = 0;
void set_add_profile_portion	(add_profile_portion_callback callback)
{
	add_profile_portion		= callback;
}

struct profiler {
	u64						m_time;
	LPCSTR					m_timer_id;

	IC	profiler::profiler	(LPCSTR timer_id)
	{
		if (!add_profile_portion)
			return;

		m_timer_id			= timer_id;
		m_time				= CPU::QPC();
	}

	IC	profiler::~profiler	()
	{
		if (!add_profile_portion)
			return;

		u64					time = CPU::QPC();
		(*add_profile_portion)(m_timer_id,time - m_time);
	}
};
#endif // PROFILE_CRITICAL_SECTIONS

#ifdef PROFILE_CRITICAL_SECTIONS
xrCriticalSection::xrCriticalSection	(LPCSTR id) : m_id(id)
#else // PROFILE_CRITICAL_SECTIONS
xrCriticalSection::xrCriticalSection	()
#endif // PROFILE_CRITICAL_SECTIONS
{
	pmutex							= xr_alloc<CRITICAL_SECTION>(1);
	InitializeCriticalSection		( (CRITICAL_SECTION*)pmutex	);
}

xrCriticalSection::~xrCriticalSection	()
{
	DeleteCriticalSection			( (CRITICAL_SECTION*)pmutex	);
	xr_free							( pmutex		);
}

#ifdef DEBUG
	extern void OutputDebugStackTrace	(const char *header);
#endif // DEBUG

void	xrCriticalSection::Enter	()
{
#ifdef PROFILE_CRITICAL_SECTIONS
#	if 0//def DEBUG
		static bool					show_call_stack = false;
		if (show_call_stack)
			OutputDebugStackTrace	("----------------------------------------------------");
#	endif // DEBUG
	profiler						temp(m_id);
#endif // PROFILE_CRITICAL_SECTIONS
	EnterCriticalSection			( (CRITICAL_SECTION*)pmutex );
}

void	xrCriticalSection::Leave	()
{
	LeaveCriticalSection			( (CRITICAL_SECTION*)pmutex );
}

BOOL	xrCriticalSection::TryEnter	()
{
	return TryEnterCriticalSection	( (CRITICAL_SECTION*)pmutex );
}

xrCriticalSection::raii::raii		(xrCriticalSection* critical_section) 
									: critical_section(critical_section) 
{
	VERIFY(critical_section);
	critical_section->Enter();
}

xrCriticalSection::raii::~raii		()
{ 
	critical_section->Leave(); 
}
