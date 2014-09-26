#ifndef ACCOUNT_MANAGER
#define ACCOUNT_MANAGER

#include <boost/noncopyable.hpp>
#include "mixed_delegate.h"
#include "GameSpy/GameSpy_FuncDefs.h"
#include "script_export_space.h"
#include "queued_async_method.h"

class CGameSpy_GP;

namespace gamespy_gp
{

struct new_profile_data
{
	shared_str	nick;
	shared_str	unique_nick;
	shared_str	email;
	shared_str	password;
	//shared_str	cd_key;
}; //struct new_account_data

typedef mixed_delegate<
	void (bool, char const *),
	account_operation_cb_tag>			account_operation_cb;

typedef mixed_delegate<
	void (u32 const, char const *),
	account_profiles_cb_tag>			account_profiles_cb;

typedef mixed_delegate<
	void (bool, char const *),
	found_emails_cb_tag>				found_email_cb;

typedef mixed_delegate<
	void (u32 const, char const *),
	suggest_nicks_cb_tag>				suggest_nicks_cb;


class account_manager : private boost::noncopyable
{
public:
	explicit	account_manager		(CGameSpy_GP* gsgp_inst);
				~account_manager	();

	typedef	xr_vector<shared_str>	suggested_nicks_t;
	typedef xr_vector<char const*>	suggested_nicks_ptrs_t;
	void		suggest_unique_nicks			(char const * unick,
												 suggest_nicks_cb sncb);
	bool		is_suggest_unique_nicks_active	() const;
	void		reinit_suggest_unique_nicks		();
	void		stop_suggest_unique_nicks		();

	void		create_profile			(char const * nick,
										 char const * unique_nick,
										 char const * email,
										 char const * password,
										 account_operation_cb opcb);

	void		delete_profile		(account_operation_cb dpcb);
	
	typedef	xr_vector<shared_str>	profiles_store_t;
	typedef xr_vector<char const*>	profiles_nicks_ptrs_t;

	void		get_account_profiles			(char const * email,
												 char const * password,
												 account_profiles_cb profiles_cb);
	bool		is_get_account_profiles_active	() const;
	void		reinit_get_account_profiles		();
	void		stop_fetching_account_profiles	();
	
	void		search_for_email				(char const * email,
												 found_email_cb found_email_cb);
	bool		is_email_searching_active		() const;
	void		reinit_email_searching			();
	void		stop_searching_email			();

	bool		verify_unique_nick		(char const * unick);
	bool		verify_email			(char const * email);
	bool		verify_password			(char const * pass);
	char const*	get_verify_error_descr	() const { return m_verifyer_error.c_str(); }

	profiles_nicks_ptrs_t const &	get_found_profiles	() const { return m_result_profiles_ptrs; };
	suggested_nicks_ptrs_t const &	get_suggested_unicks() const { return m_suggested_nicks_ptrs; };
private:
	CGameSpy_GP*				m_gamespy_gp;
	account_operation_cb		m_account_creation_cb;
	void	__stdcall			only_log_creation_cb(bool success,
													 char const * descr);

	account_operation_cb		m_profile_deleting_cb;
	void	__stdcall			only_log_profdel_cb	(bool success,
													 char const * descr);

	//fetching account profiles
	typedef parameters_tuple2<shared_str, shared_str> get_account_params_t;
	void		get_account_profiles_raw	(get_account_params_t const & args,
											 account_profiles_cb profiles_cb);
	void		release_account_profiles	(u32 const profiles_count, 
											 char const * description);
	queued_async_method<
		account_manager,
		get_account_params_t,
		account_profiles_cb,
		&account_manager::get_account_profiles_raw,
		&account_manager::release_account_profiles>		m_get_account_profiles_qam;

	account_profiles_cb			m_account_profiles_cb;
	profiles_store_t			m_result_profiles;
	profiles_nicks_ptrs_t		m_result_profiles_ptrs;
	void	__stdcall			only_log_profiles	(u32 const profiles_count,
													 char const * description);
	//end of fetching account profiles

	//searching for emails
	typedef		parameters_tuple1<shared_str>	search_for_email_params_t;
	void		search_for_email_raw	(search_for_email_params_t const & email,
										 found_email_cb found_email_cb);
	void		release_found_email		(bool found, char const * user_name);
	queued_async_method<
		account_manager,
		search_for_email_params_t,
		found_email_cb,
		&account_manager::search_for_email_raw,
		&account_manager::release_found_email>	m_search_for_email_qam;
	void		stop_search_for_email	();
	found_email_cb				m_found_email_cb;
	void	__stdcall			only_log_found_email(bool found,
													 char const * user_name);
	//end of searching for emails

	//suggesting unique nicks
	typedef		parameters_tuple1<shared_str>	suggest_uniqie_nicks_params_t;
	void		suggest_unique_nicks_raw		(suggest_uniqie_nicks_params_t const & unick,
												 suggest_nicks_cb sncb);
	void		release_suggest_uniqie_nicks	(u32 const, char const *);
	queued_async_method<
		account_manager,
		suggest_uniqie_nicks_params_t,
		suggest_nicks_cb,
		&account_manager::suggest_unique_nicks_raw,
		&account_manager::release_suggest_uniqie_nicks>	m_suggest_uniqie_nicks_qam;
	//end of suggesting uniqiue nicks

	suggest_nicks_cb			m_suggest_nicks_cb;
	suggested_nicks_t			m_suggested_nicks;
	suggested_nicks_ptrs_t		m_suggested_nicks_ptrs;
	void	__stdcall			only_log_suggestions(u32 const profiles_count,
													 char const * description);




	shared_str					m_verifyer_error;
	bool						verify_nick				(char const * nick);
	
	//callbacks
	static void __cdecl			new_user_cb				(GPConnection * connection,
														 void * arg,
														 void * param);
	static void __cdecl			user_nicks_cb			(GPConnection * connection,
														 void * arg,
														 void * param);
	
	static void __cdecl			unicks_suggestion_cb	(GPConnection * connection,
														 void * arg,
														 void * param);
	static void __cdecl			delete_profile_cb		(GPConnection * connection,
														 void * arg,
														 void * param);
	static void __cdecl			search_profile_cb		(GPConnection * connection,
														 void * arg,
														 void * param);
	/*static void __cdecl			profiles_cb		(GPConnection * connection,
												 void * arg,
												 void * param);*/
	DECLARE_SCRIPT_REGISTER_FUNCTION
}; //class account_manager


} //namespace gamespy_gp

typedef gamespy_gp::account_manager			gamespy_gp_account_manager;
typedef gamespy_gp::suggest_nicks_cb		gamespy_gp_suggest_nicks_cb;
typedef gamespy_gp::account_operation_cb	gamespy_gp_account_operation_cb;
typedef gamespy_gp::account_profiles_cb		gamespy_gp_account_profiles_cb;
typedef gamespy_gp::found_email_cb			gamespy_gp_found_email_cb;

add_to_type_list(gamespy_gp_suggest_nicks_cb)
#undef script_type_list
#define script_type_list save_type_list(gamespy_gp_suggest_nicks_cb)

add_to_type_list(gamespy_gp_account_operation_cb)
#undef script_type_list
#define script_type_list save_type_list(gamespy_gp_account_operation_cb)

add_to_type_list(gamespy_gp_account_profiles_cb)
#undef script_type_list
#define script_type_list save_type_list(gamespy_gp_account_profiles_cb)

add_to_type_list(gamespy_gp_found_email_cb)
#undef script_type_list
#define script_type_list save_type_list(gamespy_gp_found_email_cb)

add_to_type_list(gamespy_gp_account_manager)
#undef script_type_list
#define script_type_list save_type_list(gamespy_gp_account_manager)


#endif //#ifndef ACCOUNT_MANAGER
