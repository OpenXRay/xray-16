#ifndef DEMOPLAY_CONTROL
#define DEMOPLAY_CONTROL


#include "Message_Filter.h"

class demoplay_control
{
public:
	
	demoplay_control		();
	~demoplay_control		();

	typedef fastdelegate::FastDelegate0<void> user_callback_t;

	enum EAction
	{
		on_round_start			= 0,	//ignore param
		on_kill,						//param contains the name of a killer
		on_die,							//param contains the name of a victim
		on_artefactdelivering,			//param contains the name of a player that delivered the art
		on_artefactcapturing,			//param contains the name of a player that captured the art
		on_artefactloosing				//param contains the name of a player that loosed the art
	};

	void	pause_on		(EAction const action, shared_str const & param);
	void	cancel_pause_on	();
	bool	rewind_until	(EAction const action,
								shared_str const & param,
								user_callback_t ucb = no_user_callback);
	void	stop_rewind		();
	
	static user_callback_t	no_user_callback;
private:
	typedef message_filter::msg_type_subtype_func_t msg_type_subtype_func_t;
	enum ECurrentControlMode
	{
		not_active				= 0,
		rewinding,
		waiting_for_actions
	};
	static const	float		rewind_speed;	//speed for rewinding ...
	
	ECurrentControlMode			m_current_mode;
	EAction						m_current_action;
	float						m_prev_speed;

	
	shared_str					m_action_param_str;
	//ready to use delegates
	msg_type_subtype_func_t		m_onround_start;
	msg_type_subtype_func_t		m_on_kill;
	msg_type_subtype_func_t		m_on_die;
	msg_type_subtype_func_t		m_on_artefactdelivering;
	msg_type_subtype_func_t		m_on_artefactcapturing;
	msg_type_subtype_func_t		m_on_artefactloosing;

	user_callback_t				m_user_callback;

	void					activate_filer		(EAction const action, shared_str const & param);
	void					deactivate_filter	();
	void					process_action		();	//pause game and changes demo play speed if it's need

	void __stdcall			on_round_start_impl					(u32 message, u32 subtype, NET_Packet & packet);
	void __stdcall			on_kill_impl						(u32 message, u32 subtype, NET_Packet & packet);
	void __stdcall			on_die_impl							(u32 message, u32 subtype, NET_Packet & packet);
	void __stdcall			on_artefactdelivering_impl			(u32 message, u32 subtype, NET_Packet & packet);
	void __stdcall			on_artefactcapturing_impl			(u32 message, u32 subtype, NET_Packet & packet);
	void __stdcall			on_artefactloosing_impl				(u32 message, u32 subtype, NET_Packet & packet);
}; //class demoplay_control


#endif //#ifndef DEMOPLAY_CONTROL