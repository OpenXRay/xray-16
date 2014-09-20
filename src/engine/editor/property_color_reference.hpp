////////////////////////////////////////////////////////////////////////////
//	Module 		: property_color_reference.hpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : color property reference implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLOR_REFERENCE_HPP_INCLUDED
#define PROPERTY_COLOR_REFERENCE_HPP_INCLUDED

#include "property_color_base.hpp"

public ref class property_color_reference : public property_color_base {
public:
	typedef property_color_base							inherited;
	typedef editor::color								color;

public:
					property_color_reference	(
						color& value,
						array<System::Attribute^>^ attributes
					);
	virtual			~property_color_reference	();
					!property_color_reference	();
	virtual color	get_value_raw				() override;
	virtual void	set_value_raw				(color value) override;

private:
	value_holder<color>*	m_value;
}; // ref class property_color_reference

#endif // ifndef PROPERTY_COLOR_REFERENCE_HPP_INCLUDED