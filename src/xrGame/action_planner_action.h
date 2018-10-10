////////////////////////////////////////////////////////////////////////////
//	Module 		: action_planner_action.h
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Action planner action
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_base.h"
#include "action_planner.h"

class CScriptGameObject;

template <typename _object_type>
class CActionPlannerAction : public CActionPlanner<_object_type>, public CActionBase<_object_type>
{
protected:
    using inherited_planner = CActionPlanner<_object_type>;
    using inherited_action = CActionBase<_object_type>;
    using _edge_value_type = typename inherited_action::edge_value_type;
    using _condition_type = typename inherited_action::_condition_type;
    using _value_type = typename inherited_action::_value_type;

public:
    using COperatorCondition = typename inherited_action::COperatorCondition;
    using _world_operator = typename inherited_planner::world_operator;

#ifdef LOG_ACTION
public:
    virtual void set_use_log(bool value);
    virtual void show(LPCSTR offset = "");
#endif

public:
    IC CActionPlannerAction(_object_type* object = 0, LPCSTR action_name = "");
    virtual ~CActionPlannerAction();
    virtual void setup(_object_type* object, CPropertyStorage* storage);
    virtual void initialize();
    virtual void execute();
    virtual void finalize();
    virtual bool completed() const;
    IC void add_condition(_world_operator* action, _condition_type condition_id, _value_type condition_value);
    IC void add_effect(_world_operator* action, _condition_type condition_id, _value_type condition_value);

    virtual void save(NET_Packet& packet)
    {
        inherited_planner::save(packet);
        inherited_action::save(packet);
    }
    virtual void load(IReader& packet)
    {
        inherited_planner::load(packet);
        inherited_action::load(packet);
    }
};
typedef CActionPlannerAction<CScriptGameObject> CScriptActionPlannerAction;

#include "action_planner_action_inline.h"
