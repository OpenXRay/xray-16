////////////////////////////////////////////////////////////////////////////
//	Module 		: property_vec3f.hpp
//	Created 	: 29.12.2007
//  Modified 	: 29.12.2007
//	Author		: Dmitriy Iassenev
//	Description : vec3f property implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_VEC3F_HPP_INCLUDED
#define PROPERTY_VEC3F_HPP_INCLUDED

#include "property_vec3f_base.hpp"

public ref class property_vec3f : public property_vec3f_base {
public:
	typedef editor::property_holder::vec3f_getter_type	vec3f_getter_type;
	typedef editor::property_holder::vec3f_setter_type	vec3f_setter_type;
	typedef property_vec3f_base							inherited;

public:
							property_vec3f	(
								vec3f_getter_type const &getter,
								vec3f_setter_type const &setter
							);
	virtual					~property_vec3f	();
							!property_vec3f	();
	virtual editor::vec3f	get_value_raw	() override;
	virtual void			set_value_raw	(editor::vec3f value) override;

private:
	vec3f_getter_type		*m_getter;
	vec3f_setter_type		*m_setter;
}; // ref class property_vec3f

#endif // ifndef PROPERTY_VEC3F_HPP_INCLUDED