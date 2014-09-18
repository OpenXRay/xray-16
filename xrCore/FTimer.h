#ifndef FTimerH
#define FTimerH
#pragma once

class	CTimer_paused;

class XRCORE_API				pauseMngr
{
	xr_vector<CTimer_paused*>	m_timers;
	BOOL						m_paused;
public:
			pauseMngr			();
	BOOL	Paused				(){return m_paused;};
	void	Pause				(BOOL b);
	void	Register			(CTimer_paused* t);
	void	UnRegister			(CTimer_paused* t);
};

extern XRCORE_API pauseMngr		g_pauseMngr;

class XRCORE_API CTimerBase		{
protected:
	u64			qwStartTime		;
	u64			qwPausedTime	;
	u64			qwPauseAccum	;
	BOOL		bPause			;
public:
				CTimerBase		()		: qwStartTime(0),qwPausedTime(0),qwPauseAccum(0),bPause(FALSE)		{ }
	ICF	void	Start			()		{	if(bPause) return;	qwStartTime = CPU::QPC()-qwPauseAccum;		}
	ICF u64		GetElapsed_ticks()const	{	if(bPause) return	qwPausedTime; else return CPU::QPC()-qwStartTime-CPU::qpc_overhead-qwPauseAccum; }
	IC	u32		GetElapsed_ms	()const	{	return u32(GetElapsed_ticks()*u64(1000)/CPU::qpc_freq );	}
	IC	float	GetElapsed_sec	()const	{
#ifndef _EDITOR
		FPU::m64r	()			;
#endif        
		float		_result		=		float(double(GetElapsed_ticks())/double(CPU::qpc_freq )	)	;
#ifndef _EDITOR
		FPU::m24r	()			;
#endif
		return		_result		;
	}
	IC	void	Dump			() const
	{
		Msg("* Elapsed time (sec): %f",GetElapsed_sec());
	}
};

class XRCORE_API CTimer : public CTimerBase {
private:
	typedef CTimerBase					inherited;

private:
	float				m_time_factor;
	u64					m_real_ticks;
	u64					m_ticks;

private:
	IC	u64				GetElapsed_ticks(const u64 &current_ticks) const
	{
		u64				delta = current_ticks - m_real_ticks;
		double			delta_d = (double)delta;
		double			time_factor_d = time_factor();
		double			time = delta_d*time_factor_d + .5;
		u64				result = (u64)time;
		return			(m_ticks + result);
	}

public:
	IC					CTimer			() : m_time_factor(1.f), m_real_ticks(0), m_ticks(0) {}

	ICF	void			Start			()
	{
		if (bPause)
			return;

		inherited::Start();

		m_real_ticks	= 0;
		m_ticks			= 0;
	}

	IC	const float		&time_factor	() const
	{
		return			(m_time_factor);
	}

	IC	void			time_factor		(const float &time_factor)
	{
		u64				current = inherited::GetElapsed_ticks();
		m_ticks			= GetElapsed_ticks(current);
		m_real_ticks	= current;
		m_time_factor	= time_factor;
	}

	IC	u64				GetElapsed_ticks() const
	{
#ifndef _EDITOR
		FPU::m64r		();
#endif // _EDITOR

		u64				result = GetElapsed_ticks(inherited::GetElapsed_ticks());

#ifndef _EDITOR
		FPU::m24r		();
#endif // _EDITOR

		return			(result);
	}

	IC	u32				GetElapsed_ms	() const
	{
		return			(u32(GetElapsed_ticks()*u64(1000)/CPU::qpc_freq));
	}
	
	IC	float			GetElapsed_sec	() const
	{
#ifndef _EDITOR
		FPU::m64r		();
#endif        
		float			result = float(double(GetElapsed_ticks())/double(CPU::qpc_freq )	)	;
#ifndef _EDITOR
		FPU::m24r		();
#endif
		return			(result);
	}

	IC	void			Dump			() const
	{
		Msg				("* Elapsed time (sec): %f",GetElapsed_sec());
	}
};

class XRCORE_API CTimer_paused_ex : public CTimer		{
	u64							save_clock;
public:
	CTimer_paused_ex			()		{ }
	virtual ~CTimer_paused_ex	()		{ }
	IC BOOL		Paused			()const	{ return bPause;				}
	IC void		Pause			(BOOL b){
		if(bPause==b)			return	;

		u64		_current		=		CPU::QPC()-CPU::qpc_overhead	;
		if( b )	{
			save_clock			= _current				;
			qwPausedTime		= CTimerBase::GetElapsed_ticks()	;
		}else	{
			qwPauseAccum		+=		_current - save_clock;
		}
		bPause = b;
	}
};

class XRCORE_API CTimer_paused  : public CTimer_paused_ex		{
public:
	CTimer_paused				()		{ g_pauseMngr.Register(this);	}
	virtual ~CTimer_paused		()		{ g_pauseMngr.UnRegister(this);	}
};

extern XRCORE_API BOOL			g_bEnableStatGather;
class XRCORE_API CStatTimer
{
public:
	CTimer		T;
	u64			accum;
	float		result;
	u32			count;
public:
				CStatTimer		();
	void		FrameStart		();
	void		FrameEnd		();

	ICF void	Begin			()		{	if (!g_bEnableStatGather) return;	count++; T.Start();				}
	ICF void	End				()		{	if (!g_bEnableStatGather) return;	accum += T.GetElapsed_ticks();	}

	ICF u64		GetElapsed_ticks()const	{	return accum;					}

	IC	u32		GetElapsed_ms	()const	{	return u32(GetElapsed_ticks()*u64(1000)/CPU::qpc_freq );	}
	IC	float	GetElapsed_sec	()const	{
#ifndef _EDITOR
		FPU::m64r	()			;
#endif        
		float		_result		=		float(double(GetElapsed_ticks())/double(CPU::qpc_freq )	)	;
#ifndef _EDITOR
		FPU::m24r	()			;
#endif
		return		_result		;
	}
};

#endif // FTimerH
