#ifndef THREADS_H_INCLUDED
#define THREADS_H_INCLUDED

#include <iostream>
#include <stdexcept>
#include <typeinfo>
#include <pthread.h>
#include <sched.h>
#include <signal.h>

namespace xray
{

class mutex
{
public:
			mutex		();
			~mutex		();
	void	lock		();
	bool	try_lock	();
	void	unlock		();
	pthread_mutex_t* get() { return &m_mutex; }
private:
	pthread_mutex_t	m_mutex;
};//class mutex

class condition
{
public:
			condition	();
			~condition	();
	void	wait		(mutex & m);
	void	signal		();
private:
	pthread_cond_t	m_cond;
}; //class condition


template<typename T>
class thread_method
{
public:
	thread_method	(void* (T::*method)(), T* this_ptr)
	{
		m_method	= method;
		m_this_ptr	= this_ptr;
		pthread_attr_init(&m_attr);
		pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);
		m_thread_is_running = true;
		if (pthread_create(&m_pid, &m_attr, worker, this) != 0)
		{
			std::string err_string("failed to start thread: ");
			err_string.append(typeid(method).name());
			throw std::runtime_error(err_string);
		}
		pthread_detach(m_pid);
	}

	~thread_method	()
	{
		m_stop_mutex.lock();
		if (m_thread_is_running)
		{
			pthread_kill(m_pid, SIGTERM);
			while (m_thread_is_running)
				m_stopped_cond.wait(m_stop_mutex);
		}
		m_stop_mutex.unlock();
	}
	//pthread_t const get_pid() const { return m_pid; };
private:
	thread_method() {};
	typedef void* (T::*method_t)();
	
	pthread_t		m_pid;
	pthread_attr_t	m_attr;
	T*				m_this_ptr;
	method_t		m_method;

	mutex			m_stop_mutex;
	condition		m_stopped_cond;
	volatile long	m_thread_is_running;

	static void* worker(void* arg)
	{
		thread_method* me			= static_cast<thread_method*>(arg);
		assert(me);
		T* this_ptr			= me->m_this_ptr;
		assert(me->m_this_ptr);
		void* ret_value		= NULL;
		try
		{
			ret_value = (me->m_this_ptr->*(me->m_method))();
		} catch (...)
		{
			std::cerr << "Caught unknown exception: thread: " <<
				typeid(thread_method).name() << std::endl;
		}
		me->m_stop_mutex.lock();
		me->m_stopped_cond.signal();
		me->m_thread_is_running = false;
		me->m_stop_mutex.unlock();
		pthread_exit(ret_value);
		return ret_value;
	};
};//class thread_method

unsigned int const get_clock_ms();
void sleep(unsigned int const ms);

}//namespace xray

#endif //#ifndef THREADS_H_INCLUDED
