#pragma once
#include "ai/monsters/state.h"

template <typename _Object>
class CStateChimeraHuntingMoveToCover : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;

public:
    CStateChimeraHuntingMoveToCover(_Object* obj);

    virtual void initialize();
    virtual void execute();
    virtual bool check_start_conditions();
    virtual bool check_completion();
};

#include "chimera_state_hunting_move_to_cover_inline.h"
