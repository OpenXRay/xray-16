////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_suns_manager.hpp
//	Created 	: 13.12.2007
//  Modified 	: 13.12.2007
//	Author		: Dmitriy Iassenev
//	Description : editor environment suns manager class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_SUNS_MANAGER_HPP_INCLUDED
#define EDITOR_WEATHER_SUNS_MANAGER_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "property_collection_forward.hpp"

class CLensFlareDescriptor;

namespace editor {

class property_holder;

namespace environment {

class manager;

namespace suns {

class sun;

class manager : private boost::noncopyable {
public:
					manager			(environment::manager* environment);
					~manager		();
			void	load			();
			void	save			();
			void	fill			(editor::property_holder* holder);
			shared_str	unique_id	(shared_str const& id) const;
	CLensFlareDescriptor* get_flare	(shared_str const& id) const;

private:
			void	add				(CInifile& config, shared_str const& sun);

public:
	typedef	xr_vector<sun*>			container_type;
	typedef xr_vector<LPSTR>		suns_ids_type;

public:
	suns_ids_type const&	suns_ids() const;

private:
	typedef property_collection<
				container_type,
				manager
			>						collection_type;


private:
	container_type					m_suns;
	mutable suns_ids_type			m_suns_ids;
	collection_type*				m_collection;
	mutable bool					m_changed;

public:
	environment::manager const&		m_environment;
}; // class suns_manager

} // namespace suns
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_SUNS_MANAGER_HPP_INCLUDED