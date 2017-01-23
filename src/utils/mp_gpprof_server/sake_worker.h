#ifndef SAKE_WORKER_INCLUDED
#define SAKE_WORKER_INCLUDED

#include "gamespy_sake.h"
#include "threads.h"
#include <deque>

class sake_worker
{
public:
	sake_worker		();
	~sake_worker	();
	
	typedef bool	(*sake_task_proc_t)(void*, sake_processor*);
	void add_task	(sake_task_proc_t proc, void* arg);
private:
	typedef xray::thread_method<sake_worker>	thread_t;
	typedef std::pair<sake_task_proc_t, void*>	task_item_t;
	typedef std::deque<task_item_t>				tasks_queue_t;
	
	volatile long				m_thread_initialized;
	volatile long				m_initialization_success;
	//WARNING ! do not change order of members !
	xray::mutex					m_newtask_mutex;
	xray::condition				m_newtask_cond;
	tasks_queue_t				m_tasks;
	std::auto_ptr<thread_t>		m_thread;

	void*		worker_thread	();	
	static bool thread_stopper	(void*, sake_processor*) { return false; };
}; //class sake_worker

#endif//#ifndef SAKE_WORKER_INCLUDED
