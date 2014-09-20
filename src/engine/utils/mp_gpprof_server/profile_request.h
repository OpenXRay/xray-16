#ifndef PROFILE_REQUEST_INCLUDED
#define PROFILE_REQUEST_INCLUDED

#include <fcgi_config.h>
#include <fcgio.h>
#include <iostream>
#include "profile_data_types.h"

#define MAX_URI_SIZE 256
#define MAX_USER_NAME 128
typedef char request_uri_t[MAX_URI_SIZE];
typedef char user_name_t[MAX_USER_NAME];

char* extract_username(char const * uri, char const * root_path, user_name_t & dst_user);

class fetch_profile_request
{
public:
	typedef std::auto_ptr<FCGX_Request> request_ptr_t;

			fetch_profile_request	(request_ptr_t fcgx_request,
									 user_name_t const profile_name);
			~fetch_profile_request	();
	
	char const *	get_profile_name	() const { return m_profile_name.c_str(); };
	void			complete_success	(gamespy_profile::profile_data const & profdata);
	void			complete_failed		();
private:
	fetch_profile_request	() :
	   m_in(NULL),
	   m_out(NULL),
	   m_err(NULL)
	{
	}
	
	request_ptr_t			m_fcg_request;
	std::string				m_profile_name;
	fcgi_istream			m_in;
	fcgi_ostream			m_out;
	fcgi_ostream			m_err;
};//class fetch_profile_request

#endif//#ifndef PROFILE_REQUEST_INCLUDED