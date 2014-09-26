#ifndef CDKEY_BAN_LIST_H
#define CDKEY_BAN_LIST_H

#include "xrServer.h"

class cdkey_ban_list
{
public:
			cdkey_ban_list		();
			~cdkey_ban_list		();

	void	load				();
	void	save				();

	bool	is_player_banned	(char const * hexstr_digest, shared_str & buy_who);
	void	ban_player			(xrClientData const * player_data, s32 time_in_sec, xrClientData const * admin);
	void	ban_player_ll		(char const * hexstr_digest, s32 time_in_sec, xrClientData const * admin);
	
	void	unban_player_by_index	(size_t const index);
	void	print_ban_list			(char const * filter_string);
private:
	struct banned_client
	{
		shared_str	client_hexstr_digest;
		ip_address	client_ip_addr;
		shared_str	client_name;
		time_t		ban_start_time;
		time_t		ban_end_time;
		
		ip_address	admin_ip_addr;
		shared_str	admin_name;
		shared_str	admin_hexstr_digest; // ;) gg, for bad admins
		
		banned_client	();
		bool load		(CInifile* ini, shared_str const & name_sect);
		void save		(CInifile* ini, char const * name_sect);
	};
	void	erase_expired_ban_items();
	typedef xr_vector<banned_client*>	ban_list_t;
	
	ban_list_t		m_ban_list;
}; //class cdkey_ban_list

#endif //#ifndef CDKEY_BAN_LIST_H
