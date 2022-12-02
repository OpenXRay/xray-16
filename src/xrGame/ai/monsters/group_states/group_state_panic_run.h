#pragma once

template <typename _Object>
class CStateGroupPanicRun : public CState<_Object>
{
    static constexpr u32 MIN_UNSEEN_TIME = 15000;
    static constexpr float MIN_DIST_TO_ENEMY = 15.f;

private:
    typedef CState<_Object> inherited;

public:
    CStateGroupPanicRun(_Object* obj) : inherited(obj) {}
    virtual ~CStateGroupPanicRun() {}
    virtual void initialize();
    virtual void execute();

    virtual bool check_completion();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "group_state_panic_run_inline.h"
