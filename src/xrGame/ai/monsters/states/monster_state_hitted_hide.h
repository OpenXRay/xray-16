#pragma once

template <typename _Object>
class CStateMonsterHittedHide : public CState<_Object>
{
    static constexpr float GOOD_DISTANCE_IN_COVER = 15.f;
    static constexpr float MIN_HIDE_TIME = 3.f;

private:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

public:
    CStateMonsterHittedHide(_Object* obj) : inherited(obj) {}
    virtual ~CStateMonsterHittedHide() {}
    virtual void initialize();
    virtual void execute();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    virtual bool check_completion();
    virtual bool check_start_conditions();
};

#include "monster_state_hitted_hide_inline.h"
