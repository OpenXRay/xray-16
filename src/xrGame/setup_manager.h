////////////////////////////////////////////////////////////////////////////
//	Module 		: setup_manager.h
//	Created 	: 05.04.2004
//  Modified 	: 05.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Setup manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/object_broker.h"

template <typename _action_type, typename _object_type, typename _action_id_type>
class CSetupManager
{
protected:
    typedef std::pair<_action_id_type, _action_type*> setup_pair;
    typedef xr_vector<setup_pair> setup_actions;

private:
    struct setup_pred
    {
        _action_id_type m_action_id;

        IC setup_pred(const _action_id_type& action_id) { m_action_id = action_id; }
        IC bool operator()(const setup_pair& pair) const { return (pair.first == m_action_id); }
    };

protected:
    setup_actions m_actions;
    _object_type* m_object;
    _action_id_type m_current_action_id;
    _action_id_type m_previous_action_id;
    bool m_actuality;

public:
    IC CSetupManager(_object_type* object);
    virtual ~CSetupManager();
    virtual void reinit();
    virtual void update();
    IC void add_action(const _action_id_type& action_id, _action_type* action);
    IC _action_type& action(const _action_id_type& action_id) const;
    IC _action_type& current_action() const;
    IC const _action_id_type& current_action_id() const;
    IC void select_action();
    IC void clear();
    IC _object_type& object() const;
    IC const setup_actions& actions() const;
    IC setup_actions& actions();
};

#include "setup_manager_inline.h"
