#pragma once
#ifndef UIGAMECTA
#define UIGAMECTA

#include "UIGameMP.h"
#include "game_base.h"
#include "Inventory.h"
#include "xrCommon/xr_vector.h"
#include "xrCore/buffer_vector.h"

class UITeamPanels;

class CUIMapDesc;
class CUIMoneyIndicator;
class CUIRankIndicator;
class UITeamPanels;
class CUISpawnWnd;
class IBuyWnd;
class CWeapon;
class CWeaponMagazinedWGrenade;
class CUISkinSelectorWnd;
class CUIProgressShape;
class CUIMessageBoxEx;
class UIVoteStatusWnd;
class game_cl_CaptureTheArtefact;
class CUITextWnd;

/// This class used to control UI part of client for Capture the Artefact mp game mode.
class CUIGameCTA : public UIGameMP
{
    CUISpawnWnd* m_pUITeamSelectWnd;
    CUIStatic* m_team1_icon;
    CUIStatic* m_team2_icon;
    CUITextWnd* m_team1_score;
    CUITextWnd* m_team2_score;
    CUITextWnd* m_pFragLimitIndicator;
    game_cl_CaptureTheArtefact* m_game;

    CUIMoneyIndicator* m_pMoneyIndicator;
    CUIRankIndicator* m_pRankIndicator;
    CUITextWnd* m_pReinforcementInidcator;
    CUIMessageBoxEx* m_pBuySpawnMsgBox;
    UIVoteStatusWnd* m_voteStatusWnd;

    shared_str m_teamSectionForBuyMenu;
    IBuyWnd* m_pCurBuyMenu;
    shared_str m_teamSectionForSkinMenu;
    CUISkinSelectorWnd* m_pCurSkinMenu;
    shared_str m_costSection;

    /// This window shows the player lists.
    UITeamPanels* teamPanels;
    bool m_team_panels_shown;

    CUITextWnd* m_spectator_caption;
    CUITextWnd* m_pressjump_caption;
    CUITextWnd* m_pressbuy_caption;
    CUITextWnd* m_round_result_caption;
    CUITextWnd* m_force_respawn_time_caption;
    CUITextWnd* m_spectrmode_caption;
    CUITextWnd* m_warm_up_caption;
    CUITextWnd* m_time_caption;
    CUITextWnd* m_demo_play_caption;

    struct PresetItem
    {
        u8 SlotID;
        u8 ItemID;
        s16 BigID;
        PresetItem(u8 Slot, u8 Item) { set(Slot, Item); };
        PresetItem(s16 Big) { set(Big); };
        bool operator==(const s16& ID) { return (BigID) == (ID); }
        void set(s16 Big)
        {
            SlotID = u8((Big >> 0x08) & 0x00ff);
            ItemID = u8(Big & 0x00ff);
            BigID = Big;
        }
        void set(u8 Slot, u8 Item)
        {
            SlotID = Slot;
            ItemID = Item;
            BigID = (s16(SlotID) << 0x08) | s16(ItemID);
        };
    };

    using PRESET_ITEMS = xr_vector<PresetItem>;

    PRESET_ITEMS PlayerDefItems;

    typedef UIGameMP inherited;

    typedef buffer_vector<shared_str> aditional_ammo_t;
    void TryToDefuseAllWeapons(aditional_ammo_t& dest_ammo);
    void AdditionalAmmoInserter(aditional_ammo_t::value_type const& sect_name);

    void BuyMenuItemInserter(PIItem const& item);
    void BuyMenuItemInserter(CInventorySlot const& slot);
    void SetPlayerItemsToBuyMenu();
    void SetPlayerParamsToBuyMenu();
    void SetPlayerDefItemsToBuyMenu();
    void LoadTeamDefaultPresetItems(const shared_str& caSection);
    void LoadDefItemsForRank();
    s16 GetBuyMenuItemIndex(u8 Addons, u8 ItemID);

public:
    typedef std::pair<u8, u8> BuyMenuItemPair;
    typedef xr_vector<BuyMenuItemPair> BuyMenuItemsCollection;

    CUIGameCTA();
    virtual ~CUIGameCTA();
    virtual void SetClGame(game_cl_GameState* g);
    virtual void Init(int stage);
    virtual void UnLoad();
    virtual void OnFrame();
    virtual void Render();

    virtual bool IR_UIOnKeyboardPress(int dik);
    virtual bool IR_UIOnKeyboardRelease(int dik);

    bool IsTeamPanelsShown();
    void ShowTeamPanels(bool bShow);
    void UpdateTeamPanels();

    bool IsTeamSelectShown();
    void ShowTeamSelectMenu();

    void UpdateBuyMenu(shared_str const& teamSection, shared_str const& costSection);
    bool CanBuyItem(shared_str const& sect_name);

    void ShowBuyMenu();
    void HideBuyMenu();
    BuyMenuItemPair GetBuyMenuItem(shared_str const& itemSectionName);
    void GetPurchaseItems(BuyMenuItemsCollection& dest, s32& moneyDif);
    IBuyWnd* GetBuyWnd() const { return m_pCurBuyMenu; };
    void ReInitPlayerDefItems();

    bool IsBuySpawnShown();
    void ShowBuySpawn(s32 spawn_cost);
    void HideBuySpawn();

    void UpdateSkinMenu(shared_str const& teamSection);
    void ShowSkinMenu(s8 currentSkin);
    s8 GetSelectedSkinIndex();

    void AddPlayer(ClientID const& clientId);
    void RemovePlayer(ClientID const& clientId);
    void UpdatePlayer(ClientID const& clientId);

    void SetReinforcementTimes(u32 curTime, u32 maxTime);

    virtual void ChangeTotalMoneyIndicator(LPCSTR newMoneyString);
    virtual void DisplayMoneyChange(LPCSTR deltaMoney);
    virtual void DisplayMoneyBonus(KillMessageStruct* bonus);

    void SetRank(ETeam team, u8 rank);
    void SetScore(s32 max_score, s32 greenTeamScore, s32 blueTeamScore);

    void SetRoundResultCaption(LPCSTR str);
    void SetPressBuyMsgCaption(LPCSTR str);
    void SetPressJumpMsgCaption(LPCSTR str);
    void SetSpectatorMsgCaption(LPCSTR str);
    void SetSpectrModeMsgCaption(LPCSTR str);
    void SetWarmUpCaption(LPCSTR str);
    void SetTimeMsgCaption(LPCSTR str);
    void SetDemoPlayCaption(LPCSTR str);
    void ResetCaptions();

    void SetVoteMessage(LPCSTR str);
    void SetVoteTimeResultMsg(LPCSTR str);
};

#endif
