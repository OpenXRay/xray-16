#ifndef CHIMERA_ATTACK_STATE_H_INCLUDED
#define CHIMERA_ATTACK_STATE_H_INCLUDED

template<typename Object>
class ChimeraAttackState : public CState<Object> 
{
public:
							ChimeraAttackState				(Object* obj);
	virtual void			initialize						();
	virtual	void			execute							();
	virtual void 			finalize						();
	virtual void 			critical_finalize				();

private:
	virtual bool			check_control_start_conditions	(ControlCom::EControlType type);
	bool					select_target_for_move			();

	Fvector					correct_jump_pos				(Fvector const&		pos);
	enum enum_action		{ action_jump, action_attack };
	bool					select_target_for_jump			(enum_action		action);
	bool					select_target_for_attack_jump	();

	bool					check_if_jump_possible			(Fvector const&		target);
	bool					jump							(Fvector const& 	target, bool attack_jump);

	void					set_turn_animation				();

	float					get_attack_radius				() const;
	float					calculate_min_run_distance		() const;

	typedef CState<Object>	inherited;
	virtual void			remove_links	(CObject* object) { inherited::remove_links(object); }

	CControl_Com*			m_capturer;

	enum enum_run_side		{ run_side_undefined, run_side_left, run_side_right, };
	enum_run_side			m_run_side;
	TTime					m_run_side_select_tick;

	enum enum_state			{ state_undefined, state_rotate, state_prepare_jump };
	enum_state				m_state;
	TTime					m_state_end_tick;

	u32						m_num_attack_jumps;
	u32						m_num_prepare_jumps;

	TTime					m_last_jump_time;
	TTime					m_stealth_end_tick;

	Fvector					m_target;
	u32						m_target_vertex;
	Fvector					m_jump_target;

	bool					m_allow_jump;

	Fvector					m_predicted_target;
	Fvector					m_prepare_target;
	u32						m_prepare_vertex;
	bool					m_attack_jump;

	float					m_min_run_distance;

}; // ChimeraAttackState

#include "chimera_attack_state_inline.h"

#endif // #ifdef CHIMERA_ATTACK_STATE_H_INCLUDED