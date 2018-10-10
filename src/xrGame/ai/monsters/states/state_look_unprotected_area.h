#pragma once
#include "ai/monsters/state.h"
#include "state_data.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"

template <typename _Object>
class CStateMonsterLookToUnprotectedArea : public CState<_Object>
{
    typedef CState<_Object> inherited;

    SStateDataAction data;

    Fvector target_point;

public:
    CStateMonsterLookToUnprotectedArea(_Object* obj);
    virtual ~CStateMonsterLookToUnprotectedArea();

    virtual void initialize();
    virtual void execute();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    virtual bool check_completion();
};

#include "state_look_unprotected_area_inline.h"
