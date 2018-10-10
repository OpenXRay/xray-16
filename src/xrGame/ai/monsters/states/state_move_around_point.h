#pragma once

#include "ai/monsters/state.h"
#include "state_data.h"

template <typename _Object>
class CStateMonsterMoveAroundPoint : public CState<_Object>
{
    typedef CState<_Object> inherited;

    SStateDataMoveAroundPoint data;

public:
    CStateMonsterMoveAroundPoint(_Object* obj) : inherited(obj, &data) {}
    virtual ~CStateMonsterMoveAroundPoint() {}
    virtual void initialize();
    virtual void execute();

    virtual bool check_completion();
};

#include "state_move_to_point_inline.h"
