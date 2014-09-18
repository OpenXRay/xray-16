////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_planner_target_selector.h
//	Created 	: 18.09.2007
//	Author		: Alexander Dudin
//	Description : Target selector for smart covers animation planner
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_PLANNER_TARGET_SELECTOR_H_INCLUDED
#define SMART_COVER_PLANNER_TARGET_SELECTOR_H_INCLUDED

#include <boost/noncopyable.hpp>
#include "smart_cover_detail.h"
#include "action_planner_action.h"
#include "script_callback_ex.h"
#include "debug_make_final.hpp"

namespace smart_cover {

class animation_planner;

class target_selector : 
	public CActionPlannerAction<animation_planner>,
	private boost::noncopyable,
	private debug::make_final<target_selector>
{
private:
	typedef CActionPlannerAction<animation_planner> inherited;
	
public:
	typedef CScriptCallbackEx<void>					callback_type;

private:
	callback_type	m_script_callback;
	CRandom			m_random;

private:
			void	add_evaluators	();
			void	add_actions		();

public:
	virtual	void	setup			(animation_planner *object, CPropertyStorage *storage);
	virtual void	update			();
	virtual LPCSTR	object_name		() const;
			void	callback		(callback_type const &callback);
	IC callback_type const& callback() const;
};

} //namespace smart_cover

#include "smart_cover_planner_target_selector_inline.h"

#endif // SMART_COVER_PLANNER_TARGET_SELECTOR_H_INCLUDED