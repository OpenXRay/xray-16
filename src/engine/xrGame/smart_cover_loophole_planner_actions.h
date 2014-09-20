////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_loophole_planner_actions.h
//	Created 	: 04.09.2007
//	Author		: Alexander Dudin
//	Description	: Smart cover loophole planner action classes
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_LOOPHOLE_PLANNER_ACTIONS_H_INCLUDED
#define SMART_COVER_LOOPHOLE_PLANNER_ACTIONS_H_INCLUDED

#include "smart_cover_detail.h"
#include "smart_cover_planner_actions.h"
#include <boost/noncopyable.hpp>
#include "xrServer_Space.h"

class CAI_Stalker;
class CPropertyStorage;

namespace StalkerDecisionSpace {
	enum EWorldProperties;
}

namespace smart_cover {

class animation_planner;

class loophole_action_base : public action_base {
private:
	typedef action_base								inherited;

private:
			Fvector		nearest_loophole_direction	(Fvector const& position) const;
			void		process_fire_position		(bool const& change_sight);
			void		process_fire_object			(bool const& change_sight);
			void		process_default				(bool const& change_sight);
			bool		enemy_in_fov				() const;
			bool		process_enemy				(bool const& change_sight);

protected:
			bool		setup_sight					(bool const& change_sight);

public:
						loophole_action_base		(CAI_Stalker *object, LPCSTR action_name);
};

class loophole_action : public loophole_action_base {
private:
	typedef loophole_action_base					inherited;

public:
	typedef xr_vector<shared_str>					Animations;

protected:
	shared_str			m_action_id;
	shared_str			m_animation;
	CRandom				m_random;

public:
						loophole_action				(CAI_Stalker *object, LPCSTR action_name);
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
	virtual void		select_animation			(shared_str &result);
	virtual	void		on_animation_end			();
};

class loophole_action_no_sight : public loophole_action {
private:
	typedef loophole_action							inherited;

public:
						loophole_action_no_sight	(CAI_Stalker *object, LPCSTR action_name);
	virtual void		initialize						();
	virtual void		finalize						();
}; // class loophole_action_no_sight

class loophole_lookout :
	public loophole_action,
	private debug::make_final<loophole_lookout>
{
private:
	typedef loophole_action							inherited;

public:
						loophole_lookout			(CAI_Stalker *object, LPCSTR action_name);
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
};

class loophole_fire :
	public loophole_action,
	private debug::make_final<loophole_fire>
{
private:
	typedef loophole_action							inherited;

private:
	float				m_previous_time;
	bool				m_firing;

public:
						loophole_fire				(CAI_Stalker *object, LPCSTR action_name);
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
	virtual void		select_animation			(shared_str &result);
	virtual	void		on_animation_end			();
	virtual	void		on_mark						();
	virtual	void		on_no_mark					();
};

class loophole_reload :
	public loophole_action_no_sight,
	private debug::make_final<loophole_reload>
{
private:
	typedef loophole_action_no_sight				inherited;

public:
						loophole_reload				(CAI_Stalker *object, LPCSTR action_name);
	virtual void		select_animation			(shared_str &result);
};

class transition : public loophole_action_base {
private:
	typedef loophole_action_base					inherited;

private:
	shared_str			m_action_from;
	shared_str			m_action_to;
	CRandom				m_random;
	StalkerDecisionSpace::EWorldProperties m_state_from;
	StalkerDecisionSpace::EWorldProperties m_state_to;
	animation_planner	*m_planner;

protected:
	shared_str			m_animation;

public:
						transition					(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner);
	virtual void		initialize					();
	virtual void		finalize					();
	virtual void		select_animation			(shared_str &result);
	virtual	void		on_animation_end			();
};

class idle_2_fire_transition : public transition {
private:
	typedef transition	inherited;

public:
						idle_2_fire_transition		(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner, bool const& use_weapon);
	virtual void		initialize					();
	virtual void		finalize					();
}; // class idle_2_fire_transition

class fire_2_idle_transition : public transition {
private:
	typedef transition	inherited;

public:
						fire_2_idle_transition		(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner);
	virtual void		initialize					();
	virtual void		finalize					();
}; // class idle_2_fire_transition

class idle_2_lookout_transition : public transition {
private:
	typedef transition	inherited;

public:
						idle_2_lookout_transition	(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner);
	virtual void		initialize					();
	virtual void		finalize					();
}; // class idle_2_fire_transition

class lookout_2_idle_transition : public transition {
private:
	typedef transition	inherited;

public:
						lookout_2_idle_transition	(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner);
	virtual void		initialize					();
	virtual void		finalize					();
}; // class lookout_2_idle_transition

} // namespace smart_cover

#include "smart_cover_loophole_planner_actions_inline.h"

#endif // SMART_COVER_LOOPHOLE_PLANNER_ACTIONS_H_INCLUDED