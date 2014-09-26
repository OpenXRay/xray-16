////////////////////////////////////////////////////////////////////////////
//	Module 		: property_color_base.cpp
//	Created 	: 27.12.2007
//  Modified 	: 27.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property color base class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_color_base.hpp"
#include "property_container.hpp"
#include "property_float_limited.hpp"

using System::Object;
using System::String;
using Flobbster::Windows::Forms::PropertySpec;
using System::Collections::DictionaryEntry;

ref class property_converter_float;

static float s_increment_factor	= .001f;

#if 0
	using System::Windows::Forms::ColorDialog;
#endif // #if 0

color_components::color_components			(property_color_base^ holder) :
	m_holder			(holder)
{
}

float color_components::red_getter			()
{
	return				(m_holder->get_value_raw().r);
}

void color_components::red_setter			(float value)
{
	m_holder->red		(value);
}

float color_components::green_getter		()
{
	return				(m_holder->get_value_raw().g);
}

void color_components::green_setter			(float value)
{
	m_holder->green		(value);
}

float color_components::blue_getter			()
{
	return				(m_holder->get_value_raw().b);
}

void color_components::blue_setter			(float value)
{
	m_holder->blue		(value);
}

property_color_base::property_color_base	(editor::color const% color, array<System::Attribute^>^ attributes)
{
	m_container			= gcnew property_container(nullptr, this);
	m_components		= new color_components(this);

	u32					n = attributes ? attributes->Length : 0;
	m_attributes		= gcnew array<System::Attribute^>(n + 1);
	for (u32 i = 0; i < n; ++i)
		m_attributes[i]	= attributes[i];

	m_attributes[n]		= gcnew System::ComponentModel::NotifyParentPropertyAttribute(true);

	typedef editor::property_holder::float_getter_type	float_getter_type;
	typedef editor::property_holder::float_setter_type	float_setter_type;

	float_getter_type	getter;
	float_setter_type	setter;

	getter.bind			(m_components, &color_components::red_getter);
	setter.bind			(m_components, &color_components::red_setter);

	PropertySpec		^spec;
	spec				=
		gcnew PropertySpec(
			"red",
			float::typeid,
			"components",
			"red component",
			color.r,
			(String^)nullptr,
			property_converter_float::typeid
		);
	spec->Attributes	=
		gcnew array<System::Attribute^>{
				gcnew System::ComponentModel::NotifyParentPropertyAttribute(true)
		};
	m_container->add_property	(
		spec,
		gcnew property_float_limited(
			getter,
			setter,
			.0025f,
			0.f,
			1.f
		)
	);

	getter.bind			(m_components, &color_components::green_getter);
	setter.bind			(m_components, &color_components::green_setter);
	spec				=
		gcnew PropertySpec(
			"green",
			float::typeid,
			"components",
			"green component",
			color.g,
			(String^)nullptr,
			property_converter_float::typeid
		);

	spec->Attributes	=
		gcnew array<System::Attribute^>{
				gcnew System::ComponentModel::NotifyParentPropertyAttribute(true)
		};
	m_container->add_property	(
		spec,
		gcnew property_float_limited(
			getter,
			setter,
			.0025f,
			0.f,
			1.f
		)
	);

	getter.bind			(m_components, &color_components::blue_getter);
	setter.bind			(m_components, &color_components::blue_setter);
	spec				=
		gcnew PropertySpec(
			"blue",
			float::typeid,
			"components",
			"blue component",
			color.b,
			(String^)nullptr,
			property_converter_float::typeid
		);

	spec->Attributes	=
		gcnew array<System::Attribute^>{
				gcnew System::ComponentModel::NotifyParentPropertyAttribute(true)
		};
	m_container->add_property	(
		spec,
		gcnew property_float_limited(
			getter,
			setter,
			.0025f,
			0.f,
			1.f
		)
	);
}

property_color_base::~property_color_base	()
{
	this->!property_color_base();
}

property_color_base::!property_color_base	()
{
	delete				(m_container);
}

Object^ property_color_base::get_value		()
{
	return				(m_container);
}

void property_color_base::set_value			(Object ^object)
{
	Color				color = safe_cast<Color>(object);
	editor::color		value;
	value.r				= color.r;
	value.g				= color.g;
	value.b				= color.b;
	set_value_raw		(value);
}

void property_color_base::red				(float value)
{
	editor::color		current = get_value_raw();
	current.r			= value;
	set_value_raw		(current);
}

void property_color_base::green				(float value)
{
	editor::color		current = get_value_raw();
	current.g			= value;
	set_value_raw		(current);
}

void property_color_base::blue				(float value)
{
	editor::color		current = get_value_raw();
	current.b			= value;
	set_value_raw		(current);
}

void property_color_base::on_double_click	(editor::controls::property_grid^ property_grid)
{
#if 0
	ColorDialog					^dialog = gcnew ColorDialog();
	dialog->FullOpen			= true;
	editor::color				color = get_value_raw();
	dialog->Color				= System::Drawing::Color::FromArgb(255, int(255.f*color.r), int(255.f*color.g), int(255.f*color.b));
	if (dialog->ShowDialog() == System::Windows::Forms::DialogResult::Cancel)
		return;

	set_value					(::Color(dialog->Color.R/255.f, dialog->Color.G/255.f, dialog->Color.B/255.f));
	property_grid->Refresh		();
#endif // #if 0
}

static void increment_and_clamp				(float& value, float const& increment)
{
	value						+= increment;
	if (value > 1.f) {
		value					= 1.f;
		return;
	}

	if (value < 0.f)
		value					= 0.f;
}

void property_color_base::increment			(float const% increment)
{
	float						increment_value = increment*s_increment_factor;
	editor::color				value = get_value_raw();
	increment_and_clamp			(value.r, increment_value);
	increment_and_clamp			(value.g, increment_value);
	increment_and_clamp			(value.b, increment_value);
	set_value_raw				(value);
}