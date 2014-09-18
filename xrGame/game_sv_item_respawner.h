#pragma once
#include "game_base.h"
#include "xrServer_Object_Base.h"
#include "associative_vector.h"

class xrServer;

class item_respawn_manager
{
private:
	struct spawn_item
	{
		CSE_Abstract*		item_object;
		u32					respawn_time;
		u16					last_game_id;
		u32					last_spawn_time;

		spawn_item();
		spawn_item(u32 r_time);
		spawn_item(spawn_item const & clone);
	};
	
	struct section_item
	{
		shared_str	section_name;
		u32			respawn_time;
		u8			addons;
		u16			count_of_ammo;
	};

	typedef xr_vector<spawn_item>								respawn_collection;
	typedef respawn_collection::iterator						respawn_iter;

	typedef xr_vector<section_item>							section_items;
	typedef section_items::iterator							section_items_iter;
	typedef associative_vector<shared_str, section_items*>	respawn_sections_map;
	typedef respawn_sections_map::iterator					respawn_section_iter;


	struct search_by_id_predicate : public std::binary_function<spawn_item, u16, bool>
	{
		bool operator()(spawn_item const & left, u16 right) const;
	};
	
	NET_Packet					spawn_packet_store;
	xrServer*					m_server;
	respawn_collection			m_respawns;
	respawn_sections_map		m_respawn_sections_cache;
	
	u16						respawn_item				(CSE_Abstract* item_object);
	bool					parse_string				(char const* str, u32 str_size, section_item & result);
	respawn_section_iter	load_respawn_section		(shared_str const & section_name);
	u32						load_section_items			(CInifile & ini, const char* section_name, section_items* items);
	void					clear_respawn_sections		();

	CSE_Abstract*			make_respawn_entity			(shared_str const & section_name, u8 addons, u16 count_of_ammo);
	
	xr_set<u16>				level_items_respawn;
	void					clear_level_items			();
public:
	item_respawn_manager		();
	~item_respawn_manager		();

	//void load_respawn_items		(shared_str const section);
	//this method will be called only during level initialization.
	void add_new_rpoint			(shared_str profile_sect, RPoint const & point);
	void clear_respawns			();
	//void check_to_spawn			(CSE_Abstract* item);
	void respawn_all_items		();
	void check_to_delete		(u16 item_id);
	void update					(u32 current_time);
	void respawn_level_items	();	//level_re.spawn
};