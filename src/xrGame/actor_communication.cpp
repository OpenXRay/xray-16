#include "pch_script.h"
#include "Actor.h"
#include "UIGameSP.h"
#include "PDA.h"
#include "Level.h"
#include "string_table.h"
#include "PhraseDialog.h"
#include "character_info.h"
#include "relation_registry.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_registry_container.h"
#include "script_game_object.h"
#include "game_cl_base.h"
#include "xrServer.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_registry_wrappers.h"
#include "map_manager.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIPdaWnd.h"
#include "ui/UITalkWnd.h"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "encyclopedia_article.h"
#include "GametaskManager.h"
#include "GameTaskDefs.h"
#include "InfoPortion.h"
#include "Inventory.h"
#include "CustomDetector.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/trader/ai_trader.h"

void CActor::AddGameNews(GAME_NEWS_DATA& news_data)
{
    GAME_NEWS_VECTOR& news_vector = game_news_registry->registry().objects();
    news_data.receive_time = Level().GetGameTime();
    news_vector.push_back(news_data);

    if (CurrentGameUI())
    {
        CurrentGameUI()->UIMainIngameWnd->ReceiveNews(&news_data);
    }
}

bool CActor::OnReceiveInfo(shared_str info_id) const
{
    if (!CInventoryOwner::OnReceiveInfo(info_id))
        return false;

    CInfoPortion info_portion;
    info_portion.Load(info_id);

    callback(GameObject::eInventoryInfo)(lua_game_object(), *info_id);

    if (!CurrentGameUI())
        return false;
    //только если находимся в режиме single
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (!pGameSP)
        return false;

    if (pGameSP->TalkMenu->IsShown())
    {
        pGameSP->TalkMenu->NeedUpdateQuestions();
    }

    return true;
}

void CActor::OnDisableInfo(shared_str info_id) const
{
    CInventoryOwner::OnDisableInfo(info_id);

    if (!CurrentGameUI())
        return;

    //только если находимся в режиме single
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (!pGameSP)
        return;

    if (pGameSP->TalkMenu->IsShown())
        pGameSP->TalkMenu->NeedUpdateQuestions();
}

void CActor::ReceivePhrase(DIALOG_SHARED_PTR& phrase_dialog)
{
    //только если находимся в режиме single
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (!pGameSP)
        return;

    if (pGameSP->TalkMenu->IsShown())
        pGameSP->TalkMenu->NeedUpdateQuestions();

    CPhraseDialogManager::ReceivePhrase(phrase_dialog);
}

void CActor::UpdateAvailableDialogs(CPhraseDialogManager* partner)
{
    m_AvailableDialogs.clear();
    m_CheckedDialogs.clear();

    //добавить актерский диалог собеседника
    CInventoryOwner* pInvOwnerPartner = smart_cast<CInventoryOwner*>(partner);
    VERIFY(pInvOwnerPartner);

    for (u32 i = 0; i < pInvOwnerPartner->CharacterInfo().ActorDialogs().size(); i++)
        AddAvailableDialog(pInvOwnerPartner->CharacterInfo().ActorDialogs()[i], partner);

    CPhraseDialogManager::UpdateAvailableDialogs(partner);
}

void CActor::TryToTalk()
{
    if (!IsTalking())
    {
        RunTalkDialog(m_pPersonWeLookingAt, false);
    }
}

void CActor::RunTalkDialog(CInventoryOwner* talk_partner, bool disable_break)
{
    //предложить поговорить с нами
    if (talk_partner->OfferTalk(this))
    {
        StartTalk(talk_partner);

        if (CurrentGameUI()->TopInputReceiver())
            CurrentGameUI()->TopInputReceiver()->HideDialog();

        //		smart_cast<CUIGameSP*>(CurrentGameUI())->StartTalk(disable_break);
        smart_cast<CUIGameSP*>(CurrentGameUI())->StartTalk(talk_partner->bDisableBreakDialog);
    }
}

void CActor::StartTalk(CInventoryOwner* talk_partner)
{
    PIItem det_active = inventory().ItemFromSlot(DETECTOR_SLOT);
    if (det_active)
    {
        CCustomDetector* det = smart_cast<CCustomDetector*>(det_active);
        det->HideDetector(true);
    }

    CGameObject* GO = smart_cast<CGameObject*>(talk_partner);
    VERIFY(GO);
    CInventoryOwner::StartTalk(talk_partner);
}

void CActor::NewPdaContact(CInventoryOwner* pInvOwner)
{
    if (!IsGameTypeSingle())
        return;

    bool b_alive = !!(smart_cast<CEntityAlive*>(pInvOwner))->g_Alive();
    CurrentGameUI()->UIMainIngameWnd->AnimateContacts(b_alive);

    Level().MapManager().AddRelationLocation(pInvOwner);
}

void CActor::LostPdaContact(CInventoryOwner* pInvOwner)
{
    CGameObject* GO = smart_cast<CGameObject*>(pInvOwner);
    if (GO)
    {
        for (int t = ALife::eRelationTypeFriend; t < ALife::eRelationTypeLast; ++t)
        {
            ALife::ERelationType tt = (ALife::ERelationType)t;
            Level().MapManager().RemoveMapLocation(RELATION_REGISTRY().GetSpotName(tt), GO->ID());
        }
        Level().MapManager().RemoveMapLocation("deadbody_location", GO->ID());
    };
}

void CActor::AddGameNews_deffered(GAME_NEWS_DATA& news_data, u32 delay)
{
    GAME_NEWS_DATA* d = new GAME_NEWS_DATA(news_data);
    //*d = news_data;
    m_defferedMessages.push_back(SDefNewsMsg());
    m_defferedMessages.back().news_data = d;
    m_defferedMessages.back().time = Device.dwTimeGlobal + delay;
    std::sort(m_defferedMessages.begin(), m_defferedMessages.end());
}

void CActor::UpdateDefferedMessages()
{
    while (m_defferedMessages.size())
    {
        SDefNewsMsg& M = m_defferedMessages.back();
        if (M.time <= Device.dwTimeGlobal)
        {
            AddGameNews(*M.news_data);
            xr_delete(M.news_data);
            m_defferedMessages.pop_back();
        }
        else
            break;
    }
}

bool CActor::OnDialogSoundHandlerStart(CInventoryOwner* inv_owner, LPCSTR phrase)
{
    CAI_Trader* trader = smart_cast<CAI_Trader*>(inv_owner);
    if (!trader)
        return false;

    trader->dialog_sound_start(phrase);
    return true;
}

bool CActor::OnDialogSoundHandlerStop(CInventoryOwner* inv_owner)
{
    CAI_Trader* trader = smart_cast<CAI_Trader*>(inv_owner);
    if (!trader)
        return false;

    trader->dialog_sound_stop();
    return true;
}

#ifdef DEBUG
void CActor::DumpTasks() { Level().GameTaskManager().DumpTasks(); }
#endif // DEBUG
