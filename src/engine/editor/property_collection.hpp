////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection.hpp
//	Created 	: 24.12.2007
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property collection implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLLECTION_HPP_INCLUDED
#define PROPERTY_COLLECTION_HPP_INCLUDED

#include "property_collection_base.hpp"

[System::ComponentModel::EditorAttribute(property_collection_editor::typeid,System::Drawing::Design::UITypeEditor::typeid)]
[System::ComponentModel::TypeConverter(property_collection_converter::typeid)]
public ref class property_collection : public property_collection_base {
public:
							property_collection	(collection_type* collection);
	virtual					~property_collection();
							!property_collection();

protected:
	virtual	collection_type*collection			() override;

private:
	collection_type*		m_collection;
}; // ref class property_collection

#endif // ifndef PROPERTY_COLLECTION_HPP_INCLUDED