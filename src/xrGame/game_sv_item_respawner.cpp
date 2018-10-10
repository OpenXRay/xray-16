#include "StdAfx.h"
#include "game_sv_item_respawner.h"
#include "game_sv_base.h"
#include "Level.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrNetServer/NET_Messages.h"
#include <functional>

item_respawn_manager::spawn_item::spawn_item()
{
    item_object = NULL;
    respawn_time = 0;
    last_spawn_time = 0;
    last_game_id = u16(-1);
}
item_respawn_manager::spawn_item::spawn_item(u32 r_time)
{
    item_object = NULL;
    respawn_time = r_time;
    last_game_id = u16(-1);
    last_spawn_time = 0;
}

item_respawn_manager::spawn_item::spawn_item(spawn_item const& clone)
{
    item_object = clone.item_object;
    respawn_time = clone.respawn_time;
    last_game_id = clone.last_game_id;
    last_spawn_time = clone.last_spawn_time;
}

bool item_respawn_manager::search_by_id_predicate::operator()(spawn_item const& left, u16 right) const
{
    if (left.last_game_id == right)
        return true;

    return false;
}

item_respawn_manager::item_respawn_manager()
{
    clear_respawns();
    m_server = Level().Server;
}

void item_respawn_manager::clear_respawns()
{
    respawn_iter ie = m_respawns.end();
    for (respawn_iter i = m_respawns.begin(); i != ie; ++i)
    {
        if (i->item_object)
        {
            F_entity_Destroy(i->item_object);
            i->item_object = NULL;
        }
    }
    m_respawns.clear();
}

item_respawn_manager::~item_respawn_manager()
{
    clear_respawns();
    clear_respawn_sections();
}
/*
void item_respawn_manager::load_respawn_items(shared_str const section)
{
    clear_respawns();
    CInifile*	level_ini_file	= Level().pLevel;
    R_ASSERT2	(level_ini_file, "level ini file not initialized");

    if (!level_ini_file->section_exist(section.c_str()))
        return;

    CInifile::Sect resp_sect	= level_ini_file->r_section(section.c_str());

    typedef CInifile::Items::iterator sect_iter;
    sect_iter ie				= resp_sect.Data.end();
    u32 temp_int;
    for (sect_iter i = resp_sect.Data.begin(); i != ie; ++i)
    {
        sscanf(i->second.c_str(), "%d", &temp_int);
        m_respawns.insert(std::make_pair(i->first, spawn_item(temp_int * 1000)));
    }
}

void item_respawn_manager::check_to_spawn(CSE_Abstract* item)
{
    R_ASSERT(item);
    respawn_iter temp_iter = m_respawns.find(shared_str(item->name_replace()));
    if (temp_iter != m_respawns.end())
    {
        if (!temp_iter->second.item_object)
        {
            NET_Packet clone_store;
            CSE_Abstract* temp_entity = F_entity_Create(item->s_name.c_str());
            item->Spawn_Write(clone_store, false);
            temp_entity->Spawn_Read(clone_store);
            temp_iter->second.item_object = temp_entity;
        }
        temp_iter->second.last_game_id		= item->ID;
        temp_iter->second.last_spawn_time	= 0;
    }
}*/

CSE_Abstract* item_respawn_manager::make_respawn_entity(shared_str const& section_name, u8 addons, u16 count_of_ammo)
{
    R_ASSERT(m_server);
    R_ASSERT(m_server->GetGameState());
    CSE_Abstract* temp_entity = F_entity_Create(section_name.c_str());
    R_ASSERT2(temp_entity, make_string("failed to create entity [%s]", section_name.c_str()).c_str());

    temp_entity->ID = 0xffff; // server must generate ID
    temp_entity->ID_Parent = 0xffff; // no-parent
    temp_entity->ID_Phantom = 0xffff; // no-phantom
    temp_entity->RespawnTime = 0; // no-respawn
    CSE_ALifeItemWeapon* pWeapon = smart_cast<CSE_ALifeItemWeapon*>(temp_entity);

    if (pWeapon)
    {
        pWeapon->a_elapsed = pWeapon->get_ammo_magsize();

        if (count_of_ammo < pWeapon->a_elapsed)
            pWeapon->a_elapsed = count_of_ammo;

        pWeapon->m_addon_flags.assign(addons);
    };
    return temp_entity;
}

bool item_respawn_manager::parse_string(char const* str, u32 str_size, section_item& result)
{
    string256 temp_string;
    int params_count = _GetItemCount(str);

    if (params_count == 0)
        return false;

    result.respawn_time = 0;
    result.addons = 0;
    result.count_of_ammo = 0;

    _GetItem(str, 0, temp_string);
    result.section_name = temp_string;

    if (params_count >= 2)
    {
        _GetItem(str, 1, temp_string);
        result.respawn_time = static_cast<u32>(atoi(temp_string) * 1000);
    }

    if (params_count >= 3)
    {
        _GetItem(str, 2, temp_string);
        result.addons = static_cast<u8>(atoi(temp_string));
    }

    if (params_count >= 4)
    {
        _GetItem(str, 3, temp_string);
        result.count_of_ammo = static_cast<u16>(atoi(temp_string));
    }
    return true;
}

u32 item_respawn_manager::load_section_items(CInifile& ini, const char* section_name, section_items* items)
{
    if (!ini.section_exist(section_name))
    {
#ifndef MASTER_GOLD
        Msg("! ERROR: section %s not exist", section_name);
#endif // #ifndef MASTER_GOLD
        return 0;
    }

    string32 item_name;
    string512 item_value;

    u32 item_number = 0;
    u32 added_items = 0;
    xr_sprintf(item_name, "item%d", item_number);

    while (ini.line_exist(section_name, item_name))
    {
        section_item temp_sect_item;
        xr_strcpy(item_value, ini.r_string(section_name, item_name));
        if (!parse_string(item_value, xr_strlen(item_value), temp_sect_item))
        {
            Msg("! WARNING: failed to parse item [%s] in section [%s]", item_name, section_name);
        }
        else
        {
            items->push_back(temp_sect_item);
            ++added_items;
        }
        ++item_number;
        xr_sprintf(item_name, "item%d", item_number);
    }
    return added_items;
}

item_respawn_manager::respawn_section_iter item_respawn_manager::load_respawn_section(shared_str const& section_name)
{
    string_path fn;
    FS.update_path(fn, "$game_config$", "mp" DELIMITER "respawn_items.ltx");
    CInifile ini(fn);

    u32 sections_count = _GetItemCount(section_name.c_str());
    string256 temp_section_name;

    section_items* tmp_sect_items = new section_items();

    for (u32 is = 0; is < sections_count; ++is)
    {
        _GetItem(section_name.c_str(), is, temp_section_name);
        if (!load_section_items(ini, temp_section_name, tmp_sect_items))
        {
            Msg("! WARNING: section [%s] is empty", temp_section_name);
        }
    }
    std::pair<respawn_section_iter, bool> insert_res =
        m_respawn_sections_cache.insert(std::make_pair(section_name, tmp_sect_items));
    if (insert_res.second)
        return insert_res.first;

    xr_delete(tmp_sect_items);
    return m_respawn_sections_cache.end();
}

void item_respawn_manager::clear_respawn_sections()
{
    respawn_section_iter temp_ie = m_respawn_sections_cache.end();
    for (respawn_section_iter temp_iter = m_respawn_sections_cache.begin(); temp_iter != temp_ie; ++temp_iter)
    {
        R_ASSERT(temp_iter->second);
        xr_delete(temp_iter->second);
    }
    m_respawn_sections_cache.clear();
}

void item_respawn_manager::add_new_rpoint(shared_str profile_sect, RPoint const& point)
{
    respawn_section_iter tmp_resp_sect = m_respawn_sections_cache.find(profile_sect);

    if (tmp_resp_sect == m_respawn_sections_cache.end())
    {
        tmp_resp_sect = load_respawn_section(profile_sect);
        if (tmp_resp_sect == m_respawn_sections_cache.end())
        {
#ifndef MASTER_GOLD
            Msg("! ERROR: not found section %s in respawn_items.ltx", profile_sect.c_str());
#endif // #ifndef MASTER_GOLD
            return;
        }
    }
    R_ASSERT2(tmp_resp_sect->second,
        make_string("collection of respawn items section (%s) is NULL", profile_sect.c_str()).c_str());

    section_items_iter iter_ie = tmp_resp_sect->second->end();
    for (section_items_iter iter_rsect = tmp_resp_sect->second->begin(); iter_rsect != iter_ie; ++iter_rsect)
    {
        spawn_item new_item(iter_rsect->respawn_time);
        new_item.item_object =
            make_respawn_entity(iter_rsect->section_name, iter_rsect->addons, iter_rsect->count_of_ammo);
        if (new_item.item_object)
        {
            new_item.item_object->o_Position.set(point.P);
            new_item.item_object->o_Angle.set(point.A);
            m_respawns.push_back(new_item);
        }
        else
        {
#ifndef MASTER_GOLD
            Msg("! ERROR: failed to create entity [%s] with addons [%d]", iter_rsect->section_name, iter_rsect->addons);
#endif // #ifndef MASTER_GOLD
        }
    }
}

void item_respawn_manager::check_to_delete(u16 item_id)
{
    respawn_iter temp_iter =
        std::find_if(m_respawns.begin(), m_respawns.end(), std::bind(search_by_id_predicate(), std::placeholders::_1, item_id));

    if (temp_iter != m_respawns.end())
    {
        temp_iter->last_spawn_time = Level().timeServer();
    }

    xr_set<u16>::iterator temp_li_iter = level_items_respawn.find(item_id);
    if (temp_li_iter != level_items_respawn.end())
    {
        level_items_respawn.erase(temp_li_iter);
    }
}

void item_respawn_manager::update(u32 current_time)
{
    respawn_iter ie = m_respawns.end();
    for (respawn_iter i = m_respawns.begin(); i != ie; ++i)
    {
        if (i->last_spawn_time && ((i->last_spawn_time + i->respawn_time) <= current_time))
        {
            R_ASSERT2(i->item_object, "bad respawn item");
            i->last_game_id = respawn_item(i->item_object);
            i->last_spawn_time = 0;
        }
    }
}

void item_respawn_manager::respawn_all_items()
{
    respawn_iter ie = m_respawns.end();
    for (respawn_iter i = m_respawns.begin(); i != ie; ++i)
    {
        i->last_game_id = respawn_item(i->item_object);
        i->last_spawn_time = 0;
    }
}

u16 item_respawn_manager::respawn_item(CSE_Abstract* item_object)
{
    R_ASSERT(item_object);
    spawn_packet_store.write_start();
#ifdef DEBUG
    Msg("--- Respawning item %s - it's time...", item_object->name());
#endif // #ifdef DEBUG
    item_object->Spawn_Write(spawn_packet_store, false);
    u16 skip_header;
    spawn_packet_store.r_begin(skip_header);
    CSE_Abstract* spawned_item = m_server->Process_spawn(spawn_packet_store, m_server->GetServerClient()->ID);
    if (!spawned_item)
        return 0;
    return spawned_item->ID;
}

void item_respawn_manager::clear_level_items()
{
    xr_set<u16>::iterator ie = level_items_respawn.end();
    for (xr_set<u16>::iterator i = level_items_respawn.begin(); i != ie; ++i)
    {
        CSE_Abstract* entity = m_server->ID_to_entity(*i);
        if (!entity)
            continue; // this can be in case ending of a round...
        // VERIFY2(entity, make_string("entity not found [%d]", *i).c_str());
        if (entity->ID_Parent != u16(-1))
            continue;
#ifndef MASTER_GOLD
        Msg("---Destroying level item [%d] before respawn...", *i);
#endif // #ifndef MASTER_GOLD
        m_server->Perform_destroy(entity, net_flags(TRUE, TRUE));
    }
    level_items_respawn.clear();
}

void item_respawn_manager::respawn_level_items()
{
    clear_level_items();
    string_path fn_spawn;
    if (FS.exist(fn_spawn, "$level$", "level_rs.spawn"))
    {
        IReader* SP = FS.r_open(fn_spawn);
        NET_Packet P;
        u32 S_id;
        for (IReader* S = SP->open_chunk_iterator(S_id); S; S = SP->open_chunk_iterator(S_id, S))
        {
            P.B.count = S->length();
            S->r(P.B.data, P.B.count);

            u16 ID;
            P.r_begin(ID);
            R_ASSERT(M_SPAWN == ID);
            ClientID clientID;
            clientID.set(0);

            CSE_Abstract* entity = m_server->Process_spawn(P, clientID);

            if (entity)
                level_items_respawn.insert(entity->ID);
        }
        FS.r_close(SP);
    }
}
