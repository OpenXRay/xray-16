////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_impl.h
//	Created 	: 18.08.2005
//  Modified 	: 18.08.2005
//	Author		: Dmitriy Iassenev
//	Description : inventory item implementation functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Inventory.h"

IC CInventoryOwner& CInventoryItem::inventory_owner() const
{
    VERIFY(m_pInventory);
    VERIFY(m_pInventory->GetOwner());
    return (*m_pInventory->GetOwner());
}
