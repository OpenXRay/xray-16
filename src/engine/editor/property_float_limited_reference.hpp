////////////////////////////////////////////////////////////////////////////
//	Module 		: property_float_limited_reference.hpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : limited float property reference implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_FLOAT_LIMITED_REFERENCE_HPP_INCLUDED
#define PROPERTY_FLOAT_LIMITED_REFERENCE_HPP_INCLUDED

#include "property_float_reference.hpp"

public ref class property_float_limited_reference : public property_float_reference {
private:
	typedef property_float_reference		inherited;

public:
					property_float_limited_reference	(
						float& value,
						float const% increment_factor,
						float const %min,
						float const %max
					);
	virtual Object	^get_value				() override;
	virtual void	set_value				(System::Object ^object) override;

private:
	float			m_min;
	float			m_max;
}; // ref class property_float_limited_reference

#endif // ifndef PROPERTY_FLOAT_LIMITED_REFERENCE_HPP_INCLUDED