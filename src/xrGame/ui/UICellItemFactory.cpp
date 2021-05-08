#include "StdAfx.h"
#include "UICellItemFactory.h"
#include "UICellCustomItems.h"

CUICellItem* create_cell_item(CInventoryItem* itm)
{
    VERIFY(itm);
    CUICellItem* cell_item;

    CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(itm);
    CWeapon* pWeapon = smart_cast<CWeapon*>(itm);
    if (pAmmo)
    {
        cell_item = xr_new<CUIAmmoCellItem>(pAmmo);
    }
    else if (pWeapon)
    {
        cell_item = xr_new<CUIWeaponCellItem>(pWeapon);
    }
    else
    {
        cell_item = xr_new<CUIInventoryCellItem>(itm);
    }
    return cell_item;
}
