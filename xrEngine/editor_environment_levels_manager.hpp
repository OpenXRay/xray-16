////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_levels_manager.hpp
//	Created 	: 28.12.2007
//  Modified 	: 28.12.2007
//	Author		: Dmitriy Iassenev
//	Description : editor environment levels manager class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_LEVELS_MANAGER_HPP_INCLUDED
#define EDITOR_WEATHER_LEVELS_MANAGER_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "../xrserverentities/associative_vector.h"

namespace editor {

	class property_holder;

namespace environment {

	namespace weathers {
		class manager;
	} // namespace weathers

namespace levels {

class manager : private boost::noncopyable {
public:
							manager			(::editor::environment::weathers::manager* environment);
							~manager		();
			void			load			();
			void			save			();
			void			fill			();

private:
			void			fill_levels		(CInifile& config, LPCSTR prefix, LPCSTR category);

private:
	LPCSTR const* xr_stdcall collection		();
	u32  xr_stdcall			collection_size	();

private:
	struct predicate {
		inline	bool	operator()			(shared_str const& left, shared_str const& right) const
		{
			return			(xr_strcmp(left.c_str(), right.c_str()) < 0);
		}
	}; // struct predicate

	typedef associative_vector<shared_str, std::pair<LPCSTR, shared_str>, predicate>	levels_container_type;

private:
	levels_container_type						m_levels;
	::editor::environment::weathers::manager&	m_weathers;
	CInifile*									m_config_single;
	CInifile*									m_config_mp;
	editor::property_holder*					m_property_holder;
}; // class levels_manager

} // namespace levels
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_LEVELS_MANAGER_HPP_INCLUDED