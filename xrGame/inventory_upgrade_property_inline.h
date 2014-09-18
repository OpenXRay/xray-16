////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_property_inline.h
//	Created 	: 22.11.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade property class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef INVENTORY_UPGRADE_PROPERTY_INLINE_H_INCLUDED
#define INVENTORY_UPGRADE_PROPERTY_INLINE_H_INCLUDED

namespace inventory
{
namespace upgrade
{

IC const shared_str& Property::id() const
{
	return	m_id;
}

IC LPCSTR Property::id_str() const
{
	return	m_id.c_str();
}

IC LPCSTR Property::icon_name() const
{
	return	m_icon.c_str();
}

IC LPCSTR Property::name() const
{
	return	m_name.c_str();
}

IC Property::FunctorParams_type const& Property::functor_params() const
{
	return m_functor_params;
}

} // namespace upgrade
} // namespace inventory

#endif // INVENTORY_UPGRADE_PROPERTY_INLINE_H_INCLUDED
