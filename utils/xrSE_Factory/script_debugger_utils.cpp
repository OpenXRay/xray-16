#include "stdafx.h"

#include "script_debugger_utils.h"
/*
#ifndef _WIN32
#	include <sys/time.h>
#	include <errno.h>
#endif*/

xr_event::xr_event(bool broadcast, bool signalled )
{
	m_event = CreateEvent(0, broadcast, signalled, 0);
}

xr_event::~xr_event()
{
	CloseHandle(m_event);
}

bool xr_event::signal()
{
	return SetEvent(m_event) != 0;
}

bool xr_event::reset()
{
	return ResetEvent(m_event) != 0;
}

xrEventWaitRes xr_event::wait()
{
	if (WaitForSingleObject(m_event, INFINITE) == WAIT_OBJECT_0)
		return wrSignaled;
	else
		return wrError;
}

xrEventWaitRes xr_event::wait(unsigned msec)
{
	DWORD res = WaitForSingleObject(m_event, msec);
	if (res == WAIT_OBJECT_0)
		return wrSignaled;
	if (res == WAIT_TIMEOUT)
		return wrTimeOut;
	return wrError;
}
////////////////////////////////////////////////////////////

xr_mutex::xr_mutex()
{
	InitializeCriticalSection(&m_mutex);
}

xr_mutex::~xr_mutex()
{
	DeleteCriticalSection(&m_mutex);
}

bool xr_mutex::trylock()
{
	return TryEnterCriticalSection(&m_mutex) != 0;
}

bool xr_mutex::lock()
{
	EnterCriticalSection(&m_mutex);
	return true;
}

bool xr_mutex::unlock()
{
	LeaveCriticalSection(&m_mutex);
	return true;
}

xr_sync::xr_sync(xr_mutex & mutex)
: m_mutex(mutex)
{
	m_mutex.lock();
}

xr_sync::~xr_sync()
{
	m_mutex.unlock();
}

////////////////////////////////////////////////////////////
DWORD __stdcall xrThreadStart(void * th)
{
	((xr_thread*)th)->run();
	return 0;
}

xr_thread::xr_thread()
: m_thread(0)
{}

xr_thread::~xr_thread()
{
	if (m_thread) 
		CloseHandle(m_thread);
}

bool xr_thread::start()
{
	kill();
	DWORD dwID;
	m_thread = CreateThread(0, 0, &xrThreadStart, (void*)this, 0, &dwID);
	return m_thread != 0;
}

bool xr_thread::kill()
{
	if (!m_thread) return true;

	BOOL res = TRUE;
	if (WaitForSingleObject(m_thread, 0) != WAIT_OBJECT_0)
		res = TerminateThread(m_thread, 0);
	if (res)
	{
		CloseHandle(m_thread);
		m_thread = 0;
	}
	return res != 0;
}

bool xr_thread::join()
{
	return (WaitForSingleObject(m_thread, INFINITE) == WAIT_OBJECT_0);
}

bool xr_thread::yield()
{
	Sleep(0);
	return true;
}

void xr_thread::sleep(unsigned msec)
{
	Sleep(msec);
}

/*
pid_t xr_thread::getId()
{
	return GetCurrentThreadId();
}*/

unsigned xr_thread::getTickCount()
{
	return GetTickCount();
}


xr_waitableThread::xr_waitableThread()
: m_event(true, true)
{}

xr_waitableThread::~xr_waitableThread()
{}

bool xr_waitableThread::start()
{
	m_event.reset();

	if (xr_thread::start())
		return true;
	else
	{
		m_event.signal();
		return false;
	}
}

bool xr_waitableThread::kill()
{
	if (xr_thread::kill())
	{
		m_event.signal();
		return true;
	}
	else
		return false;
}

bool xr_waitableThread::join()
{
	return xr_thread::join();
}

bool xr_waitableThread::join(unsigned msec)
{
	return m_event.wait(msec) == wrSignaled;
}

bool xr_waitableThread::yield()
{
	return xr_thread::yield();
}

void xr_waitableThread::run()
{
	run_w();
	m_event.signal();
}
