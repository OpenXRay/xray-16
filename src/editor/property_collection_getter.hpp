////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_getter.hpp
//	Created 	: 08.01.2008
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property collection getter class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLLECTION_GETTER_HPP_INCLUDED
#define PROPERTY_COLLECTION_GETTER_HPP_INCLUDED

#include "property_collection_base.hpp"

[System::ComponentModel::EditorAttribute(property_collection_editor::typeid,System::Drawing::Design::UITypeEditor::typeid)]
[System::ComponentModel::TypeConverter(property_collection_converter::typeid)]
public ref class property_collection_getter : public property_collection_base {
public:
	typedef property_holder::collection_getter_type		collection_getter_type;

public:
							property_collection_getter	(collection_getter_type const& getter);
	virtual					~property_collection_getter	();
							!property_collection_getter	();

protected:
	virtual	collection_type*collection					() override;

private:
	collection_getter_type*	m_getter;
}; // ref class property_collection

#endif // ifndef PROPERTY_COLLECTION_GETTER_HPP_INCLUDED