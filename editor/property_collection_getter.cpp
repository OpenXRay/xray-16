////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_getter.cpp
//	Created 	: 08.01.2008
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property collection getter class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_collection_getter.hpp"

property_collection_getter::property_collection_getter		(collection_getter_type const& getter) :
	m_getter	(new collection_getter_type(getter))
{
}

property_collection_getter::~property_collection_getter		()
{
	this->!property_collection_getter	();
}

property_collection_getter::!property_collection_getter		()
{
	delete		(m_getter);
}

property_collection_getter::collection_type* property_collection_getter::collection	()
{
	return		((*m_getter)());
}