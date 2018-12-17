#pragma once

#include "game_cl_base.h"
#include "xrUICore/ui_defs.h"
#include "Spectator.h"
#include "file_transfer.h"
#include "screenshot_manager.h"
#include "configs_dumper.h"
#include "configs_dump_verifyer.h"
#include "screenshot_server.h"
#include "xrCore/fastdelegate.h"

class CUIWindow;
class CUISpeechMenu;
class CUIMessageBoxEx;

namespace award_system
{
class reward_manager;
} // namespace award_system

struct SND_Message
{
    ref_sound pSound;
    u32 priority;
    u32 SoundID;
    u32 LastStarted;
    bool operator==(u32 ID) { return SoundID == ID; }
    void Load(u32 const ID, u32 const prior, LPCSTR name)
    {
        SoundID = ID;
        priority = prior;
        pSound.create(name, st_Effect, sg_SourceType);
        LastStarted = 0;
    }
    ~SND_Message()
    {
        SoundID = 0;
        priority = 0;
        pSound.destroy();
    }
};

struct cl_TeamStruct
{
    shared_str caSection; // имя секции комманды
    //-----------------------------------
    ui_shader IndicatorShader;
    ui_shader InvincibleShader;

    Fvector IndicatorPos;
    float Indicator_r1;
    float Indicator_r2;
};

using CL_TEAM_DATA_LIST = xr_deque<cl_TeamStruct>;

struct cl_Message_Sound
{
    ref_sound mSound_Voice;
    ref_sound mSound_Radio;
};

using TEAMSOUND = xr_vector<cl_Message_Sound>;

struct cl_Menu_Message
{
    shared_str pMessage;
    using SOUND_VARIANTS = xr_vector<TEAMSOUND>;
    SOUND_VARIANTS aVariants;
};

struct cl_MessageMenu
{
    CUISpeechMenu* m_pSpeechMenu;
    using MENUMESSAGES = xr_vector<cl_Menu_Message>;
    MENUMESSAGES m_aMessages;

    bool operator==(CUISpeechMenu* pMenu) { return pMenu == m_pSpeechMenu; }
};

struct Bonus_Struct
{
    shared_str BonusName;
    shared_str BonusTypeName;
    shared_str MoneyStr;
    int Money;
    ui_shader IconShader;
    // ref_shader	IconShader;
    xr_vector<Frect> IconRects;
    Bonus_Struct()
    {
        BonusTypeName = "";
        BonusName = "";
        MoneyStr = "";
        Money = 0;
        // IconShader = NULL;
        IconRects.clear();
    }
    ~Bonus_Struct()
    {
        // if (IconShader)
        //	IconShader.destroy();
        // IconShader = NULL;
        IconRects.clear();
    }

    bool operator==(LPCSTR TypeName) { return !xr_strcmp(BonusTypeName.c_str(), TypeName); }
};

class CUIVotingCategory;
class CUIVote;
class CUIMessageBoxEx;
class CUIMpAdminMenu;

namespace award_system
{
class reward_event_generator;
}

class game_cl_mp : public game_cl_GameState
{
    typedef game_cl_GameState inherited;

protected:
    CL_TEAM_DATA_LIST TeamList;

    using SNDMESSAGES = xr_vector<SND_Message*>;
    SNDMESSAGES m_pSndMessages;
    bool m_bJustRestarted;
    using SNDMESSAGESINPLAY = xr_vector<SND_Message*>;
    SNDMESSAGESINPLAY m_pSndMessagesInPlay;

    using BONUSES = xr_vector<Bonus_Struct>;
    BONUSES m_pBonusList;

    bool m_bVotingActive;
    CUIVotingCategory* m_pVoteStartWindow;
    CUIMpAdminMenu* m_pAdminMenuWindow;
    CUIVote* m_pVoteRespondWindow;
    CUIMessageBoxEx* m_pMessageBox;
    BOOL m_bSpectatorSelected;

    virtual void LoadTeamData(const shared_str& TeamName);

    virtual void ChatSay(LPCSTR phrase, bool bAll);

    virtual void OnChatMessage(NET_Packet* P);
    virtual void OnWarnMessage(NET_Packet* P);
    virtual void OnRadminMessage(u16 type, NET_Packet* P);

    virtual void UpdateMapLocations(){};

    ui_shader m_EquipmentIconsShader;
    ui_shader m_KillEventIconsShader;
    ui_shader m_RadiationIconsShader;
    ui_shader m_BloodLossIconsShader;
    ui_shader m_RankIconsShader;

    virtual const ui_shader& GetEquipmentIconsShader();
    virtual const ui_shader& GetKillEventIconsShader();
    virtual const ui_shader& GetRadiationIconsShader();
    virtual const ui_shader& GetBloodLossIconsShader();
    virtual const ui_shader& GetRankIconsShader();

    virtual void OnPlayerKilled(NET_Packet& P);

    virtual bool NeedToSendReady_Actor(int key, game_PlayerState* ps);
    virtual bool NeedToSendReady_Spectator(int key, game_PlayerState* ps);

    virtual void LoadSndMessage(LPCSTR caSection, LPCSTR caLine, u32 ID);
    virtual void LoadSndMessages();
    virtual void UpdateSndMessages();

    u8 m_u8SpectatorModes;
    bool m_bSpectator_FreeFly;
    bool m_bSpectator_FirstEye;
    bool m_bSpectator_LookAt;
    bool m_bSpectator_FreeLook;
    bool m_bSpectator_TeamCamera;

    u32 m_cur_MenuID;

    virtual void LoadBonuses();

    award_system::reward_event_generator* m_reward_generator;
    void ReInitRewardGenerator(game_PlayerState* local_ps);

public:
    game_cl_mp();
    virtual ~game_cl_mp();

    void xr_stdcall OnBuySpawn(CUIWindow* pWnd, void* p);
    virtual void TranslateGameMessage(u32 msg, NET_Packet& P);

    virtual bool OnKeyboardPress(int key);

    virtual bool CanBeReady();
    virtual void shedule_Update(u32 dt);
    bool IsLocalPlayerInitialized() const;

    //// VOTING
    virtual bool IsVotingActive() { return m_bVotingActive; };
    virtual void SetVotingActive(bool Active) { m_bVotingActive = Active; };
    virtual void SendStartVoteMessage(LPCSTR args);
    virtual void SendVoteYesMessage();
    virtual void SendVoteNoMessage();
    void VotingBegin();
    void Vote();
    void OnCantVoteMsg(LPCSTR Text);
    virtual void OnVoteStart(NET_Packet& P);
    virtual void OnVoteStop(NET_Packet& P);
    virtual void OnVoteEnd(NET_Packet& P);
    void GetActiveVoting();
    virtual void OnPlayerChangeName(NET_Packet& P);
    virtual void OnPlayerVoted(game_PlayerState* ps);
    virtual void OnSpectatorSelect();
    virtual void OnSkinMenuBack(){};
    virtual void OnTeamMenuBack(){};
    virtual void OnTeamMenu_Cancel(){};
    virtual void OnMapInfoAccept(){};
    virtual void OnSkinMenu_Ok(){};
    virtual void OnSkinMenu_Cancel(){};
    virtual void OnBuySpawnMenu_Ok(){};
    virtual void OnSellItemsFromRuck(){};

    virtual void OnTeamSelect(int Result){};
    virtual void OnBuyMenu_Ok(){};
    virtual void OnBuyMenu_Cancel(){};

    virtual void OnGameMenuRespond(NET_Packet& P);
    virtual void OnGameMenuRespond_Spectator(NET_Packet& P){};
    virtual void OnGameMenuRespond_ChangeTeam(NET_Packet& P){};
    virtual void OnGameMenuRespond_ChangeSkin(NET_Packet& P){};
    virtual void OnGameRoundStarted();

    virtual void OnSwitchPhase(u32 old_phase, u32 new_phase);
    virtual void net_import_update(NET_Packet& P);
    virtual void net_import_state(NET_Packet& P);
    virtual void OnRankChanged(u8 OldRank);
    virtual void OnTeamChanged(){};
    virtual void OnMoneyChanged(){};
    virtual void OnEventMoneyChanged(NET_Packet& P);

    virtual void OnSwitchPhase_InProgress();

    virtual u8 GetTeamCount() { return 0; };
    virtual s16 ModifyTeam(s16 Team) { return Team; };
    virtual bool Is_Spectator_TeamCamera_Allowed() { return m_bSpectator_TeamCamera && !Level().IsDemoPlay(); };
    virtual bool Is_Spectator_Camera_Allowed(CSpectator::EActorCameras Camera);
    virtual bool Is_Rewarding_Allowed() const = 0;

    void SendPlayerStarted();
    virtual void OnConnected();
    virtual LPCSTR GetGameScore(string32& score_dest) = 0;

    screenshot_manager ss_manager;
    mp_anticheat::configs_dumper cd_manager;
    mp_anticheat::configs_verifyer cd_verifyer;

    award_system::reward_event_generator* get_reward_generator() const { return m_reward_generator; };
    void AddRewardTask(u32 const award_id);

    void AddSoundMessage(LPCSTR sound_name, u32 const sound_priority, u32 const soundID);
    void PlaySndMessage(u32 ID);
    typedef fastdelegate::FastDelegate<void(u32 const)> player_info_reply_cb_t;
    bool RequestPlayersInfo(player_info_reply_cb_t const pinfo_repl_cb);

private:
    u8* buffer_for_compress;
    u32 buffer_for_compress_size;
    CMemoryWriter upload_memory_writer;
    void reinit_compress_buffer(u32 need_size);
    void deinit_compress_buffer();

    award_system::reward_manager* m_reward_manager;
    void start_receive_server_info(ClientID const& svclient_id);

    player_info_reply_cb_t m_players_info_reply;
    void ProcessPlayersInfoReply(NET_Packet& P);

public:
    void __stdcall SendCollectedData(u8 const* buffer, u32 buffer_size, u32 uncompressed_size);
    void PrepareToReceiveFile(
        ClientID const& from_client, shared_str const& client_session_id, clientdata_event_t response_event);

    struct fr_callback_binder
    {
        file_transfer::filereceiver_node* m_frnode;
        shared_str m_file_name;
        clientdata_event_t m_response_type;
        bool m_active;
        u32 m_downloaded_size;
        u32 m_max_size;
        game_cl_mp* m_owner;
        CMemoryWriter m_writer;
        fr_callback_binder() : m_frnode(NULL), m_active(false){}
        void __stdcall receiving_file_callback(
            file_transfer::receiving_status_t status, u32 bytes_received, u32 data_size);
        void __stdcall receiving_serverinfo_callback(
            file_transfer::receiving_status_t status, u32 bytes_received, u32 data_size);
    };
    struct detected_cheater_t
    {
        shared_str m_file_name;
        string256 m_diff;
        u32 m_detect_time;
        static u32 const max_showing_time = 10000; // 10 seconds
    }; // struct detected_cheater_t
    void add_detected_cheater(shared_str const& file_name, string256 diff);

private:
    fr_callback_binder m_client_receiver_cbs[MAX_PLAYERS_COUNT];
    typedef xr_vector<detected_cheater_t> cheaters_collection_t;
    cheaters_collection_t m_detected_cheaters;
    bool m_ready_to_open_buy_menu;

public:
    bool is_buy_menu_ready() const { return m_ready_to_open_buy_menu; }
    void set_buy_menu_not_ready() { m_ready_to_open_buy_menu = false; }
    void decompress_and_save_screenshot(LPCSTR file_name, u8* data, u32 data_size, u32 file_size);
    void decompress_and_process_config(LPCSTR file_name, u8* data, u32 data_size, u32 file_size);

    void extract_server_info(u8* data_ptr, u32 data_size);
    fr_callback_binder* get_receiver_cb_binder();
    void draw_all_active_binder_states();
    void draw_downloads(bool draw);

    void __stdcall sending_screenshot_callback(file_transfer::sending_status_t status, u32 bytes_sent, u32 data_size);
    //-------------------------------------------------------------------------------------------------
    static xr_string generate_file_name(const xr_string& base_name, const time_t* date_time = nullptr);
    static xr_string sanitize_filename(const xr_string& base_name);
//-------------------------------------------------------------------------------------------------
#include "game_cl_mp_messages_menu.h"
};
