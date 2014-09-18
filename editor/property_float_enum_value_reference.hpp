////////////////////////////////////////////////////////////////////////////
//	Module 		: property_float_enum_value_reference.hpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property float enum value reference class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_FLOAT_ENUM_VALUE_REFERENCE_HPP_INCLUDED
#define PROPERTY_FLOAT_ENUM_VALUE_REFERENCE_HPP_INCLUDED

#include "property_float_reference.hpp"

public ref class property_float_enum_value_reference : public property_float_reference {
public:
	typedef editor::property_holder::float_getter_type	float_getter_type;
	typedef editor::property_holder::float_setter_type	float_setter_type;
	typedef Pair<float, System::String^>				ValuePair;

private:
	typedef property_float_reference					inherited;
	typedef System::Collections::ArrayList				collection_type;
	typedef System::Object								Object;
	typedef std::pair<float, LPCSTR>					pair;

public:
					property_float_enum_value_reference	(
						float& value,
						pair *values,
						u32 const &value_count
					);
	virtual Object	^get_value							() override;
	virtual void	set_value							(Object ^object) override;
	virtual void	increment							(float const% increment) override;

public:
	collection_type^m_collection;
}; // ref class property_float_enum_value_reference

#endif // ifndef PROPERTY_FLOAT_ENUM_VALUE_REFERENCE_HPP_INCLUDED