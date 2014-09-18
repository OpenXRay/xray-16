#include <iostream>
#include "sake_worker.h"

sake_worker::sake_worker() :
	m_thread_initialized(0),
	m_initialization_success(0)
{
	m_thread.reset(new thread_t(&sake_worker::worker_thread, this));
	sched_yield();
	while (!m_thread_initialized)
	{
		sched_yield();
	}
	if (!m_initialization_success)
		throw std::runtime_error("failed to initialize sake_worker");
}

sake_worker::~sake_worker()
{
	add_task(&sake_worker::thread_stopper, NULL);
}

void sake_worker::add_task(sake_task_proc_t proc, void* arg)
{
	m_newtask_mutex.lock();
	m_tasks.push_back(std::make_pair(proc, arg));
	m_newtask_cond.signal();
	m_newtask_mutex.unlock();
}

void* sake_worker::worker_thread()
{
	using namespace std;
	try {
		std::auto_ptr<sake_processor> m_sake_inst(new sake_processor());
		m_thread_initialized		= 1;
		m_initialization_success	= 1;
		bool stop_signal = false;
		while (!stop_signal)
		{
			m_newtask_mutex.lock	();
			while (m_tasks.empty())
				m_newtask_cond.wait(m_newtask_mutex);

			while (!m_tasks.empty())
			{
				task_item_t & tmp_item = m_tasks.front();
				if (tmp_item.first == thread_stopper)
				{
					stop_signal = true;
					break;
				}
				if (tmp_item.first(tmp_item.second, m_sake_inst.get()))
				{
					m_tasks.push_back(tmp_item);
				}
				m_tasks.pop_front();
			}
			m_newtask_mutex.unlock	();
		}
	} catch (std::exception const & e)
	{
		m_thread_initialized		= 1;
		m_initialization_success	= 0;
		cerr << "Caught exception: " << e.what() << endl
			<< "Type: " << typeid(e).name() << endl;
	}
	return NULL;
}