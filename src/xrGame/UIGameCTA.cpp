#include "stdafx.h"
#include "UIGameCTA.h"

#include "UITeamPanels.h"

#include "game_cl_base.h"
#include "game_cl_capture_the_artefact.h"
#include "game_cl_mp.h"

#include "Level.h"
#include "actor.h"
#include "artefact.h"
#include "inventory.h"
#include "xrServer_Objects_ALife_Items.h"
#include "weapon.h"
#include "WeaponMagazinedWGrenade.h"
#include "WeaponKnife.h"
#include "xr_level_controller.h"

#include "Common/object_broker.h"

#include "weaponknife.h"

#include "ui/UISkinSelector.h"
//.#include "ui/UIInventoryWnd.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIMapDesc.h"
#include "ui/UISpawnWnd.h"
#include "ui/UIBuyWndBase.h"
#include "ui/UIMpTradeWnd.h"
#include "ui/UIBuyWndShared.h"
#include "ui/UIMoneyIndicator.h"
#include "ui/UIRankIndicator.h"
#include "ui/UIProgressShape.h"
#include "ui/UIMessageBoxEx.h"
#include "ui/UIVoteStatusWnd.h"
#include "ui/UIActorMenu.h"
#include "ui/UISkinSelector.h"
#include "ui/UIHelper.h"

#define CTA_GAME_WND_XML "ui_game_cta.xml"

#define TEAM_PANELS_XML_NAME "ui_team_panels_cta.xml"

CUIGameCTA::CUIGameCTA()
    : teamPanels(NULL), m_pFragLimitIndicator(NULL), m_team1_score(NULL), m_team2_score(NULL), m_pCurBuyMenu(NULL),
      m_pCurSkinMenu(NULL), m_pBuySpawnMsgBox(NULL), m_game(NULL), m_voteStatusWnd(NULL), m_team_panels_shown(false)
{
    m_pUITeamSelectWnd = new CUISpawnWnd();
}

void CUIGameCTA::Init(int stage)
{
    if (stage == 0)
    {
        m_round_result_caption = UIHelper::CreateTextWnd(*MsgConfig, "mp_round_result", Window);
        m_pressbuy_caption = UIHelper::CreateTextWnd(*MsgConfig, "mp_pressbuy", Window);
        m_pressjump_caption = UIHelper::CreateTextWnd(*MsgConfig, "mp_pressjump", Window);
        m_spectator_caption = UIHelper::CreateTextWnd(*MsgConfig, "mp_spectator", Window);
        m_spectrmode_caption = UIHelper::CreateTextWnd(*MsgConfig, "mp_spetatormode", Window);
        m_warm_up_caption = UIHelper::CreateTextWnd(*MsgConfig, "mp_warm_up", Window);
        m_time_caption = UIHelper::CreateTextWnd(*MsgConfig, "mp_timelimit", Window);
        m_demo_play_caption = UIHelper::CreateTextWnd(*MsgConfig, "mp_demo_play", Window);

        teamPanels = new UITeamPanels();
        teamPanels->Init(TEAM_PANELS_XML_NAME, "team_panels_wnd");

        CUIXml uiXml;
        uiXml.Load(CONFIG_PATH, UI_PATH, CTA_GAME_WND_XML);

        CUIXmlInit::InitWindow(uiXml, "global", 0, Window);

        m_pMoneyIndicator = new CUIMoneyIndicator();
        m_pMoneyIndicator->SetAutoDelete(true);
        m_pMoneyIndicator->InitFromXML(uiXml);

        m_pRankIndicator = new CUIRankIndicator();
        m_pRankIndicator->SetAutoDelete(true);
        m_pRankIndicator->InitFromXml(uiXml);

        m_pReinforcementInidcator = new CUITextWnd();
        m_pReinforcementInidcator->SetAutoDelete(true);
        CUIXmlInit::InitTextWnd(uiXml, "reinforcement", 0, m_pReinforcementInidcator);

        m_team1_icon = new CUIStatic();
        m_team2_icon = new CUIStatic();
        CUIXmlInit::InitStatic(uiXml, "team1_icon", 0, m_team1_icon);
        CUIXmlInit::InitStatic(uiXml, "team2_icon", 0, m_team2_icon);

        m_team1_score = new CUITextWnd();
        m_team2_score = new CUITextWnd();
        m_team1_score->SetAutoDelete(true);
        m_team2_score->SetAutoDelete(true);
        CUIXmlInit::InitTextWnd(uiXml, "team1_score", 0, m_team1_score);
        CUIXmlInit::InitTextWnd(uiXml, "team2_score", 0, m_team2_score);

        m_pFragLimitIndicator = new CUITextWnd();
        m_pFragLimitIndicator->SetAutoDelete(true);
        CUIXmlInit::InitTextWnd(uiXml, "fraglimit", 0, m_pFragLimitIndicator);
    }

    if (stage == 2)
    {
        inherited::Init(stage);
        Window->AttachChild(m_pMoneyIndicator);
        Window->AttachChild(m_pRankIndicator);
        Window->AttachChild(m_pReinforcementInidcator);
        Window->AttachChild(m_pFragLimitIndicator);
        Window->AttachChild(m_team1_score);
        Window->AttachChild(m_team2_score);
    }
}

void CUIGameCTA::UnLoad()
{
    inherited::UnLoad();
    xr_delete(teamPanels);
    xr_delete(m_team1_icon);
    xr_delete(m_team2_icon);
}

CUIGameCTA::~CUIGameCTA()
{
    delete_data(m_pUITeamSelectWnd);
    delete_data(m_pBuySpawnMsgBox);
    xr_delete(m_voteStatusWnd);
    xr_delete(m_pCurBuyMenu);
    xr_delete(m_pCurSkinMenu);
}

bool CUIGameCTA::IsTeamPanelsShown()
{
    VERIFY(teamPanels);
    return m_team_panels_shown; // teamPanels->IsShown();
}
void CUIGameCTA::ShowTeamPanels(bool bShow)
{
    if (bShow)
    {
        AddDialogToRender(teamPanels);
    }
    else
    {
        RemoveDialogToRender(teamPanels);
    }
    m_team_panels_shown = bShow;
}

void CUIGameCTA::UpdateTeamPanels()
{
    teamPanels->NeedUpdatePanels();
    teamPanels->NeedUpdatePlayers();
}

void CUIGameCTA::SetClGame(game_cl_GameState* g)
{
    inherited::SetClGame(g);
    m_game = smart_cast<game_cl_CaptureTheArtefact*>(g);
    VERIFY(m_game);

    /*if (m_pMapDesc)
    {
        if (m_pMapDesc->IsShown())
        {
            m_pMapDesc->HideDialog();
        }
        delete_data(m_pMapDesc);
    }
    m_pMapDesc = new CUIMapDesc();*/

    if (m_pBuySpawnMsgBox)
    {
        if (m_pBuySpawnMsgBox->IsShown())
        {
            m_pBuySpawnMsgBox->HideDialog();
        }
        delete_data(m_pBuySpawnMsgBox);
    }

    m_pBuySpawnMsgBox = new CUIMessageBoxEx();
    m_pBuySpawnMsgBox->InitMessageBox("message_box_buy_spawn");
    m_pBuySpawnMsgBox->SetText("");

    m_game->SetGameUI(this);
    m_pBuySpawnMsgBox->func_on_ok = CUIWndCallback::void_function(m_game, &game_cl_CaptureTheArtefact::OnBuySpawn);
}

void CUIGameCTA::AddPlayer(ClientID const& clientId) { teamPanels->AddPlayer(clientId); }
void CUIGameCTA::RemovePlayer(ClientID const& clientId) { teamPanels->RemovePlayer(clientId); }
void CUIGameCTA::UpdatePlayer(ClientID const& clientId) { teamPanels->UpdatePlayer(clientId); }
bool CUIGameCTA::IsTeamSelectShown()
{
    VERIFY(m_pUITeamSelectWnd);
    return m_pUITeamSelectWnd->IsShown();
}
void CUIGameCTA::ShowTeamSelectMenu()
{
    if (Level().IsDemoPlay())
        return;
    VERIFY(m_pUITeamSelectWnd);
    if (!m_pUITeamSelectWnd->IsShown())
    {
        m_pUITeamSelectWnd->ShowDialog(true);
    }
}

void CUIGameCTA::UpdateBuyMenu(shared_str const& teamSection, shared_str const& costSection)
{
    if (m_pCurBuyMenu)
    {
        if (m_teamSectionForBuyMenu == teamSection)
        {
            if (m_pCurBuyMenu->IsShown())
                HideBuyMenu();
            m_pCurBuyMenu->IgnoreMoneyAndRank(false);
            m_pCurBuyMenu->SetRank(m_game->local_player->rank);
            m_pCurBuyMenu->ClearPreset(_preset_idx_last);
            return;
        }
        xr_delete(m_pCurBuyMenu);
    }
    m_teamSectionForBuyMenu = teamSection;
    /// warning !!!
    m_pCurBuyMenu = new BUY_WND_TYPE();
    m_pCurBuyMenu->Init(m_teamSectionForBuyMenu, costSection);
    m_costSection = costSection;
}

bool CUIGameCTA::CanBuyItem(shared_str const& sect_name)
{
    CUIMpTradeWnd* buy_menu = smart_cast<CUIMpTradeWnd*>(m_pCurBuyMenu);
    R_ASSERT(buy_menu);
    return buy_menu->HasItemInGroup(sect_name);
}

void CUIGameCTA::UpdateSkinMenu(shared_str const& teamSection)
{
    game_PlayerState* tempPlayerState = Game().local_player;
    VERIFY2(tempPlayerState, "local_player not initialized");

    if (m_pCurSkinMenu)
    {
        if (m_teamSectionForSkinMenu == teamSection)
        {
            return;
        }
        xr_delete(m_pCurSkinMenu);
        m_pCurSkinMenu = NULL;
    }
    m_teamSectionForSkinMenu = teamSection;
    m_pCurSkinMenu = new CUISkinSelectorWnd(m_teamSectionForSkinMenu.c_str(), static_cast<s16>(tempPlayerState->team));
}

void CUIGameCTA::HideBuyMenu()
{
    R_ASSERT2(m_pCurBuyMenu, "buy menu not initialized");
    if (m_pCurBuyMenu->IsShown())
    {
        m_pCurBuyMenu->HideDialog();
    }
}

void CUIGameCTA::ShowBuyMenu()
{
    if (Level().IsDemoPlay())
        return;
    R_ASSERT2(m_pCurBuyMenu, "buy menu not initialized");
    if (!m_pCurBuyMenu->IsShown())
    {
        m_pCurBuyMenu->IgnoreMoneyAndRank(m_game->InWarmUp());

        m_pCurBuyMenu->ResetItems();
        m_pCurBuyMenu->SetupPlayerItemsBegin();

        SetPlayerItemsToBuyMenu();
        SetPlayerParamsToBuyMenu();

        m_pCurBuyMenu->SetupPlayerItemsEnd();

        m_pCurBuyMenu->ShowDialog(true);
        m_game->OnBuyMenuOpen();
    }
}
/*
void CUIGameCTA::BuyMenuItemIDInserter(u16 const & itemID)
{

}*/

void TryToDefuseWeapon(CWeapon const* weapon, TIItemContainer const& all_items, buffer_vector<shared_str>& dest_ammo);

void CUIGameCTA::TryToDefuseAllWeapons(aditional_ammo_t& dest_ammo)
{
    game_PlayerState* ps = Game().local_player;
    VERIFY2(ps, "local player not initialized");
    CActor* actor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
    R_ASSERT2(actor || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD),
        make_string("bad actor: not found in game (GameID = %d)", ps->GameID).c_str());

    TIItemContainer const& all_items = actor->inventory().m_all;

    for (TIItemContainer::const_iterator i = all_items.begin(), ie = all_items.end(); i != ie; ++i)
    {
        CWeapon* tmp_weapon = smart_cast<CWeapon*>(*i);
        if (tmp_weapon)
            TryToDefuseWeapon(tmp_weapon, all_items, dest_ammo);
    }
}

struct AmmoSearcherPredicate
{
    u16 additional_ammo_count;
    shared_str ammo_section;

    AmmoSearcherPredicate(u16 ammo_elapsed, shared_str const& ammo_sect)
        : additional_ammo_count(ammo_elapsed), ammo_section(ammo_sect)
    {
    }

    bool operator()(PIItem const& item)
    {
        CWeaponAmmo* temp_ammo = smart_cast<CWeaponAmmo*>(item);
        if (!temp_ammo)
            return false;

        if (temp_ammo->m_boxCurr >= temp_ammo->m_boxSize)
            return false;

        if (temp_ammo->cNameSect() != ammo_section)
            return false;

        if ((temp_ammo->m_boxCurr + additional_ammo_count) < temp_ammo->m_boxSize)
            return false;

        return true;
    }
};

void TryToDefuseGrenadeLauncher(
    CWeaponMagazinedWGrenade const* weapon, TIItemContainer const& all_items, buffer_vector<shared_str>& dest_ammo)
{
    if (!weapon)
        return;

    xr_vector<shared_str> const* tmp_ammo_types = NULL;
    u8 const* tmp_ammo_type = NULL;
    u16 ammo_elapsed = 0;
    if (weapon->m_bGrenadeMode)
    {
        tmp_ammo_types = &weapon->m_ammoTypes;
        tmp_ammo_type = &weapon->m_ammoType;
        ammo_elapsed = (u16)weapon->GetAmmoElapsed();
    }
    else
    {
        tmp_ammo_types = &weapon->m_ammoTypes2;
        tmp_ammo_type = &weapon->m_ammoType2;
        ammo_elapsed = (u16)weapon->m_magazine2.size();
    }

    if (tmp_ammo_types->size() <= u32(*tmp_ammo_type))
        return;

    shared_str ammo_section = (*tmp_ammo_types)[*tmp_ammo_type];

    VERIFY2(ammo_section.size(),
        make_string("grenade ammo type of [%s] hasn't section name", weapon->cNameSect().c_str()).c_str());
    if (!ammo_section.size())
        return;

    VERIFY(pSettings->line_exist(ammo_section.c_str(), "box_size"));

    u16 ammo_box_size = pSettings->r_u16(ammo_section.c_str(), "box_size");

    R_ASSERT2(ammo_elapsed <= 1,
        make_string("weapon [%s] can't have more than one grenade in grenade launcher", weapon->cNameSect().c_str())
            .c_str());

    while (ammo_elapsed >= ammo_box_size)
    {
        dest_ammo.push_back(ammo_section);
        ammo_elapsed = ammo_elapsed - ammo_box_size;
    }
    if (!ammo_elapsed)
        return;

    AmmoSearcherPredicate ammo_completitor(ammo_elapsed, ammo_section);

    TIItemContainer::const_iterator temp_iter = std::find_if(all_items.begin(), all_items.end(), ammo_completitor);

    if (temp_iter == all_items.end())
        return;

    CWeaponAmmo* temp_ammo = smart_cast<CWeaponAmmo*>(*temp_iter);
    R_ASSERT2(temp_ammo, "failed to create ammo after defusing weapon");
    temp_ammo->m_boxCurr = temp_ammo->m_boxSize;
}

void TryToDefuseWeapon(CWeapon const* weapon, TIItemContainer const& all_items, buffer_vector<shared_str>& dest_ammo)
{
    if (!weapon)
        return;

    CWeaponMagazinedWGrenade const* tmp_gl_weapon = smart_cast<CWeaponMagazinedWGrenade const*>(weapon);
    if (weapon->IsGrenadeLauncherAttached())
        TryToDefuseGrenadeLauncher(tmp_gl_weapon, all_items, dest_ammo);

    xr_vector<shared_str> const* tmp_ammo_types = NULL;
    u8 const* tmp_ammo_type = NULL;
    u16 ammo_elapsed = 0;
    if (tmp_gl_weapon && tmp_gl_weapon->m_bGrenadeMode)
    {
        tmp_ammo_types = &tmp_gl_weapon->m_ammoTypes2;
        tmp_ammo_type = &tmp_gl_weapon->m_ammoType2;
        ammo_elapsed = (u16)tmp_gl_weapon->m_magazine2.size();
    }
    else
    {
        tmp_ammo_types = &weapon->m_ammoTypes;
        tmp_ammo_type = &weapon->m_ammoType;
        ammo_elapsed = (u16)weapon->GetAmmoElapsed();
    }

    if (tmp_ammo_types->size() <= u32(*tmp_ammo_type))
        return;

    shared_str ammo_section = (*tmp_ammo_types)[*tmp_ammo_type];

    VERIFY2(ammo_section.size(), make_string("ammo type of [%s] hasn't section name", weapon->cName().c_str()).c_str());
    if (!ammo_section.size())
        return;

    VERIFY(pSettings->line_exist(ammo_section.c_str(), "box_size"));

    u16 ammo_box_size = pSettings->r_u16(ammo_section.c_str(), "box_size");

    while (ammo_elapsed >= ammo_box_size)
    {
        dest_ammo.push_back(ammo_section);
        ammo_elapsed = ammo_elapsed - ammo_box_size;
    }
    if (!ammo_elapsed)
        return;

    AmmoSearcherPredicate ammo_completitor(ammo_elapsed, ammo_section);

    TIItemContainer::const_iterator temp_iter = std::find_if(all_items.begin(), all_items.end(), ammo_completitor);

    if (temp_iter == all_items.end())
        return;

    CWeaponAmmo* temp_ammo = smart_cast<CWeaponAmmo*>(*temp_iter);
    R_ASSERT2(temp_ammo, "failed to create ammo after defusing weapon");
    temp_ammo->m_boxCurr = temp_ammo->m_boxSize;
}

void CUIGameCTA::AdditionalAmmoInserter(aditional_ammo_t::value_type const& sect_name)
{
    VERIFY(m_pCurBuyMenu);

    if (!pSettings->line_exist(m_costSection, sect_name.c_str()))
        return;

    m_pCurBuyMenu->ItemToSlot(sect_name.c_str(), 0);
}

void CUIGameCTA::BuyMenuItemInserter(PIItem const& item)
{
    VERIFY(m_pCurBuyMenu);
    if (!item)
        return;

    if (item->IsInvalid() || smart_cast<CWeaponKnife*>(&item->object()))
        return;

    CArtefact* pArtefact = smart_cast<CArtefact*>(item);
    if (pArtefact)
        return;

    if (!pSettings->line_exist(m_costSection, item->object().cNameSect()))
        return;

    if (!item->CanTrade())
        return;

    u8 addons = 0;
    CWeapon* pWeapon = smart_cast<CWeapon*>(item);
    if (pWeapon)
        addons = pWeapon->GetAddonsState();

    CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(item);
    if (pAmmo && (pAmmo->m_boxCurr != pAmmo->m_boxSize))
        return;

    m_pCurBuyMenu->ItemToSlot(item->object().cNameSect(), addons);
}

void CUIGameCTA::BuyMenuItemInserter(CInventorySlot const& slot) { BuyMenuItemInserter(slot.m_pIItem); }
void CUIGameCTA::SetPlayerDefItemsToBuyMenu()
{
    if (m_pCurBuyMenu->IsShown())
        return;
    m_pCurBuyMenu->ResetItems();
    m_pCurBuyMenu->SetupDefaultItemsBegin();
    //---------------------------------------------------------
    u8 KnifeSlot, KnifeIndex;
    m_pCurBuyMenu->GetWeaponIndexByName("mp_wpn_knife", KnifeSlot, KnifeIndex);
    //---------------------------------------------------------
    PRESET_ITEMS TmpPresetItems;
    auto It = PlayerDefItems.begin();
    auto Et = PlayerDefItems.end();
    for (; It != Et; ++It)
    {
        PresetItem PIT = *It;
        if (PIT.ItemID == KnifeIndex)
            continue;
        m_pCurBuyMenu->ItemToSlot(m_pCurBuyMenu->GetWeaponNameByIndex(0, PIT.ItemID), PIT.SlotID);
    };
    //---------------------------------------------------------
    m_pCurBuyMenu->SetupDefaultItemsEnd();
}

void CUIGameCTA::SetPlayerItemsToBuyMenu()
{
    VERIFY(m_pCurBuyMenu);
    game_PlayerState* ps = Game().local_player;
    VERIFY2(ps, "local player not initialized");
    CActor* actor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
    R_ASSERT2(actor || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD),
        make_string("bad actor: not found in game (GameID = %d)", ps->GameID).c_str());

    if (actor && !ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        auto& inventory = actor->inventory();
        u32 max_addammo_count = actor->inventory().m_all.size();
        aditional_ammo_t add_ammo(
            _alloca(sizeof(aditional_ammo_t::value_type) * (max_addammo_count * 2)), max_addammo_count * 2);
        TryToDefuseAllWeapons(add_ammo);
        for (u16 i = inventory.FirstSlot(); i <= inventory.LastSlot(); i++)
            BuyMenuItemInserter(inventory.ItemFromSlot(i));
        for (auto& item : actor->inventory().m_belt)
            BuyMenuItemInserter(item);
        for (auto& item : actor->inventory().m_ruck)
            BuyMenuItemInserter(item);
        for (auto& ammo_item : add_ammo)
            AdditionalAmmoInserter(ammo_item);
    }
    else
    {
        SetPlayerDefItemsToBuyMenu();
    }
}

void CUIGameCTA::ReInitPlayerDefItems()
{
    R_ASSERT(m_pCurBuyMenu);
    LoadDefItemsForRank();
    SetPlayerDefItemsToBuyMenu();
}

void CUIGameCTA::SetPlayerParamsToBuyMenu()
{
    VERIFY(m_pCurBuyMenu);

    game_PlayerState* ps = Game().local_player;
    VERIFY2(ps, "local player not initialized");
    CActor* actor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
    R_ASSERT2(actor || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD),
        make_string("bad actor: not found in game (GameID = %d)", ps->GameID).c_str());

    m_pCurBuyMenu->SetRank(ps->rank);
    m_pCurBuyMenu->SetMoneyAmount(ps->money_for_round);
}

void CUIGameCTA::GetPurchaseItems(BuyMenuItemsCollection& dest, s32& moneyDif)
{
    R_ASSERT(m_game);
    R_ASSERT(m_pCurBuyMenu);
    preset_items const* tmpPresItems = &(m_pCurBuyMenu->GetPreset(_preset_idx_last));
    if (tmpPresItems->size() == 0)
    {
        tmpPresItems = &(m_pCurBuyMenu->GetPreset(_preset_idx_default)); //_preset_idx_origin));
    }
    preset_items::const_iterator pie = tmpPresItems->end();
    for (preset_items::const_iterator pi = tmpPresItems->begin(); pi != pie; ++pi)
    {
        u8 addon;
        u8 itemId;
        // we just use addon variable as temp storage
        m_pCurBuyMenu->GetWeaponIndexByName(pi->sect_name, addon, itemId);

        addon = pi->addon_state;

        for (u32 ic = 0; ic < pi->count; ++ic)
            dest.push_back(std::make_pair(addon, itemId));
    }

    if (m_game->local_player && m_game->local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        u8 KnifeSlot, KnifeIndex;
        m_pCurBuyMenu->GetWeaponIndexByName("mp_wpn_knife", KnifeSlot, KnifeIndex);
        dest.push_back(std::make_pair(KnifeSlot, KnifeIndex));
    }

    moneyDif = m_pCurBuyMenu->GetPresetCost(_preset_idx_origin) - m_pCurBuyMenu->GetPresetCost(_preset_idx_last);
}

CUIGameCTA::BuyMenuItemPair CUIGameCTA::GetBuyMenuItem(shared_str const& itemSectionName)
{
    VERIFY(m_pCurBuyMenu);
    u8 groupId;
    u8 itemId;
    // we just use addon variable as temp storage
    m_pCurBuyMenu->GetWeaponIndexByName(itemSectionName, groupId, itemId);
    return std::make_pair(groupId, itemId);
}

void CUIGameCTA::ShowSkinMenu(s8 currentSkin)
{
    if (Level().IsDemoPlay())
        return;
    // VERIFY2(m_pCurSkinMenu, "skin menu not initialized");
    if (!m_pCurSkinMenu)
    {
#ifdef CLIENT_CTA_LOG
        Msg("Warning: current skin window not initialized while trying to show it");
#endif
        return;
    }
    if (!m_pCurSkinMenu->IsShown())
    {
        m_pCurSkinMenu->ShowDialog(true);
    }
}

s8 CUIGameCTA::GetSelectedSkinIndex()
{
    VERIFY(m_pCurSkinMenu);
    return static_cast<s8>(m_pCurSkinMenu->GetActiveIndex());
}

void CUIGameCTA::SetReinforcementTimes(u32 curTime, u32 maxTime)
{
    string128 _buff;
    m_pReinforcementInidcator->SetText(xr_itoa(curTime / 1000, _buff, 10));
}

void CUIGameCTA::DisplayMoneyChange(LPCSTR deltaMoney) { m_pMoneyIndicator->SetMoneyChange(deltaMoney); }
void CUIGameCTA::DisplayMoneyBonus(KillMessageStruct* bonus) { m_pMoneyIndicator->AddBonusMoney(*bonus); }
void CUIGameCTA::ChangeTotalMoneyIndicator(LPCSTR newMoneyString) { m_pMoneyIndicator->SetMoneyAmount(newMoneyString); }
void CUIGameCTA::SetRank(ETeam team, u8 rank)
{
    m_pRankIndicator->SetRank(static_cast<u8>(team), rank);
    if (m_pCurBuyMenu)
    {
        m_pCurBuyMenu->SetRank(rank);
    }
};

void CUIGameCTA::SetScore(s32 max_score, s32 greenTeamScore, s32 blueTeamScore)
{
    string32 str;
    xr_sprintf(str, "%d", greenTeamScore);
    m_team1_score->SetText(str);
    xr_sprintf(str, "%d", blueTeamScore);
    m_team2_score->SetText(str);
    if (max_score <= 0)
    {
        xr_strcpy(str, "--");
    }
    else
    {
        xr_sprintf(str, "%d", max_score);
    }
    m_pFragLimitIndicator->SetText(str);
    teamPanels->SetArtefactsCount(greenTeamScore, blueTeamScore);
}

void CUIGameCTA::OnFrame()
{
    inherited::OnFrame();
    if (m_voteStatusWnd)
        m_voteStatusWnd->Update();
}

void CUIGameCTA::Render()
{
    m_team1_icon->Draw();
    m_team2_icon->Draw();

    inherited::Render();

    if (m_voteStatusWnd)
        m_voteStatusWnd->Draw();
}

void CUIGameCTA::SetRoundResultCaption(LPCSTR str) { m_round_result_caption->SetTextST(str); }
void CUIGameCTA::SetPressBuyMsgCaption(LPCSTR str) { m_pressbuy_caption->SetTextST(str); }
void CUIGameCTA::SetPressJumpMsgCaption(LPCSTR str) { m_pressjump_caption->SetTextST(str); }
void CUIGameCTA::SetSpectatorMsgCaption(LPCSTR str) { m_spectator_caption->SetTextST(str); }
void CUIGameCTA::SetSpectrModeMsgCaption(LPCSTR str) { m_spectrmode_caption->SetTextST(str); }
void CUIGameCTA::SetWarmUpCaption(LPCSTR str) { m_warm_up_caption->SetTextST(str); }
void CUIGameCTA::SetTimeMsgCaption(LPCSTR str) { m_time_caption->SetTextST(str); }
void CUIGameCTA::SetDemoPlayCaption(LPCSTR str) { m_demo_play_caption->SetTextST(str); }
void CUIGameCTA::ResetCaptions()
{
    // bad ...
    SetRoundResultCaption(NULL);
    SetPressBuyMsgCaption(NULL);
    SetPressJumpMsgCaption(NULL);
    SetSpectatorMsgCaption(NULL);
    SetWarmUpCaption(NULL);
    SetTimeMsgCaption(NULL);
}

bool CUIGameCTA::IsBuySpawnShown()
{
    if (!m_pBuySpawnMsgBox)
        return false;

    if (m_pBuySpawnMsgBox->IsShown())
        return true;

    return false;
}

void CUIGameCTA::ShowBuySpawn(s32 spawn_cost)
{
    VERIFY(m_pBuySpawnMsgBox);
    VERIFY(Game().local_player);

    if (m_pBuySpawnMsgBox->IsShown())
        return;

    CStringTable st;
    LPCSTR format_str = st.translate("mp_press_yes2pay").c_str();
    VERIFY(format_str);
    size_t pay_frm_size = xr_strlen(format_str) * sizeof(char) + 64;
    PSTR pay_frm_str = static_cast<char*>(_alloca(pay_frm_size));

    xr_sprintf(pay_frm_str, pay_frm_size, format_str, abs(Game().local_player->money_for_round), abs(spawn_cost));

    m_pBuySpawnMsgBox->SetText(pay_frm_str);
    m_pBuySpawnMsgBox->ShowDialog(true);
}

void CUIGameCTA::HideBuySpawn()
{
    if (IsBuySpawnShown())
    {
        m_pBuySpawnMsgBox->HideDialog();
    }
}

void CUIGameCTA::SetVoteMessage(LPCSTR str)
{
    if (m_voteStatusWnd)
    {
        xr_delete(m_voteStatusWnd);
    }
    if (str)
    {
        CUIXml uiXml;
        uiXml.Load(CONFIG_PATH, UI_PATH, "ui_game_dm.xml");
        m_voteStatusWnd = new UIVoteStatusWnd();
        m_voteStatusWnd->InitFromXML(uiXml);
        m_voteStatusWnd->Show(true);
        m_voteStatusWnd->SetVoteMsg(str);
    }
};

void CUIGameCTA::SetVoteTimeResultMsg(LPCSTR str)
{
    if (m_voteStatusWnd)
        m_voteStatusWnd->SetVoteTimeResultMsg(str);
}

bool CUIGameCTA::IR_UIOnKeyboardPress(int dik)
{
    if (inherited::IR_UIOnKeyboardPress(dik))
        return true;

    switch (dik)
    {
    case SDL_SCANCODE_CAPSLOCK:
    {
        if (m_game)
        {
            if (m_game->Get_ShowPlayerNamesEnabled())
                m_game->Set_ShowPlayerNames(!m_game->Get_ShowPlayerNames());
            else
                m_game->Set_ShowPlayerNames(true);
            return true;
        };
    }
    break;
    }

    EGameActions cmd = get_binded_action(dik);
    switch (cmd)
    {
    case kINVENTORY:
    case kBUY:
    case kSKIN:
    case kTEAM:

    case kSPEECH_MENU_0:
    case kSPEECH_MENU_1: { return Game().OnKeyboardPress(cmd);
    }
    break;
    }

    return false;
}

bool CUIGameCTA::IR_UIOnKeyboardRelease(int dik)
{
    if (inherited::IR_UIOnKeyboardRelease(dik))
        return true;

    switch (dik)
    {
    case SDL_SCANCODE_CAPSLOCK:
    {
        if (m_game)
        {
            if (!m_game->Get_ShowPlayerNamesEnabled())
                m_game->Set_ShowPlayerNames(false);
            return true;
        };
    }
    break;
    }
    return false;
}

s16 CUIGameCTA::GetBuyMenuItemIndex(u8 Addons, u8 ItemID)
{
    s16 ID = (s16(Addons) << 0x08) | s16(ItemID);
    return ID;
};

void CUIGameCTA::LoadTeamDefaultPresetItems(const shared_str& caSection)
{
    if (!pSettings->line_exist(caSection, "default_items"))
        return;
    if (!m_pCurBuyMenu)
        return;

    PlayerDefItems.clear();

    string256 ItemName;
    string4096 DefItems;
    // Читаем данные этого поля
    xr_strcpy(DefItems, pSettings->r_string(caSection, "default_items"));
    u32 count = _GetItemCount(DefItems);
    // теперь для каждое имя оружия, разделенные запятыми, заносим в массив
    for (u32 i = 0; i < count; ++i)
    {
        _GetItem(DefItems, i, ItemName);

        u8 SlotID, ItemID;
        m_pCurBuyMenu->GetWeaponIndexByName(ItemName, SlotID, ItemID);
        if (SlotID == 0xff || ItemID == 0xff)
            continue;
        //		s16 ID = GetBuyMenuItemIndex(SlotID, ItemID);
        s16 ID = GetBuyMenuItemIndex(0, ItemID);
        PlayerDefItems.push_back(ID);
    };
};

void CUIGameCTA::LoadDefItemsForRank()
{
    R_ASSERT(m_pCurBuyMenu);
    R_ASSERT(m_game);
    R_ASSERT(m_game->local_player);
    //---------------------------------------------------
    game_PlayerState* local_player = m_game->local_player;
    LoadTeamDefaultPresetItems(m_game->getTeamSection(local_player->team));
    //---------------------------------------------------
    string16 RankStr;
    string256 ItemStr;
    string256 NewItemStr;
    char tmp[5];
    for (int i = 1; i <= local_player->rank; i++)
    {
        strconcat(sizeof(RankStr), RankStr, "rank_", xr_itoa(i, tmp, 10));
        if (!pSettings->section_exist(RankStr))
            continue;
        for (u32 it = 0; it < PlayerDefItems.size(); it++)
        {
            //			s16* pItemID = &(PlayerDefItems[it]);
            //			char* ItemName = pBuyMenu->GetWeaponNameByIndex(u8(((*pItemID)&0xff00)>>0x08),
            // u8((*pItemID)&0x00ff));
            PresetItem* pDefItem = &(PlayerDefItems[it]);
            const shared_str& ItemName = m_pCurBuyMenu->GetWeaponNameByIndex(pDefItem->SlotID, pDefItem->ItemID);
            if (!ItemName.size())
                continue;
            strconcat(sizeof(ItemStr), ItemStr, "def_item_repl_", ItemName.c_str());
            if (!pSettings->line_exist(RankStr, ItemStr))
                continue;

            xr_strcpy(NewItemStr, sizeof(NewItemStr), pSettings->r_string(RankStr, ItemStr));

            u8 SlotID, ItemID;
            m_pCurBuyMenu->GetWeaponIndexByName(NewItemStr, SlotID, ItemID);
            if (SlotID == 0xff || ItemID == 0xff)
                continue;

            //			s16 ID = GetBuyMenuItemIndex(SlotID, ItemID);
            s16 ID = GetBuyMenuItemIndex(0, ItemID);

            //			*pItemID = ID;
            pDefItem->set(ID);
        }
    }
    //---------------------------------------------------------
    for (u32 it = 0; it < PlayerDefItems.size(); it++)
    {
        //		s16* pItemID = &(PlayerDefItems[it]);
        //		char* ItemName = pBuyMenu->GetWeaponNameByIndex(u8(((*pItemID)&0xff00)>>0x08), u8((*pItemID)&0x00ff));
        PresetItem* pDefItem = &(PlayerDefItems[it]);
        const shared_str& ItemName = m_pCurBuyMenu->GetWeaponNameByIndex(pDefItem->SlotID, pDefItem->ItemID);
        if (!ItemName.size())
            continue;
        if (!xr_strcmp(*ItemName, "mp_wpn_knife"))
            continue;
        if (!pSettings->line_exist(ItemName, "ammo_class"))
            continue;

        string1024 wpnAmmos, BaseAmmoName;
        xr_strcpy(wpnAmmos, pSettings->r_string(ItemName, "ammo_class"));
        _GetItem(wpnAmmos, 0, BaseAmmoName);

        u8 SlotID, ItemID;
        m_pCurBuyMenu->GetWeaponIndexByName(BaseAmmoName, SlotID, ItemID);
        if (SlotID == 0xff || ItemID == 0xff)
            continue;

        //		s16 ID = GetBuyMenuItemIndex(SlotID, ItemID);

        s16 ID = GetBuyMenuItemIndex(0, ItemID);
        PlayerDefItems.push_back(ID);
        PlayerDefItems.push_back(ID);
    };
};
