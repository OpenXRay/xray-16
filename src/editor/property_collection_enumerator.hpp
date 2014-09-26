////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_enumerator.hpp
//	Created 	: 24.12.2007
//  Modified 	: 24.12.2007
//	Author		: Dmitriy Iassenev
//	Description : collection property implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLLECTION_ENUMERATOR_HPP_INCLUDED
#define PROPERTY_COLLECTION_ENUMERATOR_HPP_INCLUDED

#include "property_holder_include.hpp"

public ref class property_collection_enumerator : public System::Collections::IEnumerator {
public:
	typedef editor::property_holder_collection	collection_type;
	typedef System::Collections::IEnumerator	IEnumerator;
	typedef System::Array						Array;
	typedef System::Object						Object;

public:
							property_collection_enumerator	(collection_type* collection);
	virtual	void			Reset							();
	virtual	bool			MoveNext						();
	property Object^		Current
	{
		virtual Object^		get								();
	}

private:
	collection_type*		m_collection;
	int						m_cursor;
}; // ref class property_collection_enumerator

#endif // ifndef PROPERTY_COLLECTION_ENUMERATOR_HPP_INCLUDED