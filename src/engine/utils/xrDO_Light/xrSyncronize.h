#ifndef xrSyncronizeH
#define xrSyncronizeH
#pragma once

#if 0//def DEBUG
#	define PROFILE_CRITICAL_SECTIONS
#endif // DEBUG

#ifdef PROFILE_CRITICAL_SECTIONS
	typedef void	(*add_profile_portion_callback)	(LPCSTR id, const u64 &time);
	void XRCORE_API	set_add_profile_portion			(add_profile_portion_callback callback);

#	define STRINGIZER_HELPER(a)		#a
#	define STRINGIZER(a)			STRINGIZER_HELPER(a)
#	define CONCATENIZE_HELPER(a,b)	a##b
#	define CONCATENIZE(a,b)			CONCATENIZE_HELPER(a,b)
#	define MUTEX_PROFILE_PREFIX_ID	#mutexes/
#	define MUTEX_PROFILE_ID(a)		STRINGIZER(CONCATENIZE(MUTEX_PROFILE_PREFIX_ID,a))
#endif // PROFILE_CRITICAL_SECTIONS

// Desc: Simple wrapper for critical section
class XRCORE_API		xrCriticalSection
{
private:
	void*				pmutex;
#ifdef PROFILE_CRITICAL_SECTIONS
	LPCSTR				m_id;
#endif // PROFILE_CRITICAL_SECTIONS

public:
#ifdef PROFILE_CRITICAL_SECTIONS
    xrCriticalSection	(LPCSTR id);
#else // PROFILE_CRITICAL_SECTIONS
    xrCriticalSection	();
#endif // PROFILE_CRITICAL_SECTIONS
    ~xrCriticalSection	();

    void				Enter	();
    void				Leave	();
	BOOL				TryEnter();
};

#endif // xrSyncronizeH