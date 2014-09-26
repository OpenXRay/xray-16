////////////////////////////////////////////////////////////////////////////
//	Module 		: property_incrementable.hpp
//	Created 	: 26.12.2007
//  Modified 	: 26.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property incrementable
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_INCREMENTABLE_HPP_INCLUDED
#define PROPERTY_INCREMENTABLE_HPP_INCLUDED

namespace editor {
namespace controls {

public interface class property_incrementable {
public:
	virtual void	increment	(float const% increment) = 0;
}; // interface class property_value

} // namespace controls
} // namespace editor

#endif // ifndef PROPERTY_INCREMENTABLE_HPP_INCLUDED