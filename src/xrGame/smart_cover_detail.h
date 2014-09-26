////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_detail.h
//	Created 	: 17.08.2007
//	Author		: Alexander Dudin
//	Description : Smart cover auxillary namespace
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_DETAIL_H_INCLUDED
#define SMART_COVER_DETAIL_H_INCLUDED

#include "restriction_space.h"
#include "script_space_forward.h"

namespace smart_cover {

	namespace detail {

		typedef RestrictionSpace::CTimeIntrusiveBase	intrusive_base_time;
		
		float	parse_float		(
					luabind::object const &table,
					LPCSTR identifier,
					float const &min_threshold = flt_min,
					float const &max_threshold = flt_max
				);
		LPCSTR	parse_string	(luabind::object const &table, LPCSTR identifier);
		void	parse_table		(luabind::object const &table, LPCSTR identifier, luabind::object &result);
		bool	parse_bool		(luabind::object const &table, LPCSTR identifier);
		int		parse_int		(luabind::object const &table, LPCSTR identifier);
		Fvector	parse_fvector	(luabind::object const &table, LPCSTR identifier);

	}; //namespace detail

} //namespace smart_cover

#endif //SMART_COVER_DETAIL_H_INCLUDED