////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_editor.cpp
//	Created 	: 24.12.2007
//  Modified 	: 25.12.2007
//	Author		: Dmitriy Iassenev
//	Description : collection property editor implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_collection_editor.hpp"
#include "property_container.hpp"
#include "property_holder.hpp"
#include "property_collection.hpp"
#include "ide_impl.hpp"
#include "window_ide.h"
#include "window_view.h"

using System::Type;
using System::String;
using System::Object;
using System::ComponentModel::PropertyDescriptor;
using editor::property_holder_collection;
using Flobbster::Windows::Forms::PropertyBag;
using System::ComponentModel::Design::CollectionEditor;

typedef PropertyBag::PropertySpecDescriptor	PropertySpecDescriptor;

#pragma unmanaged
extern ide_impl* g_ide;
#pragma managed

property_collection_editor::property_collection_editor				(Type^ type) :
	inherited					(type)
{
}

Type^ property_collection_editor::CreateCollectionItemType			()
{
	return						(property_container::typeid);
}

Object^ property_collection_editor::CreateInstance					(Type^ type)
{
	property_container^			container = safe_cast<property_container^>(Context->Instance);
	PropertySpecDescriptor^		descriptor = safe_cast<PropertySpecDescriptor^>(Context->PropertyDescriptor);
	property_value^				raw_value = container->value(descriptor->item);
	property_collection^		collection = safe_cast<property_collection^>(raw_value);
	return						(collection->create());
}

String^ property_collection_editor::GetDisplayText					(Object^ value)
{
	property_container^			container = safe_cast<property_container^>(value);

	property_holder_collection*	collection = container->holder().collection();
	if (!collection)
		return					(container->holder().display_name());

	int							index = collection->index(&container->holder());
	if (index < 0)
		return					(container->holder().display_name());

	VERIFY						((index < (int)collection->size()));
	char						buffer[256];
	collection->display_name	((u32)index, buffer, sizeof(buffer));

	return						(to_string(buffer));
}

void property_collection_editor::on_move							(Object^ sender, EventArgs^ e)
{
	g_ide->window()->view().Invalidate	();
}

property_collection_editor::CollectionForm^ property_collection_editor::CreateCollectionForm	()
{
//	VERIFY						(!m_collection_form);
	m_collection_form			= inherited::CreateCollectionForm();
	m_collection_form->Move		+= gcnew System::EventHandler(this, &property_collection_editor::on_move);
	return						(m_collection_form);
}

Object^	property_collection_editor::EditValue						(
		ITypeDescriptorContext^ context,
		IServiceProvider^ provider,
		Object^ value
	)
{
	if (!m_collection_form || !m_collection_form->Visible)
		return					(inherited::EditValue(context, provider, value));

	property_collection_editor^	editor = gcnew property_collection_editor(CollectionType);
	return						(editor->EditValue(context, provider, value));
}
