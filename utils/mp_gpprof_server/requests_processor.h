#ifndef REQUESTS_PROCESSOR_INCLUDED
#define REQUESTS_PROCESSOR_INCLUDED

#include <vector>
#include <memory>
#include <fcgi_config.h>
#include <fcgio.h>
#include "threads.h"
#include "sake_worker.h"
#include "profile_request.h"
#include "profiles_cache.h"

class requests_poll
{
public:
			requests_poll	();
			~requests_poll	();
	typedef fetch_profile_request::request_ptr_t request_ptr_t;
	void	add_request		(request_ptr_t fcgx_request);
private:

	typedef std::vector<fetch_profile_request*>	requests_t;
	static bool requests_worker	(void* arg, sake_processor* sproc);
	
	bool	request_processor		(sake_processor* sproc);
	void	start_work				();
	void	add_new_request			(request_ptr_t fcgx_request,
									 user_name_t const profile_name);
	void	process_result			(sake_processor* sproc);

	xray::mutex			m_new_request_sync;
	xray::mutex			m_cache_sync;
	requests_t			m_new_requests;
	requests_t			m_active_requests;
	profiles_cache		m_profiles_cache;
	sake_worker			m_sake_worker;
};//class requests_poll

#endif//#ifndef REQUESTS_PROCESSOR_INCLUDED