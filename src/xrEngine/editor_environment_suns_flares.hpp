////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_suns_flares.hpp
//	Created 	: 26.01.2008
//  Modified 	: 26.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment suns flares class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_SUNS_FLARES_HPP_INCLUDED
#define EDITOR_WEATHER_SUNS_FLARES_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "property_collection_forward.hpp"

namespace editor {

class property_holder;
class property_holder_collection;

namespace environment {
namespace suns {

class manager;
class flare;

class flares : private boost::noncopyable {
public:
					flares	();
	virtual			~flares	();
			void	load	(CInifile& config, shared_str const& section);
			void	save	(CInifile& config, shared_str const& section);
			void	fill	(manager const& manager, editor::property_holder* holder, editor::property_holder_collection* collection);
private:
	typedef xr_vector<flare*>							flares_type;
	typedef editor::property_holder_collection			property_holder_collection;

public:
	typedef property_collection<flares_type, flares>	collection_type;

private:
	flares_type				m_flares;
    shared_str				m_shader;
	collection_type*		m_collection;
    bool					m_use;
}; // class flares

} // namespace suns
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_SUNS_FLARES_HPP_INCLUDED