////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_enumerator.cpp
//	Created 	: 24.12.2007
//  Modified 	: 24.12.2007
//	Author		: Dmitriy Iassenev
//	Description : collection property implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_collection_enumerator.hpp"
#include "property_holder.hpp"

using System::Object;
using System::InvalidOperationException;

property_collection_enumerator::property_collection_enumerator	(collection_type* collection) :
	m_collection				(collection),
	m_cursor					(-1)
{
}

void property_collection_enumerator::Reset						()
{
	m_cursor					= -1;
}

bool property_collection_enumerator::MoveNext					()
{
	if (m_cursor < (int)m_collection->size())
		++m_cursor;

	return						(m_cursor != (int)m_collection->size());
}

Object^ property_collection_enumerator::Current::get			()
{
	if (m_cursor < 0)
		throw					(gcnew InvalidOperationException());

	if (m_cursor >= (int)m_collection->size())
		throw					(gcnew InvalidOperationException());

	editor::property_holder*	holder_raw = m_collection->item((u32)m_cursor);
	property_holder*			holder = dynamic_cast<property_holder*>(holder_raw);
	VERIFY						(holder);
	return						(holder->container());
}