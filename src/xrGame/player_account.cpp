#include "StdAfx.h"
#include "player_account.h"
#include "MainMenu.h"
#include "login_manager.h"
#include "profile_store.h"

using namespace gamespy_profile;

player_account::player_account() : m_player_name(""), m_clan_name(""), m_clan_leader(false), m_online_account(false) {}

void player_account::load_account()
{
    const auto tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    const auto tmp_store = MainMenu()->GetProfileStore();
    VERIFY(tmp_store);
    const auto tmp_curr_prof = tmp_lmngr->get_current_profile();

    if (!tmp_curr_prof)
    {
        Msg("* WARNING: player not logged in");
    }

    if (tmp_curr_prof)
    {
        m_player_name = tmp_curr_prof->m_unique_nick;
        m_online_account = tmp_curr_prof->online();
        m_profile_id = static_cast<u32>(tmp_curr_prof->profile_id());
    }
    else
    {
        m_player_name = "";
        m_online_account = false;
        m_profile_id = 0;
    }
    m_clan_name = "";
    m_clan_leader = false;
}

void player_account::net_Import(NET_Packet& P)
{
    P.r_u32(m_profile_id);
    P.r_stringZ(m_player_name);
    P.r_stringZ(m_clan_name);

    m_clan_leader = P.r_u8() ? true : false;
    m_online_account = P.r_u8() ? true : false;
    const u16 awards_count = P.r_u16();
    for (u16 i = 0; i < awards_count; ++i)
    {
        std::ignore = P.r_u16();
        std::ignore = P.r_u16();
    }
}

void player_account::skip_Import(NET_Packet& P)
{
    string256 tmp_string;
    P.r_u32();
    P.r_stringZ_s(tmp_string);
    P.r_stringZ_s(tmp_string);
    P.r_u8();
    P.r_u8();
    const u16 awards_count = P.r_u16();
    for (u16 i = 0; i < awards_count; ++i)
    {
        std::ignore = P.r_u16();
        std::ignore = P.r_u16();
    }
}

void player_account::net_Export(NET_Packet& P)
{
    P.w_u32(m_profile_id);
    P.w_stringZ(m_player_name);
    P.w_stringZ(m_clan_name);
    P.w_u8(m_clan_leader ? 1 : 0);
    P.w_u8(m_online_account ? 1 : 0);
    P.w_u16(0); // awards
}

void player_account::set_player_name(char const* new_name)
{
    R_ASSERT(!is_online());
    m_player_name = new_name;
}
