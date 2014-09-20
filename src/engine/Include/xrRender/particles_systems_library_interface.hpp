////////////////////////////////////////////////////////////////////////////
//	Module 		: particles_systems_library_interface.hpp
//	Created 	: 26.01.2008
//  Modified 	: 26.01.2008
//	Author		: Dmitriy Iassenev
//	Description : particles systems library interface
////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLES_SYSTEMS_LIBRARY_INTERFACE_HPP_INCLUDED
#define PARTICLES_SYSTEMS_LIBRARY_INTERFACE_HPP_INCLUDED

namespace PS {
	class CPGDef;
} // namespace PS

namespace particles_systems {

class library_interface {
public:
	virtual	PS::CPGDef const* const*	particles_group_begin	() const = 0;
	virtual	PS::CPGDef const* const*	particles_group_end		() const = 0;
	virtual	void						particles_group_next	(PS::CPGDef const* const*& iterator) const = 0;
	virtual	shared_str const&			particles_group_id		(PS::CPGDef const& particles_group) const = 0;
}; // class library_interface

} // namespace particles_systems

#endif // #define PARTICLES_SYSTEMS_LIBRARY_INTERFACE_HPP_INCLUDED