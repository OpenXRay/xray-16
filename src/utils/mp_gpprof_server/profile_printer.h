#ifndef PROFILE_PRINTER_INCLUDED
#define PROFILE_PRINTER_INCLUDED

#include "profile_data_types.h"
#include <iostream>

template<typename Traits>
inline std::basic_ostream<char, Traits> & crlf(std::basic_ostream<char, Traits> & ostr)
{
	ostr << "\r\n";
	return ostr;
}

template<typename Traits>
inline std::basic_ostream<char, Traits> & operator << (
	std::basic_ostream<char, Traits> & ostr,
	gamespy_profile::profile_data const & profile_data)
{
	using namespace gamespy_profile;
	for (int i = 0; i < at_awards_count; ++i)
	{
		char const * tmp_award_name = get_award_name(enum_awards_t(i));
		ostr << tmp_award_name << "=" 
			<< profile_data.m_awards[i].m_count << crlf
			<< tmp_award_name << "_rdate=" 
			<< profile_data.m_awards[i].m_last_reward_date << crlf;
	}
	for (int i = 0; i < bst_score_types_count; ++i)
	{
		char const * tmp_score_name = get_best_score_name(enum_best_score_type(i));
		ostr << tmp_score_name << "="
			<< profile_data.m_best_scores[i] << crlf;
	}
	return ostr;
}

#endif//#ifndef PROFILE_PRINTER_INCLUDED