////////////////////////////////////////////////////////////////////////////
//	Module 		: property_color_base.hpp
//	Created 	: 27.12.2007
//  Modified 	: 27.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property color base class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLOR_BASE_HPP_INCLUDED
#define PROPERTY_COLOR_BASE_HPP_INCLUDED

#include "property_holder_include.hpp"
#include "property_container_holder.hpp"

ref class property_container;
ref class property_color_base;

class color_components {
public:
									color_components(property_color_base^ holder);

			float	xr_stdcall		red_getter		();
			void	xr_stdcall		red_setter		(float);

			float	xr_stdcall		green_getter	();
			void	xr_stdcall		green_setter	(float);

			float	xr_stdcall		blue_getter		();
			void	xr_stdcall		blue_setter		(float);

private:
	gcroot<property_color_base^>	m_holder;
}; // class color_components

public value struct Color {
public:
	inline					Color			(float red, float green, float blue)
	{
		r			= red;
		g			= green;
		b			= blue;
	}

	property float	r;
	property float	g;
	property float	b;
}; // value struct Color

public ref class property_color_base abstract :
	public property_value,
	public property_container_holder,
	public editor::controls::property_mouse_events,
	public editor::controls::property_incrementable
{
public:
	typedef System::Attribute					Attribute;

public:
							property_color_base	(editor::color const% color, array<System::Attribute^>^ attributes);
	virtual					~property_color_base();
							!property_color_base();
			void			red					(float value);
			void			green				(float value);
			void			blue				(float value);

public:
	virtual System::Object	^get_value			();
	virtual void			set_value			(System::Object ^object);
	virtual	void			on_double_click		(editor::controls::property_grid^ property_grid);
	virtual void			increment			(float const% increment);

public:
	virtual editor::color	get_value_raw		() = 0;
	virtual void			set_value_raw		(editor::color color) = 0;

private:
	property_container^							m_container;
	color_components*							m_components;
	array<System::Attribute^>^					m_attributes;
}; // ref class property_color_base abstract

#endif // ifndef PROPERTY_COLOR_BASE_HPP_INCLUDED