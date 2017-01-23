#ifndef EVENT_CONDITIONS_COLLECTION_INCLUDED
#define EVENT_CONDITIONS_COLLECTION_INCLUDED

#include "state_arguments_functions.h"

namespace award_system
{

class game_state_accumulator;

enum enum_event_operation
{
	//binary predicates
	eo_logical_and		= 0x00,
	eo_logical_or,
	//player predicates
	eo_hit_params,				//count, weapon_id, bone_id, operation for fly_dist, bullet fly_dist argument
	eo_kill_params,				//count, weapon_id, bone_id, operation for fly_dist, bullet fly_dist argument
	eo_accumul_value_params		//param name, operation for argument, argument
}; //enum enum_event_operation


struct event_condition_t;

struct event_argument_type
{
	enum enum_argument_type
	{
		at_u16				= 0x00,
		at_u32,
		at_float,
		at_float_bfunction,
		at_u32_bfunction,
		at_operation,
		at_condition
	};//enum enum_type

	union union_argument_value
	{
		u16						u16_value;
		u32						u32_value;
		float					float_value;
		float_binary_function*	float_function_ptr;
		u32_binary_function*	u32_function_ptr;
		enum_event_operation	operation_value;
		event_condition_t*		cond_ptr_value;
	}; //union union_argument_value

	enum_argument_type		m_argument_type_tag;
	union_argument_value	m_argument_value;
}; //struct event_argument_type
typedef xr_vector<event_argument_type>	arguments_t;

struct event_condition_t
{
	enum_event_operation	m_operation;
	arguments_t				m_arguments;
}; //struct event_condition_t


struct event_root_condition_t
{
	u32					m_delegate_argument;
	u32					m_rise_count;
	u32					m_game_mask;
	event_condition_t*	m_root_condition;
}; //event_root_condition_t

typedef fastdelegate::FastDelegate<void (u32)> event_action_delegate_t;

class event_conditions_collection
{
public:
	explicit		event_conditions_collection		(game_state_accumulator* pstate_accum,
													 event_action_delegate_t ea_delegate);
	virtual			~event_conditions_collection	();
	virtual void	init							() = 0;
			void	check_for_events				();
			void	clear_events					();
protected:

	event_condition_t*			add_condition		(enum_event_operation operation,
													 buffer_vector<event_argument_type> & arguments);
	void						add_event			(event_condition_t*	root_condition,
													 u32 const max_rise_count,
													 u32 const game_id_mask,
													 u32 const delegate_argument);
	//helper functions
	event_condition_t*			add_or_condition			(event_condition_t* left, event_condition_t* right);
	event_condition_t*			add_and_condition			(event_condition_t* left, event_condition_t* right);
	
	event_condition_t*			add_hit_condition_dist		(u32 hit_counts, u16 weapon_id, u16 bone_id, float_binary_function* fbfunc, float distanse);
	event_condition_t*			add_kill_condition_dist		(u32 kill_counts, u16 weapon_id, u16 kill_type, u16 special_kill_type, u32 time_period = u32(-1));

	event_condition_t*			add_accumm_value_condition	(u16 param_id, float_binary_function* fbfunc, float argument);
	event_condition_t*			add_accumm_value_condition	(u16 param_id, u32_binary_function* fbfunc, u32 argument);

private:
	typedef xr_vector<event_condition_t*>		event_conditions_t;
	typedef xr_vector<event_root_condition_t>	event_root_conditions_t;

	game_state_accumulator*		m_player_state_accum;
	event_action_delegate_t		m_event_action;

	event_conditions_t			m_all_conditions;
	event_root_conditions_t		m_root_conditions;

	// operation implementations 
	bool	logical_and			(arguments_t & arguments);
	bool	logical_or			(arguments_t & arguments);
	
	bool	hit_params			(arguments_t & arguments);
	bool	kill_params			(arguments_t & arguments);
	bool	accumul_params		(arguments_t & arguments);

	bool	execute_condition		(event_condition_t* cond);
	void	execute_root_condtiion	(event_root_conditions_t::value_type & rcond);
}; //class event_conditions_collection

} //namespace award_system

#endif //#ifndef EVENT_CONDITIONS_COLLECTION_INCLUDED
