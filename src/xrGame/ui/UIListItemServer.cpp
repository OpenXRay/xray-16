#include "StdAfx.h"

#include "UIListItemServer.h"
#include "xrUICore/XML/UITextureMaster.h"

CUIListItemServer::CUIListItemServer(float height) : inherited(height)
{
    m_iconPass = AddIconField(5.0f);
    m_iconDedicated = AddIconField(5.0f);
    //	m_iconPunkBuster	= AddIconField(5.0f);
    m_iconUserPass = AddIconField(5.0f);

    m_server = GetTextItem();
    m_map = AddTextField("", 5.0f);
    m_game = AddTextField("", 5.0f);
    m_players = AddTextField("", 5.0f);
    m_ping = AddTextField("", 5.0f);
    m_version = AddTextField("", 5.0f);
}

void CUIListItemServer::InitItemServer(LIST_SRV_ITEM& params)
{
    float offset = 0.0f;

    float icon_size = CUITextureMaster::GetTextureHeight("ui_icon_password");

    icon_size *= UI().get_current_kx();
    m_iconPass->SetStretchTexture(true);
    m_iconDedicated->SetStretchTexture(true);
    //	m_iconPunkBuster->SetStretchTexture	(true);
    m_iconUserPass->SetStretchTexture(true);

    float icon_y = (GetHeight() - icon_size) / 2.0f;

    m_iconPass->SetWndPos(Fvector2().set(offset, icon_y));
    m_iconPass->SetWndSize(Fvector2().set(icon_size, icon_size));
    m_iconPass->InitTexture("ui_icon_password");
    offset += icon_size;

    m_iconDedicated->SetWndPos(Fvector2().set(offset, icon_y));
    m_iconDedicated->SetWndSize(Fvector2().set(icon_size, icon_size));
    m_iconDedicated->InitTexture("ui_icon_dedicated");
    offset += icon_size;

    // m_iconPunkBuster->SetWndPos(Fvector2().set(offset, icon_y));
    // m_iconPunkBuster->SetWndSize(Fvector2().set(icon_size,icon_size));
    // m_iconPunkBuster->InitTexture("ui_icon_punkbuster");
    // offset					+= icon_size;

    m_iconUserPass->SetWndPos(Fvector2().set(offset, icon_y));
    m_iconUserPass->SetWndSize(Fvector2().set(icon_size, icon_size));
    m_iconUserPass->InitTexture("ui_icon_punkbuster");
    offset += icon_size;

    m_server->SetWndPos(Fvector2().set(offset, 0.0f));
    m_server->SetWidth(params.size.server);
    m_server->SetHeight(params.size.height);
    m_server->SetFont(params.text_font);
    m_server->SetTextColor(params.text_color);
    offset += params.size.server;

    m_map->SetWndPos(Fvector2().set(offset, 0.0f));
    m_map->SetWidth(params.size.map);
    m_map->SetHeight(params.size.height);
    m_map->SetFont(params.text_font);
    m_map->SetTextColor(params.text_color);
    offset += params.size.map;

    m_game->SetWndPos(Fvector2().set(offset, 0.0f));
    m_game->SetWidth(params.size.game);
    m_game->SetHeight(params.size.height);
    m_game->SetFont(params.text_font);
    m_game->SetTextColor(params.text_color);
    offset += params.size.game;

    m_players->SetWndPos(Fvector2().set(offset, 0.0f));
    m_players->SetWidth(params.size.players);
    m_players->SetHeight(params.size.height);
    m_players->SetFont(params.text_font);
    m_players->SetTextColor(params.text_color);
    offset += params.size.players;

    m_ping->SetWndPos(Fvector2().set(offset, 0.0f));
    m_ping->SetWidth(params.size.ping);
    m_ping->SetHeight(params.size.height);
    m_ping->SetFont(params.text_font);
    m_ping->SetTextColor(params.text_color);
    offset += params.size.ping;

    m_version->SetWndPos(Fvector2().set(offset, 0.0f));
    m_version->SetWidth(params.size.version);
    m_version->SetHeight(params.size.height);
    m_version->SetFont(params.text_font);
    m_version->SetTextColor(params.text_color);

    SetParams(params);

    m_srv_info = params;
}

#include "string_table.h"
u32 CutStringByLength(CGameFont* font, LPCSTR src, pstr dst, u32 dstSize, float length);

void CUIListItemServer::SetParams(LIST_SRV_ITEM& params)
{
    string1024 buff;

    LPCSTR _srv_name = StringTable().translate(params.info.server).c_str();
    CutStringByLength(m_map->GetFont(), _srv_name, buff, sizeof(buff), m_server->GetWidth());
    m_server->SetText(buff);

    LPCSTR _map_name = StringTable().translate(params.info.map).c_str();
    CutStringByLength(m_map->GetFont(), _map_name, buff, sizeof(buff), m_map->GetWidth());
    m_map->SetText(buff);

    LPCSTR _game_name = StringTable().translate(params.info.game).c_str();
    CutStringByLength(m_game->GetFont(), _game_name, buff, sizeof(buff), m_game->GetWidth());
    m_game->SetText(buff);

    m_players->SetText(params.info.players.c_str());
    m_ping->SetText(params.info.ping.c_str());
    m_version->SetText(params.info.version.c_str());

    m_iconPass->Show(params.info.icons.pass);
    m_iconDedicated->Show(params.info.icons.dedicated);
    //	m_iconPunkBuster->Show	(params.info.icons.punkbuster);
    m_iconUserPass->Show(params.info.icons.user_pass);

    SetTAG(params.info.Index);
}

void CUIListItemServer::CreateConsoleCommand(
    xr_string& command, LPCSTR player_name, LPCSTR player_pass, LPCSTR server_psw)
{
    command = "start client(";
    command += *m_srv_info.info.address;
    command += "/name=";
    command += player_name;
    command += "/pass=";
    command += player_pass;
    command += "/psw=";
    command += server_psw;
    command += ")";
}
