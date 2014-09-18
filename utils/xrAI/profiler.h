////////////////////////////////////////////////////////////////////////////
//	Module 		: profiler.h
//	Created 	: 23.07.2004
//  Modified 	: 23.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Profiler
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef XRGAME_EXPORTS
#	ifdef DEBUG
#		define	USE_PROFILER
#	endif // DEBUG
#endif // XRGAME_EXPORTS

#ifdef USE_PROFILER
#	include "ai_debug.h"

#ifdef PROFILE_CRITICAL_SECTIONS
	extern void add_profile_portion(LPCSTR id, const u64 &time);
#endif // PROFILE_CRITICAL_SECTIONS

#pragma pack(push,4)
struct CProfileResultPortion {
	u64				m_time;
	LPCSTR			m_timer_id;
};
#pragma pack(pop)

struct CProfilePortion : public CProfileResultPortion {
	IC				CProfilePortion		(LPCSTR timer_id);
	IC				~CProfilePortion	();
};

struct CProfileStats {
	u32				m_update_time;
	shared_str		m_name;
	float			m_time;
	float			m_min_time;
	float			m_max_time;
	float			m_total_time;
	u32				m_count;
	u32				m_call_count;

	IC				CProfileStats		();
};

class CProfiler {
private:
	struct pred_rstr {
		IC	bool operator()	(const shared_str &_1, const shared_str &_2) const
		{
			return	(xr_strcmp(*_1,*_2) < 0);
		}
	};
protected:
	typedef xr_vector<CProfileResultPortion>		PORTIONS;
	typedef xr_map<shared_str,CProfileStats,pred_rstr>	TIMERS;

protected:
	PORTIONS			m_portions;
	TIMERS				m_timers;
	bool				m_actual;
	xrCriticalSection	m_section;
	u32					m_call_count;

protected:
			void		setup_timer			(LPCSTR timer_id, const u64 &timer_time, const u32 &call_count);
	IC		void		convert_string		(LPCSTR str, shared_str &out, u32 max_string_size);

public:
						CProfiler			();
						~CProfiler			();
			void		show_stats			(CGameFont *game_font, bool show);
			void		clear				();
			void		add_profile_portion	(const CProfileResultPortion &profile_portion);
};

extern 	CProfiler *g_profiler;
extern Flags32 psAI_Flags;

IC	CProfiler&	profiler();
		
#	define START_PROFILE(a) { CProfilePortion	__profile_portion__(a);
#	define STOP_PROFILE     }

#	include "profiler_inline.h"

#else // DEBUG
#	define START_PROFILE(a) {
#	define STOP_PROFILE		}
#endif // DEBUG