////////////////////////////////////////////////////////////////////////////
//	Module 		: property_vec3f_reference.hpp
//	Created 	: 29.12.2007
//  Modified 	: 29.12.2007
//	Author		: Dmitriy Iassenev
//	Description : vec3f property reference implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_VEC3F_REFERENCE_HPP_INCLUDED
#define PROPERTY_VEC3F_REFERENCE_HPP_INCLUDED

#include "property_vec3f_base.hpp"

public ref class property_vec3f_reference : public property_vec3f_base {
public:
	typedef property_vec3f_base							inherited;
	typedef editor::vec3f								vec3f;

public:
					property_vec3f_reference	(vec3f& value);
	virtual			~property_vec3f_reference	();
					!property_vec3f_reference	();
	virtual vec3f	get_value_raw				() override;
	virtual void	set_value_raw				(vec3f value) override;

private:
	value_holder<vec3f>*	m_value;
}; // ref class property_vec3f_reference

#endif // ifndef PROPERTY_VEC3F_REFERENCE_HPP_INCLUDED