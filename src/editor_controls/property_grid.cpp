////////////////////////////////////////////////////////////////////////////
//	Module 		: property_grid.cpp
//	Created 	: 23.01.2008
//  Modified 	: 23.01.2008
//	Author		: Dmitriy Iassenev
//	Description : engine interface class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_grid.hpp"
#include "property_container_interface.hpp"
#include "property_incrementable.hpp"

ref class property_container;

using editor::controls::property_grid;

using System::Windows::Forms::GridItem;
using System::Windows::Forms::MouseEventHandler;
using System::ComponentModel::PropertyDescriptor;
using System::Type;
using System::Reflection::FieldInfo;
using System::Reflection::BindingFlags;
using Microsoft::Win32::RegistryKey;
using System::String;

using Flobbster::Windows::Forms::PropertyBag;
typedef PropertyBag::PropertySpecDescriptor			PropertySpecDescriptor;

template <typename T>
inline static T registry_value						(RegistryKey ^key, String ^value_id, T const %default_value)
{
	System::Object^					object = key->GetValue(value_id);
	if (!object)
		return						(default_value);

	return							((T)object);
}

property_grid::property_grid						()
{
	initialize_grid_view			();
	setup_event_handlers			();
}

void property_grid::initialize_grid_view			()
{
	for each (Control^ i in Controls)
		if (i->GetType()->Name->ToUpper() == "PROPERTYGRIDVIEW") {
			m_property_grid_view	= i;
			break;
		}
}

void property_grid::OnChildControlMouseDoubleClick	(Object^ object, System::Windows::Forms::MouseEventArgs^ e)
{
	if (!SelectedObject)
		return;
	
	if (!SelectedGridItem)
		return;

	PropertyDescriptor^				descriptor_raw = SelectedGridItem->PropertyDescriptor;
	VERIFY							(descriptor_raw);
	PropertySpecDescriptor^			descriptor = safe_cast<PropertySpecDescriptor^>(descriptor_raw);
	property_container_interface^	container = safe_cast<property_container_interface^>(descriptor->bag);
	property_value^					value = container->value(descriptor->item);
	property_mouse_events^			value_mouse_events = dynamic_cast<property_mouse_events^>(value);
	if (!value_mouse_events)
		return;

	value_mouse_events->on_double_click	(this);
}

void property_grid::setup_event_handlers			()
{
	for each (Control^ i in Controls) {
		i->MouseDoubleClick			+= gcnew MouseEventHandler(this, &property_grid::OnChildControlMouseDoubleClick);
		i->MouseMove				+= gcnew MouseEventHandler(this, &property_grid::OnChildControlMouseMove);
		i->MouseDown				+= gcnew MouseEventHandler(this, &property_grid::OnChildControlMouseDown);
	}
}

int property_grid::splitter_width					()
{
	Type^							grid_type = m_property_grid_view->GetType();
	FieldInfo^						field = grid_type->GetField("labelWidth", BindingFlags::NonPublic | BindingFlags::Instance);
	Object^							value = field->GetValue(m_property_grid_view);
	return							(*safe_cast<System::Int32^>(value));
}

void property_grid::save							(RegistryKey^ root, String^ key)
{
	RegistryKey						^grid = root->CreateSubKey(key);
	grid->SetValue					("Splitter", (int)splitter_width());
	grid->Close						();
}

void property_grid::load							(RegistryKey^ root, String^ key)
{
	RegistryKey						^grid = root->OpenSubKey(key);
	if (!grid)
		return;

	int								position = registry_value(grid, "Splitter", splitter_width());
	grid->Close						();

	Type^							grid_type = m_property_grid_view->GetType();
	FieldInfo^						field = grid_type->GetField("labelWidth", BindingFlags::NonPublic | BindingFlags::Instance);
	field->SetValue					(m_property_grid_view, position);
}

void property_grid::OnChildControlMouseDown			(Object^ object, System::Windows::Forms::MouseEventArgs^ e)
{
	if (e->Button != System::Windows::Forms::MouseButtons::Middle)
		return;

	m_previous_location				= e->Location;
}

void property_grid::OnChildControlMouseMove			(Object^ object, System::Windows::Forms::MouseEventArgs^ e)
{
	if (e->Button != System::Windows::Forms::MouseButtons::Middle) {
		m_previous_location			= e->Location;
		return;
	}

	if (e->Location.X == m_previous_location.X)
		return;

	if (!SelectedObject)
		return;
	
	if (!SelectedGridItem)
		return;

	PropertyDescriptor^			descriptor_raw = SelectedGridItem->PropertyDescriptor;
	if (!descriptor_raw)
		return;

	PropertySpecDescriptor^		descriptor = safe_cast<PropertySpecDescriptor^>(descriptor_raw);
	property_container_interface^	container = safe_cast<property_container_interface^>(descriptor->bag);
	property_value^				raw_value = container->value(descriptor->item);
	VERIFY						(raw_value);

	property_incrementable^		incrementable = dynamic_cast<property_incrementable^>(raw_value);
	if (!incrementable)
		return;

	incrementable->increment	(float(e->Location.X - m_previous_location.X));
	Refresh						();
	m_previous_location			= e->Location;
}