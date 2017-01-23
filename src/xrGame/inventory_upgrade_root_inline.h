////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_root_inline.h
//	Created 	: 01.11.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade root class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef INVENTORY_UPGRADE_ROOT_INLINE_H_INCLUDED
#define INVENTORY_UPGRADE_ROOT_INLINE_H_INCLUDED

namespace inventory
{
namespace upgrade
{

IC LPCSTR Root::scheme() const
{
	return	m_upgrade_scheme.c_str();
}

} // namespace upgrade
} // namespace inventory

#endif // INVENTORY_UPGRADE_ROOT_INLINE_H_INCLUDED
