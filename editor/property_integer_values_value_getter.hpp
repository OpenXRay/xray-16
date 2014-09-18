////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_values_value_getter.hpp
//	Created 	: 09.01.2008
//  Modified 	: 09.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property integer values value getter class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_INTEGER_VALUES_VALUE_GETTER_HPP_INCLUDED
#define PROPERTY_INTEGER_VALUES_VALUE_GETTER_HPP_INCLUDED

#include "property_integer.hpp"
#include "property_integer_values_value_base.hpp"

public ref class property_integer_values_value_getter :
	public property_integer,
	public property_integer_values_value_base
{
public:
	typedef editor::property_holder									property_holder;
	typedef property_holder::integer_getter_type					integer_getter_type;
	typedef property_holder::integer_setter_type					integer_setter_type;
	typedef property_holder::string_collection_getter_type			string_collection_getter_type;
	typedef property_holder::string_collection_size_getter_type		string_collection_size_getter_type;

private:
	typedef property_integer										inherited;
	typedef System::Object											Object;
	typedef System::Collections::IList								IList;

public:
							property_integer_values_value_getter	(
								integer_getter_type const& getter,
								integer_setter_type const& setter,
								string_collection_getter_type const& collection_getter,
								string_collection_size_getter_type const& collection_size_getter
							);
	virtual					~property_integer_values_value_getter	();
							!property_integer_values_value_getter	();
	virtual Object			^get_value								() override;
	virtual void			set_value								(Object ^object) override;
	virtual	IList^			collection								();

public:
	string_collection_getter_type*		m_collection_getter;
	string_collection_size_getter_type*	m_collection_size_getter;
}; // ref class property_integer_values_value_getter

#endif // ifndef PROPERTY_INTEGER_VALUES_VALUE_GETTER_HPP_INCLUDED