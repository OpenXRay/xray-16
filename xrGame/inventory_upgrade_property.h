////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_property.h
//	Created 	: 22.11.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade property class
////////////////////////////////////////////////////////////////////////////

#ifndef INVENTORY_UPGRADE_PROPERTY_H_INCLUDED
#define INVENTORY_UPGRADE_PROPERTY_H_INCLUDED

#include "inventory_upgrade.h"

namespace inventory
{
namespace upgrade
{


class Property : private boost::noncopyable
{
public:
	typedef xr_vector<shared_str>		FunctorParams_type;

private:
	typedef detail::functor2<LPCSTR>		StrFunctor;

public:
					Property();
	virtual			~Property();

			void	construct( const shared_str& property_id, Manager& manager_r );
	IC shared_str const& id() const;
	IC LPCSTR		id_str() const;
	IC LPCSTR		icon_name() const;
	IC LPCSTR		name() const;

	IC FunctorParams_type const&	functor_params() const;

		bool		run_functor( LPCSTR parameter, string256& result );

public:
	
protected:
	shared_str		m_id;

	shared_str		m_name;
	shared_str		m_icon;

	StrFunctor			m_desc;
	FunctorParams_type	m_functor_params;

}; // class Property


} // namespace upgrade
} // namespace inventory

#include "inventory_upgrade_property_inline.h"

#endif // INVENTORY_UPGRADE_PROPERTY_H_INCLUDED

