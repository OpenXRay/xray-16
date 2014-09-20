#ifndef DEMOINFO_H
#define DEMOINFO_H

#include <boost/noncopyable.hpp>
#include "script_export_space.h"

class CStreamReader;
class IWriter;
struct game_PlayerState;

#define DEMOSTRING_MAX_SIZE 256

//reads string in demo file format: (u32 - str_size, stringz)
//if the string is longer than max_size, then reads max_size-1 bytes of string, and jumps to the end of string
//void stream_read_demostring	(CStreamReader* stream, shared_str & dest_rest, u32 max_size = STREAM_DEMOSTRING_MAX_SIZE);
//void stream_write_demostring(IWriter* writer, shared_str const & string_to_write, u32 max_size = STREAM_DEMOSTRING_MAX_SIZE);

class demo_player_info : private boost::noncopyable
{
private:
	shared_str	m_name;
	s16			m_frags;
	s16			m_deaths;
	u16			m_artefacts;
	s16			m_spots;
	u8			m_team;
	u8			m_rank;
public:
	demo_player_info();
	~demo_player_info();

	void	read_from_file		(CStreamReader* file_to_read);
	void	write_to_file		(IWriter* file_to_write) const;
	void	load_from_player	(game_PlayerState*	player_state);

	LPCSTR				get_name		() const { return m_name.c_str(); };
	s16	const 			get_frags		() const { return m_frags; };
	s16	const 			get_deaths		() const { return m_deaths; };
	u16	const 			get_artefacts	() const { return m_artefacts; };
	s16	const 			get_spots		() const { return m_spots; };
	u8	const 			get_team		() const { return m_team; };
	u8	const 			get_rank		() const { return m_rank; };

	static	u32	const			demo_info_max_size;
	
	DECLARE_SCRIPT_REGISTER_FUNCTION
}; //class demo_player_info

add_to_type_list(demo_player_info)
#undef script_type_list
#define script_type_list save_type_list(demo_player_info)

class demo_info : private boost::noncopyable
{
private:
	typedef xr_vector<demo_player_info*>	players_coll_t;
	shared_str		m_map_name;
	shared_str		m_map_version;
	shared_str		m_game_type;
	shared_str		m_game_score;
	shared_str		m_author_name;
	u32				m_players_count;
	players_coll_t	m_players;
	
public:
	demo_info	();
	~demo_info	();

	typedef bool (*sorting_less_comparator)(demo_player_info const *, demo_player_info const*);
	
	void	read_from_file	(CStreamReader* file_to_read);
	void	write_to_file	(IWriter* file_to_write) const;
	void	sort_players	(sorting_less_comparator sorting_comparator);
	void	load_from_game	();

	LPCSTR				get_map_name		() const { return m_map_name.c_str(); };
	LPCSTR				get_map_version		() const { return m_map_version.c_str(); };
	LPCSTR				get_game_type		() const { return m_game_type.c_str(); };
	LPCSTR				get_game_score		() const { return m_game_score.c_str(); }; 
	LPCSTR				get_author_name		() const { return m_author_name.c_str(); };
	u32 const 			get_players_count	() const { return m_players.size(); };
	
	demo_player_info const *	get_player			(u32 player_index)				const;

	static	u32 const			max_demo_info_size;
	DECLARE_SCRIPT_REGISTER_FUNCTION
}; //class DemoInfo

add_to_type_list(demo_info)
#undef script_type_list
#define script_type_list save_type_list(demo_info)


#endif //#ifndef DEMOINFO_H