////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection.cpp
//	Created 	: 24.12.2007
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property collection class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_collection.hpp"
#include "property_collection_enumerator.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"

using System::Object;
using System::Collections::IEnumerator;

property_collection::property_collection		(collection_type* collection) :
	m_collection				(collection)
{
}

property_collection::~property_collection		()
{
	this->!property_collection	();
}

property_collection::!property_collection		()
{
}

property_collection::collection_type* property_collection::collection	()
{
	return						(m_collection);
}