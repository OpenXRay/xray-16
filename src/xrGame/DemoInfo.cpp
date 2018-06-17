#include "stdafx.h"
#include "Level.h"
#include "DemoInfo.h"
#include "xrCore/stream_reader.h"
#include "Common/object_broker.h"
#include "xrScriptEngine/ScriptExporter.hpp"

LPCSTR GameTypeToString(EGameIDs gt, bool bShort);

u32 const demo_player_info::demo_info_max_size = DEMOSTRING_MAX_SIZE + 80;
/*
void stream_read_demostring(CStreamReader* stream, shared_str & dest_result, u32 max_size)
{
    R_ASSERT(stream);
    R_ASSERT(max_size > 0);
    char* dest_str		= static_cast<char*>(_alloca(max_size));
    u32 string_size = stream->r_u32();
    if (string_size > max_size)
    {
        int old_pos = stream->tell();
        stream->r(static_cast<void*>(dest_str), max_size);
        dest_str[max_size - 1] = 0;
        stream->seek(old_pos + string_size);
    } else
    {
        stream->r(static_cast<void*>(dest_str), string_size);
    }
    dest_result = dest_str;
}

void stream_write_demostring(IWriter* writer, shared_str const & string_to_write, u32 max_size)
{
    R_ASSERT(writer);
    R_ASSERT(string_to_write.size() < max_size);
    writer->w_u32		(string_to_write.size() + 1);//with zero end
    writer->w_stringZ	(string_to_write);
}*/

demo_player_info::demo_player_info() {}
demo_player_info::~demo_player_info() {}
void demo_player_info::read_from_file(CStreamReader* file_to_read)
{
    file_to_read->r_stringZ(m_name);
    m_frags = file_to_read->r_s16();
    m_deaths = file_to_read->r_s16();
    m_artefacts = file_to_read->r_u16();
    m_spots = file_to_read->r_s16();
    m_team = file_to_read->r_u8();
    m_rank = file_to_read->r_u8();
}

void demo_player_info::write_to_file(IWriter* file_to_write) const
{
    file_to_write->w_stringZ(m_name);
    file_to_write->w_s16(m_frags);
    file_to_write->w_s16(m_deaths);
    file_to_write->w_u16(m_artefacts);
    file_to_write->w_s16(m_spots);
    file_to_write->w_u8(m_team);
    file_to_write->w_u8(m_rank);
}

void demo_player_info::load_from_player(game_PlayerState* player_state)
{
	m_name		= player_state->getName();
	m_frags		= player_state->m_iRivalKills;
	m_artefacts	= static_cast<u16>(player_state->af_count);
	m_deaths	= player_state->m_iDeaths;
	m_spots		= m_frags - (player_state->m_iTeamKills * 2) - player_state->m_iSelfKills + (m_artefacts * 3);
	m_rank		= player_state->rank;
	m_team		= 0;
}

u32 const demo_info::max_demo_info_size = (demo_player_info::demo_info_max_size * 32) +	(DEMOSTRING_MAX_SIZE * 5) + sizeof(u32);

demo_info::demo_info() {}
demo_info::~demo_info() { delete_data(m_players); }
void demo_info::read_from_file(CStreamReader* file_to_read)
{
    u32 old_pos = file_to_read->tell();
    file_to_read->r_stringZ(m_map_name);
    file_to_read->r_stringZ(m_map_version);
    file_to_read->r_stringZ(m_game_type);
    file_to_read->r_stringZ(m_game_score);
    file_to_read->r_stringZ(m_author_name);
    R_ASSERT(file_to_read->tell() - old_pos <= (DEMOSTRING_MAX_SIZE * 5));

    m_players_count = file_to_read->r_u32();

	R_ASSERT				(m_players_count < 32);
	
	delete_data				(m_players);

    m_players.reserve(m_players_count);
    for (u32 i = 0; i < m_players_count; ++i)
    {
        demo_player_info* new_player = new demo_player_info();

        new_player->read_from_file(file_to_read);
        m_players.push_back(new_player);
    }
}
void demo_info::write_to_file(IWriter* file_to_write) const
{
    file_to_write->w_stringZ(m_map_name);
    file_to_write->w_stringZ(m_map_version);
    file_to_write->w_stringZ(m_game_type);
    file_to_write->w_stringZ(m_game_score);
    file_to_write->w_stringZ(m_author_name);

    file_to_write->w_u32(m_players.size());
    for (players_coll_t::const_iterator i = m_players.begin(), ie = m_players.end(); i != ie; ++i)
    {
        (*i)->write_to_file(file_to_write);
    }
}
void demo_info::sort_players(sorting_less_comparator sorting_comparator)
{
    std::sort(m_players.begin(), m_players.end(), sorting_comparator);
}

void demo_info::load_from_game()
{
	m_map_name		= Level().name();
	m_map_version	= Level().version();
	m_game_type		= "single";
	m_game_score	= 0;
	m_author_name	= "unknown";
	u32	pcount		= 0;
	delete_data			(m_players);
}

demo_player_info const* demo_info::get_player(u32 player_index) const
{
    R_ASSERT(player_index < m_players.size());
    return m_players[player_index];
}

#pragma optimize("s",on)

