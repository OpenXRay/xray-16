#include "requests_processor.h"

extern char const * default_root_path;
extern char const * root_path;

static gsi_time	const gamespy_think_time	= 100;
static unsigned int wait_train_time_ms		= 100;
static unsigned int profiles_cache_size		= 1024 * 1024 * 32; //32 Mb
unsigned int cache_expire_time				= 1000 * 60 * 5; //5 min

requests_poll::requests_poll() :
	m_profiles_cache(profiles_cache_size)
{
	start_work();	
}

requests_poll::~requests_poll()
{
}

bool requests_poll::requests_worker(void* arg, sake_processor* sproc)
{
	requests_poll* me = static_cast<requests_poll*>(arg);
	return me->request_processor(sproc);
}

void requests_poll::add_request(request_ptr_t fcgx_request)
{
	using namespace gamespy_profile;
	user_name_t	tmp_user_name;
	char* requested_uri = FCGX_GetParam("REQUEST_URI", fcgx_request->envp);
	if (!requested_uri || !strlen(requested_uri))
	{
		std::cerr << "requested invalid url" << std::endl;
		FCGX_SetExitStatus(404, fcgx_request->out);
		FCGX_Finish_r(fcgx_request.get());
		return;
	}

	char const * rpath = root_path != NULL ? root_path : default_root_path;
	extract_username(requested_uri, rpath, tmp_user_name);
	if (!strlen(tmp_user_name))
	{
		std::cerr << "no user specified in request" << std::endl;
		FCGX_SetExitStatus(404, fcgx_request->out);
		FCGX_Finish_r(fcgx_request.get());
		return;
	}

	profile_data	tmp_profdata;
	m_cache_sync.lock();
	if (m_profiles_cache.search(tmp_user_name, tmp_profdata))
	{
		m_cache_sync.unlock();
		fetch_profile_request tmp_request(fcgx_request, tmp_user_name);
		tmp_request.complete_success(tmp_profdata);
		return;
	}
	m_cache_sync.unlock();

	//trying to search name in cache ..
	//adding request to processor queue ..
	add_new_request(fcgx_request, tmp_user_name);
}

void requests_poll::add_new_request(request_ptr_t fcgx_request,
									user_name_t const profile_name)
{
	m_new_request_sync.lock();
	m_new_requests.push_back(
		new fetch_profile_request(fcgx_request, profile_name));
	m_new_request_sync.unlock();	
}

void requests_poll::start_work()
{
	m_sake_worker.add_task(&requests_poll::requests_worker, this);
}

bool requests_poll::request_processor(sake_processor* sproc)
{
		
	bool is_active = false;
	m_new_request_sync.lock();
	is_active = !m_active_requests.empty();
	m_new_request_sync.unlock();
	
	if (is_active)
	{
		sproc->think(gamespy_think_time);
		if (sproc->is_result_ready())
		{
			process_result(sproc);
		} else
		{
			return true;
		}
	}

	if (m_new_requests.empty())
	{
		xray::sleep(wait_train_time_ms);
		return true;
	}
	
	m_new_request_sync.lock();
	sproc->begin_fetch();
	for (requests_t::iterator i = m_new_requests.begin(),
		ie = m_new_requests.end(); i != ie; ++i)
	{
		sproc->add_name((*i)->get_profile_name());
	}
	sproc->fetch();
	m_active_requests = m_new_requests;
	m_new_requests.clear();
	m_new_request_sync.unlock();
	
	return true;
}


void requests_poll::process_result(sake_processor* sproc)
{
	bool need_sort			= false;
	bool cleared_expired	= false;
	m_new_request_sync.lock();
	for (requests_t::iterator i = m_active_requests.begin(),
		ie = m_active_requests.end(); i != ie; ++i)
	{
		gamespy_profile::profile_data tmp_data;
		if (sproc->get_profile((*i)->get_profile_name(), tmp_data))
		{
			m_cache_sync.lock();
			need_sort |= m_profiles_cache.add((*i)->get_profile_name(), tmp_data);
			if (!need_sort && !cleared_expired)
			{
				m_profiles_cache.clear_expired();
				need_sort |= m_profiles_cache.add((*i)->get_profile_name(), tmp_data);
				cleared_expired = true;
			}
			m_cache_sync.unlock();
			(*i)->complete_success(tmp_data);
		} else
		{
			(*i)->complete_failed();
		}
		delete (*i);
	}
	m_active_requests.clear();
	m_new_request_sync.unlock();
	if (need_sort)
	{
		m_cache_sync.lock();
		m_profiles_cache.sort();
		m_cache_sync.unlock();
	}
}