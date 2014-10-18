////////////////////////////////////////////////////////////////////////////
//	Module 		: property_float_reference.hpp
//	Created 	: 17.12.2007
//  Modified 	: 17.12.2007
//	Author		: Dmitriy Iassenev
//	Description : float property reference implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_FLOAT_REFERENCE_HPP_INCLUDED
#define PROPERTY_FLOAT_REFERENCE_HPP_INCLUDED

#include "property_holder_include.hpp"

public ref class property_float_reference :
	public XRay::SdkControls::IProperty,
	public XRay::SdkControls::IIncrementable
{
public:
							property_float_reference	(float& value, float const% increment_factor);
	virtual					~property_float_reference	();
							!property_float_reference	();
	virtual System::Object	^GetValue					();
	virtual void			SetValue					(System::Object ^object);
	virtual void			Increment					(float value);

private:
	value_holder<float>*	m_value;
	float					m_increment_factor;
}; // ref class property_float_reference

#endif // ifndef PROPERTY_FLOAT_REFERENCE_HPP_INCLUDED