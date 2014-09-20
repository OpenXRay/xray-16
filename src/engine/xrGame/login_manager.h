#ifndef LOGIN_MANAGER
#define LOGIN_MANAGER


#include <boost/noncopyable.hpp>
#include "mixed_delegate.h"
#include "../xrGameSpy/GameSpy/GP/gp.h"
#include "account_manager.h"
#include "script_export_space.h"
#include "login_manager.h"
#include "queued_async_method.h"

class CGameSpy_Full;
class CGameSpy_GP;
class CGameSpy_ATLAS;
class CGameSpy_Patching;

namespace gamespy_gp
{

struct profile
{
	GPProfile			m_profile_id;
	shared_str			m_unique_nick;
	shared_str			m_login_ticket;
	bool				m_online;
		
	GSLoginCertificate	mCertificate;
	GSLoginPrivateData	mPrivateData;
	
	profile			(GPProfile const & pid,
					 char const * unique_nick,
					 char const * login_ticket,
					 bool const online) :
		m_profile_id(pid),
		m_unique_nick(unique_nick),
		m_login_ticket(login_ticket),
		m_online(online)
	{
	}

	char const *		unique_nick		() const { return m_unique_nick.c_str(); };
	bool const			online			() const { return m_online; };
	GPProfile const		profile_id		() const { return m_profile_id; };
	//copy constructor is valid
	DECLARE_SCRIPT_REGISTER_FUNCTION
};//struct profile

//typedef fastdelegate::FastDelegate<void (profile const *, shared_str const &)>	login_operation_cb;
typedef mixed_delegate<void (profile const *, char const *), mdut_login_operation_cb_tag>	login_operation_cb;

class login_manager : private boost::noncopyable
{
public:
	explicit			login_manager		(CGameSpy_Full* fullgs_obj);
						~login_manager		();

	void				login				(char const * email,
											 char const * nick,
											 char const * password,
											 login_operation_cb logincb);
	void				stop_login			();

	void				login_offline		(char const * nick, login_operation_cb logincb);
	void				logout				();

	void				save_email_to_registry		(char const * email);
	char const *		get_email_from_registry		();

	void				save_password_to_registry	(char const * password);
	char const *		get_password_from_registry	();

	void				save_nick_to_registry		(char const * nickname);
	char const *		get_nick_from_registry		();

	void				save_remember_me_to_registry	(bool remember);
	bool				get_remember_me_from_registry	();
	
	typedef char		unique_nick_t[GP_UNIQUENICK_LEN];

	void				set_unique_nick			(char const * new_unick,
												 login_operation_cb logincb);
	void				stop_setting_unique_nick();
	
	profile const *		get_current_profile	() const { return m_current_profile; };
	
	void				delete_profile_obj	();	//deletes m_current_profile and clears m_login_operation_cb
	void				forgot_password		(char const * url);
private:
	typedef parameters_tuple3<shared_str, shared_str, shared_str>	login_params_t;
	void				login_raw				(login_params_t const & login_args,
												 login_operation_cb logincb);
	void				release_login			(profile const *, char const *);
	void				reinit_connection_tasks	();
	queued_async_method<
		login_manager,
		login_params_t,
		login_operation_cb,
		&login_manager::login_raw,
		&login_manager::release_login>				m_login_qam;



	typedef parameters_tuple1<shared_str>		set_unick_params_t;
	void				set_unique_nick_raw		(set_unick_params_t const & new_unick,
												 login_operation_cb logincb);
	void				release_set_unique_nick	(profile const *, char const *) {};
	queued_async_method<
		login_manager, 
		set_unick_params_t,
		login_operation_cb,
		&login_manager::set_unique_nick_raw,
		&login_manager::release_set_unique_nick>	m_unique_nick_qam;

	

	CGameSpy_GP*				m_gamespy_gp;
	CGameSpy_ATLAS*				m_gamespy_atlas;
	CGameSpy_Patching*			m_gamespy_patching;
	profile*					m_current_profile;

	shared_str					m_last_email;
	shared_str					m_last_nick;
	shared_str					m_last_password;
	shared_str					m_last_unick;

	string256					m_reg_email;
	string256					m_reg_password;
	string256					m_reg_nick;

	login_operation_cb			m_login_operation_cb;
		
	void	__stdcall			only_log_login		(profile const * res_profile,
													 char const * description);
	////callbacks
	static void __cdecl			login_cb			(GPConnection * connection,
													 void * arg,
													 void * param);

	static void __cdecl			wslogin_cb			(GHTTPResult httpResult,
													 WSLoginResponse * response,
													 void * userData);
	static void	__cdecl			setunick_cb			(GPConnection * connection,
													 void * arg,
													 void * param);
	DECLARE_SCRIPT_REGISTER_FUNCTION
}; //class login_manager

} //namespace gamespy_gp

typedef gamespy_gp::profile				gamespy_gp_profile;
typedef gamespy_gp::login_operation_cb	gamespy_gp_login_operation_cb;
typedef gamespy_gp::login_manager		gamespy_gp_login_manager;

add_to_type_list(gamespy_gp_profile)
#undef script_type_list
#define script_type_list save_type_list(gamespy_gp_profile)

add_to_type_list(gamespy_gp_login_operation_cb)
#undef script_type_list
#define script_type_list save_type_list(gamespy_gp_login_operation_cb)

add_to_type_list(gamespy_gp_login_manager)
#undef script_type_list
#define script_type_list save_type_list(gamespy_gp_login_manager)



#endif //#ifndef LOGIN_MANAGER