////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_enum_value_reference.hpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property integer enum value reference class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_INTEGER_ENUM_VALUE_REFERENCE_HPP_INCLUDED
#define PROPERTY_INTEGER_ENUM_VALUE_REFERENCE_HPP_INCLUDED

#include "property_integer_reference.hpp"

public ref class property_integer_enum_value_reference : public property_integer_reference {
public:
	typedef Pair<int, System::String^>						ValuePair;

private:
	typedef property_integer_reference						inherited;
	typedef System::Collections::ArrayList					collection_type;
	typedef System::Object									Object;
	typedef std::pair<int, LPCSTR>							pair;

public:
					property_integer_enum_value_reference	(
						int& value,
						pair* values,
						u32 const& value_count
					);
	virtual Object	^get_value								() override;
	virtual void	set_value								(Object ^object) override;

public:
	collection_type^m_collection;
}; // ref class property_integer_enum_value_reference

#endif // ifndef PROPERTY_INTEGER_ENUM_VALUE_REFERENCE_HPP_INCLUDED