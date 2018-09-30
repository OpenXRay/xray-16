#pragma once

#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/ListBox/UIListBoxItem.h"

struct LIST_SRV_SIZES
{
    float icon;
    float server;
    float map;
    float game;
    float players;
    float ping;
    float version;
    float height;
};

struct SRV_ICONS
{
    bool pass;
    bool dedicated;
    bool punkbuster;
    bool user_pass;
};

struct LIST_SRV_INFO
{
    shared_str server;
    shared_str address;
    shared_str map;
    shared_str game;
    shared_str players;
    shared_str ping;
    shared_str version;
    SRV_ICONS icons;
    int Index;
};

struct LIST_SRV_ITEM
{
    u32 text_color;
    CGameFont* text_font;
    LIST_SRV_SIZES size;
    LIST_SRV_INFO info;
};

class CUIListItemServer : public CUIListBoxItem
{
    typedef CUIListBoxItem inherited;

public:
    CUIListItemServer(float height);

    void InitItemServer(LIST_SRV_ITEM& params /*, Fvector2 size*/);
    void SetParams(LIST_SRV_ITEM& params);
    void CreateConsoleCommand(xr_string& command, LPCSTR player_name, LPCSTR player_pass, LPCSTR server_psw);

    int Get_gs_index() { return m_srv_info.info.Index; }
    LIST_SRV_ITEM* GetInfo() { return &m_srv_info; };
protected:
    LIST_SRV_SIZES m_sizes;

    LIST_SRV_ITEM m_srv_info;
    CUIStatic* m_iconPass;
    CUIStatic* m_iconDedicated;
    //	CUIStatic* 				m_iconPunkBuster;
    CUIStatic* m_iconUserPass;
    CUITextWnd* m_server;
    CUITextWnd* m_map;
    CUITextWnd* m_game;
    CUITextWnd* m_players;
    CUITextWnd* m_ping;
    CUITextWnd* m_version;
};
