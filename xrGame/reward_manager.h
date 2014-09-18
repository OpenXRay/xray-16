#ifndef REWARD_MANAGER_INCLUDED
#define REWARD_MANAGER_INCLUDED

#include "associative_vector.h"

class game_cl_mp;

namespace award_system
{

class reward_manager
{
public:
	explicit	reward_manager		(game_cl_mp* owner);
				~reward_manager		();

	void		add_task			(u32 const award_id);
	void		update_tasks		();
private:
	struct reward_descriptor
	{
		shared_str	m_award_name;
		shared_str	m_texture_name;
		shared_str	m_color_animation;
		u32			m_width;
		u32			m_height;
		ref_sound	m_play_sound;
		u32			m_process_time;
	};//struct reward_descriptor

	typedef		associative_vector<u32, reward_descriptor*>	rewards_map_t;
	typedef		xr_deque<u32>								reward_tasks_queue_t;
	
	u32						m_reward_process_time;
	u32						m_last_reward_time;
	reward_tasks_queue_t	m_to_reward_queue;
	rewards_map_t			m_rewards_map;
	game_cl_mp*				m_owner;
	

	void		load_rewards		();
	void		load_reward_item	(CInifile & reward_config, u32 const index, shared_str const & section);
	void		process_reward		(u32 const award_id);
};//class reward_manager

} //namespace award_system

#endif //#ifndef REWARD_MANAGER_INCLUDED