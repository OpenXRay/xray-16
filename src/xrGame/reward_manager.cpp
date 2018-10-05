#include "StdAfx.h"
#include "reward_manager.h"
#include "game_cl_mp.h"
#include "UIGameMP.h"
#include "reward_snd_messages.h"
#include "HUDManager.h"
#include "Common/object_broker.h"

namespace award_system
{
reward_manager::reward_manager(game_cl_mp* owner) : m_reward_process_time(1000), m_last_reward_time(0), m_owner(owner)
{
    load_rewards();
}

reward_manager::~reward_manager() { delete_data(m_rewards_map); }
void reward_manager::add_task(u32 const award_id) { m_to_reward_queue.push_back(award_id); }
void reward_manager::update_tasks()
{
    if (m_to_reward_queue.empty())
        return;

    u32 const tmp_award_id = m_to_reward_queue.front();

    if ((Device.dwTimeGlobal - m_last_reward_time) < m_reward_process_time)
        return;

    process_reward(tmp_award_id);
    m_to_reward_queue.pop_front();
}

void reward_manager::load_rewards()
{
    static char const* section_name_prefix = "reward_";
    string_path reward_config_path;
    FS.update_path(reward_config_path, "$game_config$", "mp" DELIMITER "rewarding.ltx");
    IReader* tmp_reader = FS.r_open(reward_config_path);
    VERIFY2(tmp_reader, "can't open $game_config$" DELIMITER "mp" DELIMITER "rewarding.ltx");
    CInifile rewards_config(tmp_reader);

    u32 reward_index = 0;
    LPCSTR section_name = NULL;
    char tmp_dst_buff[16];

    STRCONCAT(section_name, section_name_prefix, xr_itoa(reward_index, tmp_dst_buff, 10));

    while (rewards_config.section_exist(section_name))
    {
        load_reward_item(rewards_config, reward_index, section_name);
        ++reward_index;
        STRCONCAT(section_name, section_name_prefix, xr_itoa(reward_index, tmp_dst_buff, 10));
    }

    FS.r_close(tmp_reader);
}

void reward_manager::load_reward_item(CInifile& reward_config, u32 const index, shared_str const& section)
{
#ifdef DEBUG
    struct award_name_searcher
    {
        shared_str m_award_name;
        bool operator()(rewards_map_t::value_type const& item) const
        {
            return item.second->m_award_name == m_award_name;
        }
    }; // struct award_name_searcher
#endif
    VERIFY2(m_rewards_map.find(index) == m_rewards_map.end(),
        make_string("reward with id=%d already loaded", index).c_str());

    reward_descriptor* tmp_descriptor = new reward_descriptor();
    tmp_descriptor->m_award_name = reward_config.r_string(section, "name");
    tmp_descriptor->m_texture_name = reward_config.r_string(section, "ingame_texture");
    tmp_descriptor->m_color_animation = reward_config.r_string(section, "color_animation");
    tmp_descriptor->m_width = reward_config.r_u32(section, "ingame_texture_width");
    tmp_descriptor->m_height = reward_config.r_u32(section, "ingame_texture_height");

    LPCSTR tmp_sound_name = reward_config.r_string(section, "play_sound");
    tmp_descriptor->m_play_sound.create(tmp_sound_name, st_Effect, 0);
    tmp_descriptor->m_process_time = reward_config.r_u32(section, "reward_time");

#ifdef DEBUG
    award_name_searcher tmp_award_searcher;
    tmp_award_searcher.m_award_name = tmp_descriptor->m_award_name;
    VERIFY2(std::find_if(m_rewards_map.begin(), m_rewards_map.end(), tmp_award_searcher) == m_rewards_map.end(),
        make_string("reward with award %s already loaded", tmp_award_searcher.m_award_name.c_str()).c_str());
#endif
    m_rewards_map.insert(std::make_pair(index, tmp_descriptor));
}

void reward_manager::process_reward(u32 const award_id)
{
    for (rewards_map_t::iterator i = m_rewards_map.begin(), ie = m_rewards_map.end(); i != ie; ++i)
    {
        if (i->second->m_play_sound._feedback())
        {
            i->second->m_play_sound.stop();
        }
    }
    rewards_map_t::const_iterator tmp_iter = m_rewards_map.find(award_id);
    VERIFY(tmp_iter != m_rewards_map.end());
    UIGameMP* tmp_ui_mp_game = smart_cast<UIGameMP*>(CurrentGameUI());
    R_ASSERT(tmp_ui_mp_game);

    tmp_ui_mp_game->AddAchivment(tmp_iter->second->m_texture_name, tmp_iter->second->m_color_animation,
        tmp_iter->second->m_width, tmp_iter->second->m_height);

    tmp_iter->second->m_play_sound.play(NULL, sm_2D);
    m_reward_process_time = tmp_iter->second->m_process_time;
    m_last_reward_time = Device.dwTimeGlobal;
}

} // namespace award_system
