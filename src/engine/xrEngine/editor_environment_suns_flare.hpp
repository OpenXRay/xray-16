////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_suns_flare.hpp
//	Created 	: 13.12.2007
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment suns flare class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_FLARE_HPP_INCLUDED
#define EDITOR_WEATHER_FLARE_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "../include/editor/property_holder.hpp"

namespace editor {
namespace environment {
namespace suns {

class flare :
	public editor::property_holder_holder,
	private boost::noncopyable
{
public:
						flare		();
	virtual				~flare		();
			void		fill		(editor::property_holder_collection* collection);

public:
	typedef editor::property_holder	property_holder;

public:
	virtual	property_holder*object	();

private:
	property_holder*	m_property_holder;

public:
	shared_str			m_texture;
    float				m_opacity;
    float				m_position;
    float				m_radius;
}; // class flare

} // namespace suns
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_FLARE_HPP_INCLUDED