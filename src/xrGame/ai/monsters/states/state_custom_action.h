#pragma once
#include "ai/monsters/state.h"
#include "state_data.h"

template <typename _Object>
class CStateMonsterCustomAction : public CState<_Object>
{
    typedef CState<_Object> inherited;

    SStateDataAction data;

public:
    CStateMonsterCustomAction(_Object* obj);
    virtual ~CStateMonsterCustomAction();

    virtual void execute();
    virtual bool check_completion();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "state_custom_action_inline.h"
