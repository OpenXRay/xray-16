////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_limited_reference.hpp
//	Created 	: 12.12.2007
//  Modified 	: 12.12.2007
//	Author		: Dmitriy Iassenev
//	Description : limited integer property reference implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_INTEGER_LIMITED_REFERENCE_HPP_INCLUDED
#define PROPERTY_INTEGER_LIMITED_REFERENCE_HPP_INCLUDED

#include "property_integer_reference.hpp"

public ref class property_integer_limited_reference : public property_integer_reference {
private:
	typedef property_integer_reference					inherited;

public:
	typedef System::Object		Object;

public:
					property_integer_limited_reference	(
								int& value,
								int const %min,
								int const %max
							);
	virtual Object	^get_value							() override;
	virtual void	set_value							(System::Object ^object) override;

private:
	int				m_min;
	int				m_max;
}; // ref class property_integer_limited_reference

#endif // ifndef PROPERTY_INTEGER_LIMITED_REFERENCE_HPP_INCLUDED