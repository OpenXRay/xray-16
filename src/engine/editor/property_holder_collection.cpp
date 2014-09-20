////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder_collection.cpp
//	Created 	: 06.12.2007
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property holder implementation class (collection properties)
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"
#include "property_collection.hpp"
#include "property_collection_getter.hpp"

using Flobbster::Windows::Forms::PropertySpec;
using editor::property_holder_collection;

editor::property_value* property_holder::add_property				(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		property_holder_collection* collection,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			property_collection::typeid,
			to_string(category),
			to_string(description)
		),
		gcnew property_collection(
			collection
		)
	);

	return						(nullptr);
}

editor::property_value* property_holder::add_property		(
		LPCSTR identifier,
		LPCSTR category,
		LPCSTR description,
		collection_getter_type const& collection_getter,
		readonly_enum const& read_only,
		notify_parent_on_change_enum const& notify_parent,
		password_char_enum const& password,
		refresh_grid_on_change_enum const& refresh_grid
	)
{
	m_container->add_property	(
		gcnew PropertySpec(
			to_string(identifier),
			property_collection::typeid,
			to_string(category),
			to_string(description)
		),
		gcnew property_collection_getter(
			collection_getter
		)
	);

	return						(nullptr);
}