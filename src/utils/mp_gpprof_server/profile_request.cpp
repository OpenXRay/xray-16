#include <stdlib.h>
#include "profile_request.h"
#include <common/gsStringUtil.h>
#include "profile_printer.h"

template <int ArraySize>
inline char* decode_url(char const (&src_url)[ArraySize], char (&dst_url)[ArraySize])
{
	size_t src_length = strlen(src_url);
	if (src_length > (ArraySize - 1))
	{
		src_length = ArraySize - 1;
	}
	char	number[3];
	char*	pend = NULL;
	size_t i	= 0;
	size_t di	= 0;

	while (i < src_length)
	{
		char cvt_char;
		if (src_url[i] == '%')
		{
			++i;
			if ((i + 2) > src_length)
				break;
			strncpy(number, &src_url[i], 2);
			number[2] = 0;
			cvt_char = (char)strtoul(number, &pend, 16);
			i += 2;
		} else
		{
			cvt_char = src_url[i];
			++i;
		}
		dst_url[di] = cvt_char;
		++di;
	}
	dst_url[di] = 0;
	return dst_url;
}


char* extract_username(char const * uri, char const * root_path, user_name_t & dst_user)
{
	typedef char url_string_t[256];
	url_string_t tmp_format;
	sprintf(tmp_format, "%s/%%255s", root_path); // for url_string_t[256];
	url_string_t tmp_uname;
	strcpy(tmp_uname, "");
	
	if (sscanf(uri, tmp_format, tmp_uname))
	{
		url_string_t decoded_uname;
		strncpy(dst_user,
			decode_url(tmp_uname, decoded_uname),
			sizeof(dst_user) - 1);
		dst_user[sizeof(dst_user) - 1] = 0;
	} else
	{
		strcpy(dst_user, "");
	}
	return dst_user;
}

fetch_profile_request::fetch_profile_request(request_ptr_t fcgx_request,
											 user_name_t const profile_name) :
	m_fcg_request(fcgx_request),
	m_profile_name(profile_name),
	m_in(m_fcg_request->in),
	m_out(m_fcg_request->out),
	m_err(m_fcg_request->err)
{
}

fetch_profile_request::~fetch_profile_request()
{
};

void fetch_profile_request::complete_success(gamespy_profile::profile_data const & profdata)
{
	m_out << crlf << m_profile_name << crlf
		<< profdata << crlf;
	FCGX_SetExitStatus(200, m_fcg_request->out);
	FCGX_Finish_r(m_fcg_request.get());
}

void fetch_profile_request::complete_failed	()
{
	FCGX_SetExitStatus(404, m_fcg_request->out);
	FCGX_Finish_r(m_fcg_request.get());
}
