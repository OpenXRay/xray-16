////////////////////////////////////////////////////////////////////////////
//	Module 		: property_float_limited.hpp
//	Created 	: 12.12.2007
//  Modified 	: 12.12.2007
//	Author		: Dmitriy Iassenev
//	Description : limited float property implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_FLOAT_LIMITED_HPP_INCLUDED
#define PROPERTY_FLOAT_LIMITED_HPP_INCLUDED

#include "property_float.hpp"

public ref class property_float_limited : public property_float {
private:
	typedef property_float	inherited;

public:
					property_float_limited	(
						float_getter_type const &getter,
						float_setter_type const &setter,
						float const% increment_factor,
						float const %min,
						float const %max
					);
	virtual Object	^get_value				() override;
	virtual void	set_value				(System::Object ^object) override;

private:
	float			m_min;
	float			m_max;
}; // ref class property_float_limited

#endif // ifndef PROPERTY_FLOAT_LIMITED_HPP_INCLUDED