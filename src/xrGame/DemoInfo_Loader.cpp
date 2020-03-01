#include "StdAfx.h"
#include "DemoInfo_Loader.h"
#include "xrCore/stream_reader.h"
#include "Level.h"
#include "Common/object_broker.h"

demo_info_loader::demo_info_loader() {}
demo_info_loader::~demo_info_loader() { delete_data(m_demo_info_cache); }
static bool sort_team_players_by_spots(demo_player_info const* left, demo_player_info const* right)
{
    u8 left_team = left->get_team();
    u8 right_team = right->get_team();
    if (left_team != right_team)
    {
        return left_team < right_team;
    }
    return left->get_spots() > right->get_spots();
}

demo_info* demo_info_loader::load_demofile(LPCSTR demo_file_name)
{
    CStreamReader* tmp_reader = FS.rs_open("$logs$", demo_file_name);
    if (!tmp_reader)
    {
        Msg("ERROR: failed to open file [%s] ...", demo_file_name);
        return NULL;
    }
    CLevel::DemoHeader tmp_fake_header;
    shared_str tmp_fake_string;
    tmp_reader->r(&tmp_fake_header, sizeof(tmp_fake_header));
    tmp_reader->r_stringZ(tmp_fake_string);

    demo_info* tmp_demoinfo = new demo_info();
    tmp_demoinfo->read_from_file(tmp_reader);
    tmp_demoinfo->sort_players(&sort_team_players_by_spots);
    FS.r_close(tmp_reader);
    return tmp_demoinfo;
}

demo_info const* demo_info_loader::get_demofile_info(LPCSTR demo_file_name)
{
    R_ASSERT(demo_file_name);
    shared_str tmp_fn = demo_file_name;
    demo_info_cache_t::const_iterator tmp_iter = m_demo_info_cache.find(tmp_fn);
    if (tmp_iter == m_demo_info_cache.end())
    {
        demo_info* tmp_demoinfo = load_demofile(demo_file_name);
        R_ASSERT(tmp_demoinfo);
        return m_demo_info_cache.insert(std::make_pair(shared_str(demo_file_name), tmp_demoinfo)).first->second;
    }
    return tmp_iter->second;
}
