#include "StdAfx.h"

#include "MPPlayersBag.h"
#include "Level.h"

#include "game_base_space.h"

#define BAG_REMOVE_TIME 60000

CMPPlayersBag::CMPPlayersBag(){};

CMPPlayersBag::~CMPPlayersBag(){};

void CMPPlayersBag::OnEvent(NET_Packet& P, u16 type)
{
    CInventoryItemObject::OnEvent(P, type);
    u16 id;
    switch (type)
    {
    case GE_OWNERSHIP_TAKE:
    {
        P.r_u16(id);
        IGameObject* O = Level().Objects.net_Find(id);
        CInventoryItem* pIItem = smart_cast<CInventoryItem*>(O);
        R_ASSERT(pIItem->m_pInventory == NULL);

#ifdef MP_LOGGING
        Msg("--- Rukzak [%d] takes [%d][%s]", ID(), O->ID(), O->cNameSect().c_str());
#endif // MP_LOGGING

        O->H_SetParent(this);
        O->Position().set(Position());
    }
    break;
    case GE_OWNERSHIP_REJECT:
    {
        P.r_u16(id);
        IGameObject* O = Level().Objects.net_Find(id);

#ifdef MP_LOGGING
        Msg("--- Rukzak [%d] rejects [%d][%s]", ID(), O->ID(), O->cNameSect().c_str());
#endif // MP_LOGGING

        O->H_SetParent(0, !P.r_eof() && P.r_u8());
    }
    break;
    }
}

extern INT g_iWeaponRemove;
bool CMPPlayersBag::NeedToDestroyObject() const
{
    if (GameID() == eGameIDSingle)
        return false;
    if (Remote())
        return false;
    if (H_Parent())
        return false;
    if (g_iWeaponRemove == -1)
        return false;
    if (g_iWeaponRemove == 0)
        return true;
    if (TimePassedAfterIndependant() > BAG_REMOVE_TIME)
        return true;

    return false;
}
