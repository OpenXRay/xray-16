////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_evaluators.h
//	Created 	: 05.11.2007
//	Author		: Alexander Dudin
//	Description : Smart cover evaluators classes
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_EVALUATORS_H_INCLUDED
#define SMART_COVER_EVALUATORS_H_INCLUDED

#include "property_evaluator.h"
#include "wrapper_abstract.h"

class CAI_Stalker;

typedef CWrapperAbstract2<CAI_Stalker,CPropertyEvaluator>		CStalkerPropertyEvaluator;

namespace smart_cover {

class animation_planner;
class target_selector;

namespace evaluators {

//////////////////////////////////////////////////////////////////////////
// in_cover_evaluator
//////////////////////////////////////////////////////////////////////////

class in_cover_evaluator : public CStalkerPropertyEvaluator {
private:
	typedef CStalkerPropertyEvaluator inherited;

public:
						in_cover_evaluator							(CAI_Stalker *object, LPCSTR evaluator_name);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// cover_actual_evaluator
//////////////////////////////////////////////////////////////////////////

class cover_actual_evaluator : public CStalkerPropertyEvaluator {
private:
	typedef CStalkerPropertyEvaluator inherited;

public:
						cover_actual_evaluator						(CAI_Stalker *object, LPCSTR evaluator_name);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// cover_entered_evaluator
//////////////////////////////////////////////////////////////////////////

class cover_entered_evaluator : public CStalkerPropertyEvaluator {
private:
	typedef CStalkerPropertyEvaluator inherited;

public:
						cover_entered_evaluator						(CAI_Stalker *object, LPCSTR evaluator_name);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// loophole_actual_evaluator
//////////////////////////////////////////////////////////////////////////

class	loophole_actual_evaluator : public CStalkerPropertyEvaluator {
private:
	typedef CStalkerPropertyEvaluator inherited;

private:
			u32			m_loophole_value;
			animation_planner *m_planner;

public:
						loophole_actual_evaluator					(CAI_Stalker *object, LPCSTR evaluator_name, animation_planner *planner, u32 const &loophole_value);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// loophole_hit_long_ago_evaluator
//////////////////////////////////////////////////////////////////////////

class loophole_hit_long_ago_evaluator : public CPropertyEvaluator<animation_planner> {
private:
	typedef CPropertyEvaluator<animation_planner> inherited;

private:
			u32			m_time_to_wait;
public:
						loophole_hit_long_ago_evaluator				(animation_planner *object, LPCSTR evaluator_name, u32 const &time_to_wait);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// loophole_planner_const_evaluator
//////////////////////////////////////////////////////////////////////////

class loophole_planner_const_evaluator : public CPropertyEvaluator<animation_planner> {
private:
	typedef CPropertyEvaluator<animation_planner> inherited;

private:
			bool		m_value;

public:
						loophole_planner_const_evaluator			(animation_planner *object, LPCSTR evaluator_name, bool const &value);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// is_action_available_evaluator
//////////////////////////////////////////////////////////////////////////

class is_action_available_evaluator : public CPropertyEvaluator<animation_planner> {
private:
	typedef CPropertyEvaluator<animation_planner> inherited;

private:
			shared_str	m_action_id;

public:
						is_action_available_evaluator				(animation_planner *object, LPCSTR evaluator_name, LPCSTR action_id);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// loophole_exitable_evaluator
//////////////////////////////////////////////////////////////////////////

class loophole_exitable_evaluator : public CStalkerPropertyEvaluator {
private:
	typedef CStalkerPropertyEvaluator inherited;

public:
						loophole_exitable_evaluator					(CAI_Stalker *object, LPCSTR evaluator_name);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// can_exit_loophole_with_animation
//////////////////////////////////////////////////////////////////////////

class can_exit_loophole_with_animation : public CStalkerPropertyEvaluator {
private:
	typedef CStalkerPropertyEvaluator inherited;

public:
						can_exit_loophole_with_animation			(CAI_Stalker *object, LPCSTR evaluator_name);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// default_behaviour_evaluator
//////////////////////////////////////////////////////////////////////////

class default_behaviour_evaluator : public CPropertyEvaluator<animation_planner> {
private:
	typedef CPropertyEvaluator<animation_planner> inherited;

public:
						default_behaviour_evaluator					(animation_planner *object, LPCSTR evaluator_name);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// can_fire_at_enemy_evaluator
//////////////////////////////////////////////////////////////////////////

class can_fire_at_enemy_evaluator : public CPropertyEvaluator<animation_planner> {
private:
	typedef CPropertyEvaluator<animation_planner> inherited;

public:
						can_fire_at_enemy_evaluator					(animation_planner *object, LPCSTR evaluator_name);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// idle_time_interval_passed_evaluator
//////////////////////////////////////////////////////////////////////////

class idle_time_interval_passed_evaluator : public CPropertyEvaluator<animation_planner> {
private:
	typedef CPropertyEvaluator<animation_planner> inherited;

private:
	u32					m_time_interval;

public:
						idle_time_interval_passed_evaluator			(animation_planner *object, LPCSTR evaluator_name, u32 const &time_interval);
	virtual	_value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// lookout_time_interval_passed_evaluator
//////////////////////////////////////////////////////////////////////////

class lookout_time_interval_passed_evaluator : public CPropertyEvaluator<animation_planner> {
private:
	typedef CPropertyEvaluator<animation_planner> inherited;

private:
	u32					m_time_interval;

public:
						lookout_time_interval_passed_evaluator		(animation_planner *object, LPCSTR evaluator_name, u32 const &time_interval);
	virtual	_value_type	evaluate									();
};

} // namespace evaluators

} // namespace smart_covers

#endif // SMART_COVER_EVALUATORS_H_INCLUDED