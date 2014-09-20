////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_default_behaviour_planner.hpp
//	Created 	: 15.11.2007
//	Author		: Alexander Dudin
//	Description : Default behaviour planner for target selector
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_DEFAULT_BEHAVIOUR_PLANNER_HPP_INCLUDED
#define SMART_COVER_DEFAULT_BEHAVIOUR_PLANNER_HPP_INCLUDED

#include <boost/noncopyable.hpp>
#include "smart_cover_detail.h"
#include "action_planner_action.h"
#include "debug_make_final.hpp"

namespace smart_cover {

class animation_planner;

class default_behaviour_planner : 
	public CActionPlannerAction<animation_planner>,
	private boost::noncopyable,
	private debug::make_final<default_behaviour_planner>
{
private:
	typedef CActionPlannerAction<animation_planner>	inherited;

private:
				void		add_evaluators				();
				void		add_actions					();

public:
							default_behaviour_planner	(animation_planner *object, LPCSTR action_name);
		virtual	void		setup						(animation_planner *object, CPropertyStorage *storage);
		virtual	void		initialize					();
		virtual void		update						();
		virtual	void		finalize					();
	IC			u32	const	&idle_time					() const;
	IC			void		idle_time					(u32 const &value);
	IC			u32	const	&lookout_time				() const;
	IC			void		lookout_time				(u32 const &value);
		virtual LPCSTR		object_name					() const;

private:
		u32				m_idle_time;
		u32				m_lookout_time;
};

} // namespace smart_cover

#include "smart_cover_default_behaviour_planner_inline.hpp"

#endif // SMART_COVER_DEFAULT_BEHAVIOUR_PLANNER_HPP_INCLUDED