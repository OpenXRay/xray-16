#include "stdafx.h"
#include "Event.hpp"
#if defined(WINDOWS)

Event::Event() noexcept { handle = (void*)CreateEvent(NULL, FALSE, FALSE, NULL); }
Event::~Event() noexcept { CloseHandle(handle); }
void Event::Reset() noexcept { ResetEvent(handle); }
void Event::Set() noexcept { SetEvent(handle); }
void Event::Wait() noexcept { WaitForSingleObject(handle, INFINITE); }
bool Event::Wait(u32 millisecondsTimeout) noexcept
{
    return WaitForSingleObject(handle, millisecondsTimeout) != WAIT_TIMEOUT;
}
#elif defined(LINUX)
#include <pthread.h>
Event::Event() noexcept
{
	m_id.signaled = false;
	pthread_mutex_init(&m_id.mutex, nullptr);
	pthread_cond_init(&m_id.cond, nullptr);
}
Event::~Event() noexcept
{
	pthread_mutex_destroy(&m_id.mutex);
	pthread_cond_destroy(&m_id.cond);
}
void Event::Reset() noexcept
{
	pthread_mutex_lock(&m_id.mutex);
	pthread_cond_signal(&m_id.cond);
	m_id.signaled = false;
	pthread_mutex_unlock(&m_id.mutex);
}
void Event::Set() noexcept
{
	pthread_mutex_lock(&m_id.mutex);
	pthread_cond_signal(&m_id.cond);
	m_id.signaled = true;
	pthread_mutex_unlock(&m_id.mutex);
}
void Event::Wait() noexcept
{
	pthread_mutex_lock(&m_id.mutex);

	while (!m_id.signaled)
	{
		pthread_cond_wait(&m_id.cond, &m_id.mutex);
	}

	pthread_mutex_unlock(&m_id.mutex);
}
bool Event::Wait(u32 millisecondsTimeout) noexcept
{
	bool result = false;
	pthread_mutex_lock(&m_id.mutex);

	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += (long) millisecondsTimeout * 1000 * 1000;
	if(ts.tv_nsec > 1000000000)
	{
		ts.tv_nsec -= 1000000000;
		ts.tv_sec += 1;
	}

	while(!m_id.signaled)
	{
		int res = pthread_cond_timedwait(&m_id.cond, &m_id.mutex, &ts);
		if(res == ETIMEDOUT)
		{
			result = true;
			break;
		}
	}

	pthread_mutex_unlock(&m_id.mutex);

	return result;
}
#endif
