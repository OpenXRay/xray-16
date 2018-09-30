#include "pch_script.h"
#include "trade.h"
#include "Actor.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/trader/ai_trader.h"
#include "Artefact.h"
#include "Inventory.h"
#include "xrMessages.h"
#include "character_info.h"
#include "relation_registry.h"
#include "Level.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"

class CInventoryOwner;

//////////////////////////////////////////////////////////////////////////////////////////
// CTrade class //////////////////////////////////////////////////////////////////////////
CTrade::CTrade(CInventoryOwner* p_io)
{
    TradeState = false;
    m_dwLastTradeTime = 0;
    pPartner.Set(TT_NONE, 0, 0);

    m_bNeedToUpdateArtefactTasks = false;

    // Заполнить pThis
    CAI_Trader* pTrader;
    CActor* pActor;
    CAI_Stalker* pStalker;

    // Определяем потомка этого экземпляра класса
    pTrader = smart_cast<CAI_Trader*>(p_io);
    if (pTrader)
        pThis.Set(TT_TRADER, pTrader, p_io);
    else
    {
        pActor = smart_cast<CActor*>(p_io);
        if (pActor)
            pThis.Set(TT_ACTOR, pActor, p_io);
        else
        {
            pStalker = smart_cast<CAI_Stalker*>(p_io);
            if (pStalker)
                pThis.Set(TT_STALKER, pStalker, p_io);
        }
    }
}

CTrade::~CTrade() {}
void CTrade::RemovePartner() { pPartner.Set(TT_NONE, 0, 0); }
//// предложение торговли
// void CTrade::Communicate()
//{
//	// Вывести приветствие
////	Msg("--TRADE::----------------------------------------------");
////	Msg("--TRADE::          TRADE ACIVATED                      ");
////	Msg("--TRADE::----------------------------------------------");
////	Msg("--TRADE:: - Hello, my name is [%s]", *pThis.base->cName());
////	Msg("--TRADE::   Wanna trade with me?" );
//
//	if (pPartner.inv_owner->GetTrade()->OfferTrade(pThis)) {
//		StartTrade();
//	}
//
//}
//
bool CTrade::SetPartner(CEntity* p)
{
    CAI_Trader* pTrader;
    CActor* pActor;
    CAI_Stalker* pStalker;

    pTrader = smart_cast<CAI_Trader*>(p);
    if (pTrader && (pTrader != pThis.base))
        pPartner.Set(TT_TRADER, pTrader, pTrader);
    else
    {
        pActor = smart_cast<CActor*>(p);
        if (pActor && (pActor != pThis.base))
            pPartner.Set(TT_ACTOR, pActor, pActor);
        else
        {
            pStalker = smart_cast<CAI_Stalker*>(p);
            if (pStalker && (pStalker != pThis.base))
                pPartner.Set(TT_STALKER, pStalker, pStalker);
            else
                return false;
        }
    }
    return true;
}

//// Man предлагает торговать
//// возвращает true, если данный trader готов торговать с man
//// т.е. принятие торговли
// bool CTrade::OfferTrade(SInventoryOwner man)
//{
//	StartTrade();
//	pPartner.Set(man.type,man.base,man.inv_owner);
//
//	string64	s;
//	switch (pPartner.type)
//	{
//		case TT_TRADER: xr_strcpy(s, "trader"); break;
//		case TT_STALKER:
//		case TT_ACTOR: xr_strcpy(s, "stalker"); break;
//	}
//
//
//	switch (pPartner.inv_owner->m_tRank)
//	{
//		case ALife::eStalkerRankNone: xr_strcpy(s,"NO_RANK"); break;
//		case ALife::eStalkerRankNovice: xr_strcpy(s,"NOVICE"); break;
//		case ALife::eStalkerRankExperienced: xr_strcpy(s,"EXPERIENCED"); break;
//		case ALife::eStalkerRankVeteran: xr_strcpy(s,"VETERAN"); break;
//		case ALife::eStalkerRankMaster: xr_strcpy(s,"MASTER"); break;
//		case ALife::eStalkerRankDummy: xr_strcpy(s,"DUMMY"); break;
//	}
//
//	return true;
//}
//

void CTrade::StartTrade()
{
    TradeState = true;
    m_dwLastTradeTime = Level().timeServer();
    m_bNeedToUpdateArtefactTasks = false;

    //	if (pThis.type == TT_TRADER) smart_cast<CAI_Trader*>(pThis.base)->OnStartTrade();
}

void CTrade::StartTradeEx(CInventoryOwner* pInvOwner)
{
    SetPartner(smart_cast<CEntity*>(pInvOwner));
    StartTrade();
}

void CTrade::TradeCB(bool bStart)
{
    if (bStart)
    {
        if (pThis.type == TT_TRADER)
            smart_cast<CAI_Trader*>(pThis.base)->OnStartTrade();
    }
    else if (pThis.type == TT_TRADER)
        smart_cast<CAI_Trader*>(pThis.base)->OnStopTrade();
}

void CTrade::OnPerformTrade(u32 money_get, u32 money_put)
{
    if (pThis.type == TT_TRADER)
        smart_cast<CAI_Trader*>(pThis.base)->callback(GameObject::eTradePerformTradeOperation)(money_get, money_put);
}

void CTrade::StopTrade()
{
    TradeState = false;
    m_dwLastTradeTime = 0;
    //	Msg("--TRADE:: [%s]: Trade stopped...",*pThis.base->cName());

    CAI_Trader* pTrader = NULL;
    if (pThis.type == TT_TRADER)
    {
        // pTrader = smart_cast<CAI_Trader*>(pThis.base);
        // pTrader->OnStopTrade();
    }
    else if (pPartner.type == TT_TRADER)
    {
        pTrader = smart_cast<CAI_Trader*>(pPartner.base);
    }

    RemovePartner();
}

void CTrade::UpdateTrade() {}
