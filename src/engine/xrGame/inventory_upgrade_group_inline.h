////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_group_inline.h
//	Created 	: 22.10.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade group class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef INVENTORY_UPGRADE_GROUP_INLINE_H_INCLUDED
#define INVENTORY_UPGRADE_GROUP_INLINE_H_INCLUDED

namespace inventory
{
namespace upgrade
{

IC const shared_str& Group::id() const
{
	return	( m_id );
}

IC LPCSTR Group::id_str() const
{
	return	( m_id.c_str() );
}

} // namespace upgrade
} // namespace inventory

#endif // INVENTORY_UPGRADE_GROUP_INLINE_H_INCLUDED