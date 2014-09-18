////////////////////////////////////////////////////////////////////////////
//	Module 		: property_mouse_events.hpp
//	Created 	: 23.01.2008
//  Modified 	: 23.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property mouse events
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_MOUSE_EVENTS_HPP_INCLUDED
#define PROPERTY_MOUSE_EVENTS_HPP_INCLUDED

namespace editor {
namespace controls {

ref class property_grid;

public interface class property_mouse_events {
public:
	virtual void	on_double_click	(property_grid^ property_grid) = 0;
}; // interface class property_mouse_events

} // namespace controls
} // namespace editor

#endif // ifndef PROPERTY_MOUSE_EVENTS_HPP_INCLUDED