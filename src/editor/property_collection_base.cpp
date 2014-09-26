////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_base.cpp
//	Created 	: 24.12.2007
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property collection implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_collection_base.hpp"
#include "property_collection_enumerator.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"

using System::Object;
using System::Collections::IEnumerator;

property_collection_base::property_collection_base		()
{
}

property_collection_base::~property_collection_base		()
{
	this->!property_collection_base	();
}

property_collection_base::!property_collection_base		()
{
}

Object ^property_collection_base::get_value				()
{
	return						(this);
}

void property_collection_base::set_value				(Object ^object)
{
}

void property_collection_base::CopyTo					(Array^ items, int index)
{
	collection_type*			collection = this->collection();
	for (int i=index, n=collection->size(); i<n ; ++i) {
		editor::property_holder*holder_raw = collection->item(i);
		::property_holder*		holder = dynamic_cast<::property_holder*>(holder_raw);
		VERIFY					(holder);
		items->SetValue			(holder->container(), i);
	}
}

IEnumerator^ property_collection_base::GetEnumerator	()
{
	return						(gcnew property_collection_enumerator(collection()));
}

bool property_collection_base::IsSynchronized::get		()
{
	return						(false);
}
	
Object^ property_collection_base::SyncRoot::get			()
{
	return						(this);
}
	
int property_collection_base::Count::get				()
{
	return						(collection()->size());
}

int property_collection_base::Add						(Object^ value)
{
	collection_type*			collection = this->collection();
	u32							collection_size = collection->size();
	property_container^			container = safe_cast<property_container^>(value);
	collection->insert			(&container->holder(), collection_size);
	return						(collection_size - 1);
}

void property_collection_base::Clear					()
{
	collection()->clear			();
}

bool property_collection_base::Contains					(Object^ value)
{
	return						(IndexOf(value) > -1);
}

int property_collection_base::IndexOf					(Object^ value)
{
	property_container^			container = safe_cast<property_container^>(value);
	return						(collection()->index(&container->holder()));
}

void property_collection_base::Insert					(int index, Object^ value)
{
	property_container^			container = safe_cast<property_container^>(value);
	collection()->insert		(&container->holder(), index);
}

void property_collection_base::Remove					(Object^ value)
{
	RemoveAt					(IndexOf(value));
}

void property_collection_base::RemoveAt					(int index)
{
	collection()->erase			(index);
}

bool property_collection_base::IsFixedSize::get			()
{
	return						(false);
}

bool property_collection_base::IsReadOnly::get			()
{
	return						(false);
}

Object^ property_collection_base::default::get			(int index)
{
	editor::property_holder*	holder_raw = collection()->item(index);
	::property_holder*			holder = dynamic_cast<::property_holder*>(holder_raw);
	return						(holder->container());
}

void property_collection_base::default::set				(int index, Object^ value)
{
	RemoveAt					(index);
	Insert						(index, value);
}

property_container^	property_collection_base::create	()
{
	editor::property_holder*	holder_raw = collection()->create();
	::property_holder*			holder = dynamic_cast<::property_holder*>(holder_raw);
	return						(holder->container());
}