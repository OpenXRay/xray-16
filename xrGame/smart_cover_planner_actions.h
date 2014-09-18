////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_planner_actions.h
//	Created 	: 04.09.2007
//	Author		: Alexander Dudin
//	Description : Smart cover planner action classes
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_PLANNER_ACTIONS_H_INCLUDED
#define SMART_COVER_PLANNER_ACTIONS_H_INCLUDED

#include "smart_cover_detail.h"
#include <boost/noncopyable.hpp>
#include "stalker_combat_action_base.h"
#include "debug_make_final.hpp"

class CAI_Stalker;

namespace smart_cover {

class animation_planner;

////////////////////////////////////////////////////////////////////////////
// action_base 
////////////////////////////////////////////////////////////////////////////

class action_base :
	public CStalkerActionCombatBase,
	private boost::noncopyable
{
private:
	typedef CStalkerActionCombatBase inherited;

public:
						action_base					(CAI_Stalker *object, LPCSTR action_name = "");
	virtual void		select_animation			(shared_str &result) = 0;
	virtual	void		on_animation_end			() = 0;
	virtual	void		on_mark						();
	virtual	void		on_no_mark					();
	virtual bool		is_animated_action			();
			void		setup_orientation			();
};

////////////////////////////////////////////////////////////////////////////
// change_loophole
////////////////////////////////////////////////////////////////////////////

class change_loophole :	
	public action_base,
	private debug::make_final<change_loophole>
{
private:
	typedef action_base								inherited;

public:
						change_loophole				(CAI_Stalker *object, LPCSTR action_name);
	virtual void		initialize					();
	virtual	void		execute						();
	virtual void		finalize					();
	virtual void		select_animation			(shared_str &result);
	virtual	void		on_animation_end			();
};

////////////////////////////////////////////////////////////////////////////
// non_animated_change_loophole
////////////////////////////////////////////////////////////////////////////

class non_animated_change_loophole:
	public action_base,
	private debug::make_final<non_animated_change_loophole>
{
private:
	typedef action_base								inherited;

public:
						non_animated_change_loophole(CAI_Stalker *object, LPCSTR action_name);
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
	virtual bool		is_animated_action			();
	virtual void		select_animation			(shared_str &result);
	virtual	void		on_animation_end			();
};

////////////////////////////////////////////////////////////////////////////
// exit
////////////////////////////////////////////////////////////////////////////

class exit :
	public action_base,
	private debug::make_final<exit>
{
private:
	typedef action_base								inherited;

public:
						exit						(CAI_Stalker *object, LPCSTR action_name);
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
	virtual bool		is_animated_action			();
	virtual void		select_animation			(shared_str &result);
	virtual	void		on_animation_end			();
};

} // namespace smart_cover

#include "smart_cover_planner_actions_inline.h"

#endif // SMART_COVER_PLANNER_ACTIONS_H_INCLUDED