////////////////////////////////////////////////////////////////////////////
//	Module 		: property_container.cpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property container class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_container.hpp"
#include "property_holder.hpp"

using Flobbster::Windows::Forms::PropertySpecEventHandler;
using System::Collections::Hashtable;
using System::Collections::IDictionary;
using System::String;
using System::Collections::ArrayList;

#pragma unmanaged
class ide_impl;
extern ide_impl* g_ide;
#pragma managed

property_container::property_container			(
		property_holder* holder,
		property_container_holder^ container_holder
	) :
	m_holder					(holder),
	m_container_holder			(container_holder),
	m_properties				(gcnew Hashtable()),
	m_ordered_properties		(gcnew ArrayList())
{
	GetValue					+= gcnew PropertySpecEventHandler(this, &property_container::get_value_handler);
	SetValue					+= gcnew PropertySpecEventHandler(this, &property_container::set_value_handler);
}

property_container::~property_container			()
{
	this->!property_container	();
}

property_container::!property_container			()
{
	if (!m_holder)
		return;

	if (!g_ide)
		return;

	property_holder*			holder = dynamic_cast<property_holder*>(m_holder);
	VERIFY						(holder);

	holder->on_dispose			();
}

property_holder &property_container::holder		()
{
	VERIFY						(m_holder);
	return						(*m_holder);
}

property_container_holder% property_container::container_holder	()
{
	VERIFY						(m_container_holder);
	return						(*m_container_holder);
}

bool property_container::equal_category			(String^ new_category, String^ old_category)
{
	VERIFY						(!new_category->Length || (new_category[0] != '\t'));
	if (!old_category->Length || (old_category[0] != '\t'))
		return					(new_category == old_category);

	for (u32 i=0, n=old_category->Length; i<n; ++i) {
		if (old_category[i] == '\t')
			continue;

		return					(new_category == old_category->Substring(i, n - i));
	}

	NODEFAULT;
#ifdef DEBUG
	return						(false);
#endif // #ifdef DEBUG
}

String^ property_container::update_categories	(String^ new_category)
{
	for each (PropertySpec^ i in m_ordered_properties) {
		String^					category = i->Category;
		if (!equal_category(new_category, category))
			continue;

		return					(category);
	}

	for each (PropertySpec^ i in m_ordered_properties)
		i->Category				= "\t" + i->Category;

	return						(new_category);
}

void property_container::try_update_name		(PropertySpec^ description, String^ name)
{
	VERIFY						(!!name->Length);
	VERIFY						((name[0] != '\t'));
	
	String^						description_name = description->Name;
	VERIFY						(!!description_name->Length);
	if (description_name[0] != '\t') {
		if (name != description_name)
			return;
		
		description->Name		= "\t" + description_name;
		return;
	}

	for (u32 i=0, n=description_name->Length; i<n; ++i) {
		if (description_name[i] == '\t')
			continue;

		if (name != description_name->Substring(i, n - i))
			return;
		
		description->Name		= "\t" + description->Name;
		return;
	}

	NODEFAULT;
}

void property_container::update_names			(String^ name)
{
	bool						found = false;
	for each (PropertySpec^ i in m_ordered_properties) {
		if (i->Name != name)
			continue;

		found					= true;
		break;
	}

	if (!found)
		return;

	for each (PropertySpec^ i in m_ordered_properties)
		try_update_name			(i, name);
}

void property_container::add_property			(PropertySpec^ description, property_value^ value)
{
	VERIFY						(!m_properties[description]);

#if 0
	u32							n = description->Attributes ? description->Attributes->Length : 0;
	array<System::Attribute^>^	attributes = gcnew array<System::Attribute^>(n + 1);
	for (u32 i = 0; i < n; ++i)
		attributes[i + 0]		= description->Attributes[i];

	attributes[n]				= gcnew System::ComponentModel::DisplayNameAttribute(description->Name);
	description->Attributes		= attributes;
#endif

	description->Category		= update_categories(description->Category);
	update_names				(description->Name);
	m_properties[description]	= value;
	Properties->Add				(description);
	m_ordered_properties->Add	(description);
}

property_value^ property_container::value		(PropertySpec^ description)
{
	VERIFY						(m_properties[description]);
	return						(safe_cast<property_value^>(m_properties[description]));
}

IDictionary% property_container::properties		()
{
	VERIFY						(m_properties);
	return						(*m_properties);
}

ArrayList% property_container::ordered_properties	()
{
	VERIFY						(m_ordered_properties);
	return						(*m_ordered_properties);
}

void property_container::get_value_handler		(Object^ sender, PropertySpecEventArgs^ e)
{
	property_value				^value = safe_cast<property_value^>(m_properties[e->Property]);
	VERIFY						(value);
	e->Value					= value->get_value();
}

void property_container::set_value_handler		(Object^ sender, PropertySpecEventArgs^ e)
{
	property_value				^value = safe_cast<property_value^>(m_properties[e->Property]);
	VERIFY						(value);
	value->set_value			(e->Value);
}

void property_container::clear					()
{
	m_properties->Clear			();
	m_categories->Clear			();
	m_ordered_properties->Clear	();
	Properties->Clear			();
}