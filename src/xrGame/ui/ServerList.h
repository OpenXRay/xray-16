#pragma once

#include "xrUICore/Windows/UIWindow.h"

#include "xrUICore/ListBox/UIListBox.h"
#include "UIListItemServer.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "mixed_delegate.h"
#include "xrGameSpy/GameSpy_BrowsersWrapper.h"

class CUIXml;
class CUIMessageBoxEx;
struct ServerInfo;
struct GameInfo;

#define LST_SERVER 0
#define LST_SRV_PROP 1
#define LST_PLAYERS 2
#define DIF_HEIGHT 180
#define LST_COLUMN_COUNT 7

enum enum_connect_error
{
    ece_unique_nick_not_registred = 0x01,
    ece_unique_nick_expired
}; // enum enum_connect_errors

typedef mixed_delegate<void(enum_connect_error, char const*)> connect_error_cb;

class SServerFilters
{
public:
    bool empty;
    bool full;
    bool with_pass;
    bool without_pass;
    bool without_ff;
    bool listen_servers;
};

class CServerList : public CUIWindow
{
public:
    enum ESortingMode
    {
        SORT_SERVERNAME,
        SORT_MAP,
        SORT_GAMETYPE,
        SORT_PLAYERSCOUNT,
        SORT_PING,
        SORT_GAMEVERSION,
    };

    enum ESortingType
    {
        SORT_TYPE_ASCENDING,
        SORT_TYPE_DESCENDING,
        SORT_TYPE_AUTO,
    };

    CServerList();
    virtual ~CServerList();

    virtual void Update();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
    void InitFromXml(CUIXml& xml_doc, LPCSTR path);
    void InitHeader();
    void InitSeparator();
    void AddServerToList(ServerInfo* pServerInfo);
    void UpdateServerInList(ServerInfo* pServerInfo, int index);
    void UpdateServerInList(ServerInfo* pServerInfo, CUIListItemServer* pItem);
    void SetConnectionErrCb(connect_error_cb conn_err_cb) { m_connect_cb = conn_err_cb; }
    void ConnectToSelected();
    void SetFilters(SServerFilters& sf);
    void SetPlayerName(LPCSTR name);
    void SetSortFunc(LPCSTR func_name, bool make_sort);
    void NetRadioChanged(bool Local);
    virtual void RefreshGameSpyList(bool Local);
    void RefreshQuick();
    void ShowServerInfo();
    virtual void RefreshList();

private:
    void xr_stdcall OnUpdate() { RefreshList(); }

protected:
    bool IsValidItem(ServerInfo& item);
    void SrvInfo2LstSrvInfo(const ServerInfo* pSrvInfo);
    void UpdateSizes();
    void UpdateVisibility();
    void BeforeAppear();
    void AfterAppear();
    void BeforeDisapear();
    void AfterDisappear();
    void AddServerDetail(const GameInfo& info);
    void AddBoolED(const char* keyName, bool value);
    void AddBoolYN(const char* keyName, bool value);
    void AddBoolKeyED(void* s, const char* keyName, int k);
    void AddBoolKeyYN(void* s, const char* keyName, int k);
    void AddIntKey(void* s, const char* keyName, int k);
    void AddIntKeyN(void* s, float m, const char* keyName, const char* suffix, int k);
    void AddTimeKey(void* s, const char* keyName, const char* format, const char* suffix, int k);
    void AddString(const char* key, const char* value);
    void AddStringSt(const char* key, const char* value);
    void SetSortFunc_internal(ESortingMode sort_mode, ESortingType sorting_type, bool make_sort);

    void FillUpDetailedServerInfo();
    void ClearDetailedServerInfo();

    void SaveCurItem();
    void RestoreCurItem();
    void ResetCurItem();
    bool NeedToRefreshCurServer();

    LIST_SRV_ITEM m_itemInfo;
    SServerFilters m_sf;
    //	CUIListWnd		m_list[3];
    CUIListBox m_list[3];
    CUIFrameWindow m_frame[3];
    CUI3tButton m_header[LST_COLUMN_COUNT];
    CUIFrameLineWnd m_header2[4];
    CUIFrameLineWnd m_header_frames[LST_COLUMN_COUNT];
    CUIEditBox m_edit_gs_filter;
    xr_string m_playerName;
    bool m_bShowServerInfo;
    bool m_bAnimation;
    float m_fListH[2];
    float m_fEditPos[2];
    int m_cur_item;
    bool m_b_local;

    CUIMessageBoxEx* m_message_box;
    CUIMessageBoxEx* m_version_switch_msgbox;

    ESortingMode m_sort_mode;
    bool m_sort_ascending;
    xr_vector<int> m_tmp_srv_lst;
    struct SrvItem
    {
        CUIListItemServer* m_ui_item;
        bool m_busy;
        SrvItem(float h)
        {
            m_ui_item = xr_new<CUIListItemServer>(h);
            m_busy = true;
        }
    };
    CUIListItemServer* GetFreeItem();
    void DestroySrvItems();
    void ClearSrvItems();

    typedef xr_vector<SrvItem> SrvItems;
    typedef SrvItems::iterator SrvItems_It;
    SrvItems m_items_cache;
    u32 m_last_retreived_index;
    u32 m_need_refresh_fr;
    void RefreshList_internal();

    CGameSpy_BrowsersWrapper::SubscriberIdx m_subscriber_id;

private:
    connect_error_cb m_connect_cb;
    static inline CGameSpy_BrowsersWrapper* browser_LL();
    static inline CGameSpy_BrowsersWrapper& browser();
};
