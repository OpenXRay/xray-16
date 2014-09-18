////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_suns_blend.hpp
//	Created 	: 26.01.2008
//  Modified 	: 26.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment suns blend class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_SUNS_BLEND_HPP_INCLUDED
#define EDITOR_WEATHER_SUNS_BLEND_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>

namespace editor {

class property_holder;
class property_holder_collection;

namespace environment {
namespace suns {

class manager;

class blend : private boost::noncopyable {
public:
					blend		();
			void	load		(CInifile& config, shared_str const& section);
			void	save		(CInifile& config, shared_str const& section);
			void	fill		(manager const& manager, editor::property_holder* holder, editor::property_holder_collection* collection);

private:
    float			m_down_time;
    float			m_rise_time;
    float			m_time;
}; // class blend

} // namespace suns
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_SUNS_BLEND_HPP_INCLUDED