#pragma once

#include "ai/monsters/state.h"
#include "EntityCondition.h"
#include "ai/monsters/states/state_data.h"
template <typename _Object>
class CStateGroupRest : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

    u32 time_for_life;
    u32 time_for_sleep;

public:
    CStateGroupRest(_Object* obj);
    virtual ~CStateGroupRest();

    virtual void initialize();
    virtual void execute();
    virtual void finalize();
    virtual void critical_finalize();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "group_state_rest_inline.h"
