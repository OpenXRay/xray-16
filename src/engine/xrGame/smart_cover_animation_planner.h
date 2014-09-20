////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_animation_planner.h
//	Created 	: 04.09.2007
//	Author		: Alexander Dudin
//	Description : Smart cover animation planner class
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_ANIMATION_PLANNER_H_INCLUDED
#define SMART_COVER_ANIMATION_PLANNER_H_INCLUDED

#include <boost/noncopyable.hpp>
#include "smart_cover_detail.h"
#include "action_planner_script.h"
#include "stalker_decision_space.h"
#include "debug_make_final.hpp"

class CAI_Stalker;
struct SHit;
class CPropertyStorage;

namespace smart_cover {

class cover;
class target_selector;

class animation_planner : 
	public CActionPlannerScript<CAI_Stalker>,
	private boost::noncopyable,
	private debug::make_final<animation_planner>
{
private:
	typedef CActionPlannerScript<CAI_Stalker> inherited;

private:
	CWorldState			m_target;
	u32					m_time_object_hit;
	u32					m_loophole_value;
	u32					m_last_transition_time;
	u32					m_default_idle_interval;
	u32					m_default_lookout_interval;					
	float				m_head_speed;
	CRandom				m_random;
	float				m_idle_min_time;
	float				m_idle_max_time;
	float				m_lookout_min_time;
	float				m_lookout_max_time;
	bool				m_stay_idle;
	u32					m_last_idle_time;
	u32					m_last_lookout_time;

private:
			void		add_evaluators				();
			void		add_actions					();
			bool xr_stdcall hit_callback			(SHit const *hit);

public:
						animation_planner			(CAI_Stalker *object, LPCSTR action_name);
	virtual				~animation_planner			();
	virtual	void		setup						(CAI_Stalker *object, CPropertyStorage *storage);
	virtual void		update						();
			void		initialize					();
			void		finalize					();
			void		target						(StalkerDecisionSpace::EWorldProperties const &world_property);
	IC		u32 const	&time_object_hit			() const;
	IC		u32 const	&loophole_value				() const;
	IC		void		decrease_loophole_value		(u32 const &value);
	IC		u32 const	&last_transition_time		() const;
	IC		void		last_transition_time		(u32 const &value);
	IC		CPropertyStorage *property_storage		();
	IC		shared_str	cName						() const;
	IC		u32			default_idle_interval		();
	IC		u32			default_lookout_interval	();
	IC		float const	&idle_min_time				() const;
	IC		void		idle_min_time				(float const &value);
	IC		float const	&idle_max_time				() const;
	IC		void		idle_max_time				(float const &value);
	IC		float const &lookout_min_time			() const;
	IC		void		lookout_min_time			(float const &value);
	IC		float const	&lookout_max_time			() const;
	IC		void		lookout_max_time			(float const &value);
	IC		bool const	&stay_idle					() const;
	IC		void		stay_idle					(bool const &value);
	IC		u32 const	&last_idle_time				() const;
	IC		void		last_idle_time				(u32 const &value);
	IC		u32 const	&last_lookout_time			() const;
	IC		void		last_lookout_time			(u32 const &value);
	virtual	LPCSTR		object_name					() const;
};

} // namespace smart_cover

#include "smart_cover_animation_planner_inline.h"

#endif // SMART_COVER_ANIMATION_PLANNER_H_INCLUDED
