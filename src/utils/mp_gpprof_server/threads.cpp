#include <stdexcept>
#include "threads.h"

namespace xray
{

mutex::mutex()
{
	if (pthread_mutex_init(&m_mutex, NULL) != 0)
		throw std::runtime_error("failed to initialize mutex");
}
mutex::~mutex()
{
	pthread_mutex_destroy(&m_mutex);
}

void mutex::lock()
{
	pthread_mutex_lock(&m_mutex);
}
bool mutex::try_lock()
{
	return (pthread_mutex_trylock(&m_mutex) == 0);
}
void mutex::unlock()
{
	pthread_mutex_unlock(&m_mutex);
}

condition::condition()
{
	if (pthread_cond_init(&m_cond, NULL) != 0)
		throw std::runtime_error("failed to initialize condition variable");
}

condition::~condition()
{
	pthread_cond_destroy(&m_cond);
}

void condition::wait(mutex & m)
{
	pthread_cond_wait(&m_cond, m.get());
}
void condition::signal()
{
	pthread_cond_signal(&m_cond);
}

unsigned int const get_clock_ms()
{
	return (clock() / CLOCKS_PER_SEC) * 1000;
}

void sleep(unsigned int const ms)
{
	timespec tmp_ts;
	timespec tmp_tsrem;
	tmp_ts.tv_sec	= ms / 1000;
	tmp_ts.tv_nsec	= (ms % 1000) * 1000;
	nanosleep(&tmp_ts, &tmp_tsrem);
}

} //namespace xray
