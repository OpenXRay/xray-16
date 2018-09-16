#include "StdAfx.h"
#include "cdkey_ban_list.h"
#include "Common/object_broker.h"

cdkey_ban_list::cdkey_ban_list() {}
cdkey_ban_list::~cdkey_ban_list()
{
    save();
    delete_data(m_ban_list);
}

void cdkey_ban_list::load()
{
    Msg("* Loading ban list...");
    string_path banlist_file;
    FS.update_path(banlist_file, "$app_data_root$", "banned_list.ltx");
    CInifile bl_ini(banlist_file);
    CInifile::Root& banlist = bl_ini.sections();
    for (CInifile::Root::iterator i = banlist.begin(), ie = banlist.end(); i != ie; ++i)
    {
        banned_client* tmp_client = new banned_client();
        if (tmp_client->load(&bl_ini, (*i)->Name))
        {
            m_ban_list.push_back(tmp_client);
        }
        else
        {
            Msg("! ERROR: load [%s] ban item section", (*i)->Name.size() > 0 ? (*i)->Name.c_str() : "");
            xr_delete(tmp_client);
        }
    }
    erase_expired_ban_items();
}
void cdkey_ban_list::save()
{
    string_path banlist_file;
    FS.update_path(banlist_file, "$app_data_root$", "banned_list.ltx");
    CInifile bl_ini(banlist_file, FALSE, FALSE, TRUE);
    string64 tmp_sect_name;
    u32 index = 0;

    for (ban_list_t::iterator i = m_ban_list.begin(), ie = m_ban_list.end(); i != ie; ++i)
    {
        xr_sprintf(tmp_sect_name, "client_%d", index);
        (*i)->save(&bl_ini, tmp_sect_name);
        ++index;
    }
}

bool cdkey_ban_list::is_player_banned(char const* hexstr_digest, shared_str& buy_who)
{
    if (!hexstr_digest)
        return false;

    Msg("* checking for ban player [%s]", hexstr_digest);

    erase_expired_ban_items();
    for (ban_list_t::iterator i = m_ban_list.begin(), ie = m_ban_list.end(); i != ie; ++i)
    {
        //		Msg("* comparing with cheater [%s]", (*i)->client_hexstr_digest.c_str());
        if (!xr_strcmp((*i)->client_hexstr_digest, hexstr_digest))
        {
            Msg("* found banned client [%s] by admin [%s]", hexstr_digest,
                (*i)->admin_name.size() ? (*i)->admin_name.c_str() : "Server");
            buy_who = (*i)->admin_name;
            return true;
        }
    }
    return false;
}

void cdkey_ban_list::ban_player(xrClientData const* player_data, s32 end_time_sec, xrClientData const* admin)
{
    banned_client* tmp_client = new banned_client();
    if (player_data->m_admin_rights.m_has_admin_rights)
    {
        Msg("! ERROR: Can't ban player with admin rights");
        xr_delete(tmp_client);
        return;
    }
    if (!player_data->m_cdkey_digest.size())
    {
        Msg("! ERROR: Can't ban client without unique digest, try to ban by IP address.");
        xr_delete(tmp_client);
        return;
    }
    tmp_client->client_hexstr_digest = player_data->m_cdkey_digest;
    tmp_client->client_ip_addr = player_data->m_cAddress;
    if (player_data->ps)
        tmp_client->client_name = player_data->ps->getName();
    else
        tmp_client->client_name = "unknown";

    time(&tmp_client->ban_start_time);
    tmp_client->ban_end_time = tmp_client->ban_start_time + static_cast<u32>(end_time_sec);
    if (admin)
    {
        tmp_client->admin_hexstr_digest = admin->m_cdkey_digest;
        tmp_client->admin_ip_addr = admin->m_cAddress;
        if (admin->ps)
            tmp_client->admin_name = admin->ps->getName();
        else
            tmp_client->admin_name = "unknown";
    }
    else
    {
        tmp_client->admin_hexstr_digest = "";
        tmp_client->admin_ip_addr.set("0.0.0.0");
        tmp_client->admin_name = "Server";
    }
    m_ban_list.push_back(tmp_client);
    save();
}

void cdkey_ban_list::ban_player_ll(char const* hexstr_digest, s32 end_time_sec, xrClientData const* admin)
{
    banned_client* tmp_client = new banned_client();
    if (!xr_strlen(hexstr_digest))
    {
        Msg("! ERROR: Can't ban client without unique digest, try to ban by IP address.");
        xr_delete(tmp_client);
        return;
    }
    tmp_client->client_hexstr_digest = hexstr_digest;
    tmp_client->client_ip_addr.m_data.data = 0;
    tmp_client->client_name = "unknown";
    time(&tmp_client->ban_start_time);
    tmp_client->ban_end_time = tmp_client->ban_start_time + static_cast<u32>(end_time_sec);
    if (admin)
    {
        tmp_client->admin_hexstr_digest = admin->m_cdkey_digest;
        tmp_client->admin_ip_addr = admin->m_cAddress;
        if (admin->ps)
            tmp_client->admin_name = admin->ps->getName();
        else
            tmp_client->admin_name = "unknown";
    }
    else
    {
        tmp_client->admin_hexstr_digest = "";
        tmp_client->admin_ip_addr.set("0.0.0.0");
        tmp_client->admin_name = "Server";
    }
    m_ban_list.push_back(tmp_client);
    save();
}

void cdkey_ban_list::unban_player_by_index(size_t const index)
{
    if (m_ban_list.size() <= index)
    {
        Msg("! ERROR: bad player index");
        return;
    }
    xr_delete(m_ban_list[index]);
    m_ban_list.erase(m_ban_list.begin() + index);
    save();
}

char const* print_time(time_t const& src_time, string64& dest_time)
{
    tm* tmp_tm = localtime(&src_time);
    xr_sprintf(dest_time, sizeof(dest_time), "%02d.%02d.%d_%02d:%02d:%02d", tmp_tm->tm_mday, tmp_tm->tm_mon + 1,
        tmp_tm->tm_year + 1900, tmp_tm->tm_hour, tmp_tm->tm_min, tmp_tm->tm_sec);
    return dest_time;
}

void cdkey_ban_list::print_ban_list(char const* filter_string)
{
    char const* to_filter = NULL;
    if (filter_string && xr_strlen(filter_string))
        to_filter = filter_string;
    Msg("- ----banned players list begin-------");
    string512 tmp_string;
    u32 index = 0;
    for (ban_list_t::iterator i = m_ban_list.begin(), ie = m_ban_list.end(); i != ie; ++i)
    {
        string64 temp_time;
        xr_sprintf(tmp_string, "- (player index : %d), (ip : %s), (name : %s), (end time : %s), (hex digest : %s);",
            index, (*i)->client_ip_addr.to_string().c_str(), (*i)->client_name.c_str(),
            print_time((*i)->ban_end_time, temp_time), (*i)->client_hexstr_digest.c_str());
        if (filter_string)
        {
            if (strstr(tmp_string, filter_string))
                Msg(tmp_string);
        }
        else
        {
            Msg(tmp_string);
        }
        ++index;
    }
    Msg("- ----banned players list end-------");
}

cdkey_ban_list::banned_client::banned_client()
{
    ban_start_time = 0;
    ban_end_time = 0;
}

time_t get_time_from_string(LPCSTR str_time)
{
    tm tmp_time;
    int res_t = sscanf(str_time, "%02d.%02d.%d_%02d:%02d:%02d", &tmp_time.tm_mday, &tmp_time.tm_mon, &tmp_time.tm_year,
        &tmp_time.tm_hour, &tmp_time.tm_min, &tmp_time.tm_sec);
    if (res_t != 6)
        return 0;

    tmp_time.tm_mon -= 1;
    tmp_time.tm_year -= 1900;
    return mktime(&tmp_time);
};

#define CLIENT_HEX_DIGEST_KEY "client_hexstr_digest"
#define CLIENT_BAN_START_TIME_KEY "ban_start_time"
#define CLIENT_BAN_END_TIME_KEY "ban_end_time"
#define CLIENT_NAME_KEY "client_name"
#define CLIENT_IP_KEY "client_ip"
#define ADMIN_NAME_KEY "admin_name"
#define ADMIN_IP_KEY "admin_ip_addr"
#define ADMIN_HEX_DIGEST_KEY "admin_hexstr_digest"

bool cdkey_ban_list::banned_client::load(CInifile* ini, shared_str const& name_sect)
{
    if (!ini->line_exist(name_sect, CLIENT_HEX_DIGEST_KEY) || !ini->line_exist(name_sect, CLIENT_BAN_END_TIME_KEY))
    {
        return false;
    }
    client_hexstr_digest = ini->r_string(name_sect, CLIENT_HEX_DIGEST_KEY);
    LPCSTR tmp_string = ini->r_string(name_sect, CLIENT_BAN_END_TIME_KEY);

    ban_end_time = get_time_from_string(tmp_string);
    if (ban_end_time == 0)
    {
        Msg("! ERROR bad ban_end_time in section [%s]", name_sect.c_str());
        return false;
    }
    tmp_string = ini->r_string(name_sect, CLIENT_BAN_START_TIME_KEY);
    if (tmp_string)
        ban_start_time = get_time_from_string(tmp_string);

    tmp_string = ini->r_string(name_sect, CLIENT_NAME_KEY);
    if (tmp_string)
        client_name = tmp_string;

    tmp_string = ini->r_string(name_sect, CLIENT_IP_KEY);
    if (tmp_string)
        client_ip_addr.set(tmp_string);

    tmp_string = ini->r_string(name_sect, ADMIN_NAME_KEY);
    if (tmp_string)
        admin_name = tmp_string;
    tmp_string = ini->r_string(name_sect, ADMIN_IP_KEY);
    if (tmp_string)
        admin_ip_addr.set(tmp_string);
    tmp_string = ini->r_string(name_sect, ADMIN_HEX_DIGEST_KEY);
    if (tmp_string)
        admin_hexstr_digest = tmp_string;
    return true;
}

void cdkey_ban_list::banned_client::save(CInifile* ini, char const* name_sect)
{
    ini->w_string(name_sect, CLIENT_HEX_DIGEST_KEY, client_hexstr_digest.c_str());
    string64 stor_string;
    ini->w_string(name_sect, CLIENT_BAN_START_TIME_KEY, print_time(ban_start_time, stor_string));
    ini->w_string(name_sect, CLIENT_BAN_END_TIME_KEY, print_time(ban_end_time, stor_string));
    ini->w_string(name_sect, CLIENT_NAME_KEY, client_name.c_str());
    ini->w_string(name_sect, CLIENT_IP_KEY, client_ip_addr.to_string().c_str());
    ini->w_string(name_sect, ADMIN_NAME_KEY, admin_name.c_str());
    ini->w_string(name_sect, ADMIN_IP_KEY, admin_ip_addr.to_string().c_str());
    ini->w_string(name_sect, ADMIN_HEX_DIGEST_KEY, admin_hexstr_digest.c_str());
}

void cdkey_ban_list::erase_expired_ban_items()
{
    struct expire_searcher_predicate
    {
        time_t current_time;
        bool operator()(banned_client* bclient)
        {
            if (bclient->ban_end_time < current_time)
            {
                Msg("- Ban of %s is expired", bclient->client_name.c_str());
                xr_delete(bclient);
                return true;
            }
            return false;
        }
    };
    expire_searcher_predicate tmp_predicate;
    time(&tmp_predicate.current_time);
    ban_list_t::iterator new_end_iter = std::remove_if(m_ban_list.begin(), m_ban_list.end(), tmp_predicate);
    m_ban_list.erase(new_end_iter, m_ban_list.end());
}
