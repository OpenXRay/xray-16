#include "StdAfx.h"
#include "game_cl_capture_the_artefact.h"

#include "Level.h"
#include "Actor.h"
#include "Inventory.h"
#include "xrServer_Objects_ALife_Items.h"
#include "Weapon.h"
#include "xr_level_controller.h"
#include "eatable_item_object.h"
#include "Missile.h"

void game_cl_CaptureTheArtefact::OnBuyMenu_Ok()
{
#ifdef DEBUG
    Msg("--- CTA: Buy menu OK...");
#endif // #ifdef DEBUG
    typedef CUIGameCTA::BuyMenuItemsCollection TBuyCol;

    VERIFY2(m_game_ui, "game ui not initialized");
    CUIGameCTA::BuyMenuItemsCollection toBuyItemsCollection;
    s32 moneyDif = 0;
    m_game_ui->GetPurchaseItems(toBuyItemsCollection, moneyDif);
    R_ASSERT(local_player);
    if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        if (InWarmUp())
        {
            buy_amount = 0;
        }
        else
        {
            buy_amount = moneyDif;
        }
        UpdateMoneyIndicator();
    }

    CGameObject* pPlayer = smart_cast<CGameObject*>(Level().CurrentEntity());
    VERIFY(pPlayer);

    NET_Packet P;
    pPlayer->u_EventGen(P, GE_GAME_EVENT, pPlayer->ID());
    P.w_u16(GAME_EVENT_PLAYER_BUY_FINISHED);
    if (InWarmUp())
    {
        P.w_s32(0);
    }
    else
    {
        P.w_s32(moneyDif);
    }
    P.w_u16(static_cast<u16>(toBuyItemsCollection.size()));

    TBuyCol::const_iterator bie = toBuyItemsCollection.end();

    for (TBuyCol::const_iterator toBuyIter = toBuyItemsCollection.begin(); toBuyIter != bie; ++toBuyIter)
    {
        P.w_u8(toBuyIter->first);
        P.w_u8(toBuyIter->second);
    }

    pPlayer->u_EventSend(P);

    if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        u_EventGen(P, GE_GAME_EVENT, local_player->GameID);
        P.w_u16(GAME_EVENT_PLAYER_BUYMENU_CLOSE);
        u_EventSend(P);
    }
    set_buy_menu_not_ready();
}

void game_cl_CaptureTheArtefact::OnBuyMenu_Cancel()
{
    if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        NET_Packet P;
        u_EventGen(P, GE_GAME_EVENT, local_player->GameID);
        P.w_u16(GAME_EVENT_PLAYER_BUYMENU_CLOSE);
        u_EventSend(P);
    }
}

void game_cl_CaptureTheArtefact::OnBuyMenuOpen()
{
    if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        NET_Packet P;
        u_EventGen(P, GE_GAME_EVENT, local_player->GameID);
        P.w_u16(GAME_EVENT_PLAYER_BUYMENU_OPEN);
        u_EventSend(P);
    }
}

bool game_cl_CaptureTheArtefact::LocalPlayerCanBuyItem(shared_str const& name_sect)
{
    if (name_sect == "mp_wpn_knife")
        return true;
    R_ASSERT(m_game_ui);
    return m_game_ui->CanBuyItem(name_sect);
}
