#pragma once

template <typename _Object>
class CStateMonsterPanicRun : public CState<_Object>
{
    static constexpr u32 MIN_UNSEEN_TIME = 15000;
    static constexpr float MIN_DIST_TO_ENEMY = 15.f;

    typedef CState<_Object> inherited;

public:
    CStateMonsterPanicRun(_Object* obj) : inherited(obj) {}
    virtual ~CStateMonsterPanicRun() {}
    virtual void initialize();
    virtual void execute();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    virtual bool check_completion();
};

#include "monster_state_panic_run_inline.h"
