////////////////////////////////////////////////////////////////////////////
//	Module 		: action_planner.h
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Action planner
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "problem_solver.h"
#include "action_base.h"
#include "property_evaluator.h"
#include "property_storage.h"
#include "script_export_space.h"
#include "ai_debug.h"

class CScriptGameObject;

template <
	typename _object_type,
	bool	 _reverse_search = false,
	typename _world_operator = CActionBase<_object_type>,
	typename _condition_evaluator = CPropertyEvaluator<_object_type>,
	typename _world_operator_ptr = _world_operator*,
	typename _condition_evaluator_ptr = _condition_evaluator*
>
class CActionPlanner : 
	public CProblemSolver<
		GraphEngineSpace::CWorldProperty,
		GraphEngineSpace::CWorldState,
		_world_operator,
		_condition_evaluator,
		u32,
		_reverse_search,
		_world_operator_ptr,
		_condition_evaluator_ptr
	> 
{
public:
	typedef CProblemSolver<
		GraphEngineSpace::CWorldProperty,
		GraphEngineSpace::CWorldState,
		_world_operator,
		_condition_evaluator,
		u32,
		_reverse_search,
		_world_operator_ptr,
		_condition_evaluator_ptr
	>												CProblemSolver;
	typedef CProblemSolver							inherited;
	typedef typename inherited::_edge_type			_action_id_type;
	typedef GraphEngineSpace::CWorldProperty		CWorldProperty;
	typedef GraphEngineSpace::CWorldState			CWorldState;
	typedef _world_operator							_world_operator;

protected:
	bool						m_initialized;
	_action_id_type				m_current_action_id;

#ifdef LOG_ACTION
public:
	bool						m_use_log;
	string64					m_temp_string;

public:
	virtual	void				set_use_log				(bool value);
#endif

public:
	_object_type				*m_object;
	CPropertyStorage			m_storage;
	bool						m_loaded;
	bool						m_solving;

#ifdef LOG_ACTION
public:
	virtual LPCSTR				action2string			(const _action_id_type &action_id);
	virtual LPCSTR				property2string			(const _condition_type &action_id);
	virtual LPCSTR				object_name				() const;
	virtual void				show					(LPCSTR offset = "");
	IC		void				show_current_world_state();
	IC		void				show_target_world_state	();
#endif

public:
								CActionPlanner			();
	virtual						~CActionPlanner			();
	virtual	void				setup					(_object_type *object);
	virtual	void				update					();
	virtual void				finalize				();
	IC		COperator			&action					(const _action_id_type &action_id);
	IC		CConditionEvaluator	&evaluator				(const _condition_type &evaluator_id);
	IC		_action_id_type		current_action_id		() const;
	IC		COperator			&current_action			();
	IC		bool				initialized				() const;
	IC		void				add_condition			(_world_operator *action, _condition_type condition_id, _value_type condition_value);
	IC		void				add_effect				(_world_operator *action, _condition_type condition_id, _value_type condition_value);
	IC		virtual void		add_operator			(const _edge_type &operator_id,	_operator_ptr _operator);
	IC		virtual void		remove_operator			(const _edge_type	&operator_id);
	IC		virtual void		add_evaluator			(const _condition_type &condition_id, _condition_evaluator_ptr evaluator);
	IC		virtual void		remove_evaluator		(const _condition_type &condition_id);
	IC		_object_type		&object					() const;
	virtual	void				save					(NET_Packet &packet);
	virtual	void				load					(IReader &packet);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
typedef CActionPlanner<CScriptGameObject> CScriptActionPlanner;
add_to_type_list(CScriptActionPlanner)
#undef script_type_list
#define script_type_list save_type_list(CScriptActionPlanner)

#include "action_planner_inline.h"