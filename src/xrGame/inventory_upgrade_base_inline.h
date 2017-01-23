////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_base_inline.h
//	Created 	: 23.10.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade base class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef INVENTORY_UPGRADE_BASE_INLINE_H_INCLUDED
#define INVENTORY_UPGRADE_BASE_INLINE_H_INCLUDED

namespace inventory
{
namespace upgrade
{

IC const shared_str& UpgradeBase::id() const
{
	return	( m_id );
}

IC LPCSTR UpgradeBase::id_str() const
{
	return	( m_id.c_str() );
}

IC	bool UpgradeBase::is_known() const
{
	return	( m_known ); 
}

} // namespace upgrade
} // namespace inventory

#endif // INVENTORY_UPGRADE_BASE_INLINE_H_INCLUDED