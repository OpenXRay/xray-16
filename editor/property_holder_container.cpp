////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder_container.cpp
//	Created 	: 06.12.2007
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property holder implementation class (container properties)
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"
#include "property_property_container.hpp"

using Flobbster::Windows::Forms::PropertySpec;

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		editor::property_holder* value,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	property_holder*			real_value = dynamic_cast<property_holder*>(value);
	VERIFY						(real_value);
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			property_container::typeid,
			to_string(category),
			to_string(description),
			real_value->container()
		),
		gcnew property_property_container(
			real_value
		)
	);

	return						(nullptr);
}