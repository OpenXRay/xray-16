#include "stdafx.h"
#include "control_animation_base.h"

void CControlAnimationBase::AddAnim(EMotionAnim ma, LPCSTR tn, int s_id, SVelocityParam *vel, EPState p_s, LPCSTR fx_front, LPCSTR fx_back, LPCSTR fx_left, LPCSTR fx_right)
{
	SAnimItem *new_item		= xr_new<SAnimItem>();

	new_item->target_name	= tn;
	new_item->spec_id		= s_id;
	new_item->velocity		= *vel;
	new_item->pos_state		= p_s;

	new_item->fxs.front		= fx_front;
	new_item->fxs.back		= fx_back;
	new_item->fxs.left		= fx_left;
	new_item->fxs.right		= fx_right;

	new_item->count			= 0;

	m_anim_storage[ma]		= new_item;
}

void CControlAnimationBase::AddAnim(EMotionAnim ma, LPCSTR tn, int s_id, SVelocityParam *vel, EPState p_s)
{
	SAnimItem *new_item		= xr_new<SAnimItem>();

	new_item->target_name	= tn;
	new_item->spec_id		= s_id;
	new_item->velocity		= *vel;
	new_item->pos_state		= p_s;

	new_item->count			= 0;

	m_anim_storage[ma]		= new_item;
}

void CControlAnimationBase::AddTransition(EMotionAnim from, EMotionAnim to, EMotionAnim trans, bool chain, bool skip_aggressive)
{
	STransition new_item;

	new_item.from.state_used	= false;
	new_item.from.anim			= from;

	new_item.target.state_used	= false;
	new_item.target.anim		= to;

	new_item.anim_transition	= trans;
	new_item.chain				= chain;

	new_item.skip_if_aggressive	= skip_aggressive;

	m_tTransitions.push_back(new_item);
}


void CControlAnimationBase::AddTransition(EMotionAnim from, EPState to, EMotionAnim trans, bool chain, bool skip_aggressive)
{
	STransition new_item;

	new_item.from.state_used	= false;
	new_item.from.anim			= from;

	new_item.target.state_used	= true;
	new_item.target.state		= to;

	new_item.anim_transition	= trans;
	new_item.chain				= chain;
	new_item.skip_if_aggressive	= skip_aggressive;

	m_tTransitions.push_back(new_item);
}


void CControlAnimationBase::AddTransition(EPState from, EMotionAnim to, EMotionAnim trans, bool chain, bool skip_aggressive)
{
	STransition new_item;

	new_item.from.state_used	= true;
	new_item.from.state			= from;

	new_item.target.state_used	= false;
	new_item.target.anim		= to;


	new_item.anim_transition	= trans;
	new_item.chain				= chain;
	new_item.skip_if_aggressive	= skip_aggressive;

	m_tTransitions.push_back(new_item);
}


void CControlAnimationBase::AddTransition(EPState from, EPState to, EMotionAnim trans, bool chain, bool skip_aggressive)
{
	STransition new_item;

	new_item.from.state_used	= true;
	new_item.from.state			= from;

	new_item.target.state_used	= true;
	new_item.target.state		= to;

	new_item.anim_transition	= trans;
	new_item.chain				= chain;
	new_item.skip_if_aggressive	= skip_aggressive;

	m_tTransitions.push_back(new_item);
}

void CControlAnimationBase::LinkAction(EAction act, EMotionAnim pmt_motion, EMotionAnim pmt_left, EMotionAnim pmt_right, float pmt_angle)
{

	SMotionItem new_item;

	new_item.anim				= pmt_motion;
	new_item.is_turn_params		= true;
	new_item.turn.anim_left		= pmt_left;
	new_item.turn.anim_right	= pmt_right;
	new_item.turn.min_angle		= pmt_angle;

	m_tMotions.insert	(mk_pair(act, new_item));
}

void CControlAnimationBase::LinkAction(EAction act, EMotionAnim pmt_motion)
{
	SMotionItem new_item;

	new_item.anim				= pmt_motion;
	new_item.is_turn_params		= false;

	m_tMotions.insert	(mk_pair(act, new_item));
}

void CControlAnimationBase::AddReplacedAnim(bool *b_flag, EMotionAnim pmt_cur_anim, EMotionAnim pmt_new_anim)
{
	SReplacedAnim ra;

	ra.flag		= b_flag;
	ra.cur_anim = pmt_cur_anim;
	ra.new_anim = pmt_new_anim;

	m_tReplacedAnims.push_back(ra);
}
