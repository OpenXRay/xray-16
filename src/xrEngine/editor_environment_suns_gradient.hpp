////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_suns_gradient.hpp
//	Created 	: 26.01.2008
//  Modified 	: 26.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment suns gradient class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_SUNS_GRADIENT_HPP_INCLUDED
#define EDITOR_WEATHER_SUNS_GRADIENT_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>

namespace editor {

class property_holder;
class property_holder_collection;

namespace environment {
namespace suns {

class manager;

class gradient : private boost::noncopyable {
public:
					gradient	();
			void	load		(CInifile& config, shared_str const& section);
			void	save		(CInifile& config, shared_str const& section);
			void	fill		(manager const& manager, editor::property_holder* holder, editor::property_holder_collection* collection);

private:
	bool xr_stdcall	use_getter	();
	void xr_stdcall use_setter	(bool value);

private:
	bool			m_use;
    float			m_opacity;
    float			m_radius;
    shared_str		m_shader;
    shared_str		m_texture;
}; // class gradient

} // namespace suns
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_SUNS_GRADIENT_HPP_INCLUDED