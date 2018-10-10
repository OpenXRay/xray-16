#pragma once
#include "ai/monsters/state.h"

template <typename _Object>
class CStateBloodsuckerVampireExecute : public CState<_Object>
{
    typedef CState<_Object> inherited;

    enum
    {
        eActionPrepare,
        eActionContinue,
        eActionFire,
        eActionWaitTripleEnd,
        eActionCompleted
    } m_action;

    u32 time_vampire_started;

    bool m_effector_activated;

public:
    CStateBloodsuckerVampireExecute(_Object* obj) : inherited(obj) {}
    virtual void initialize();
    virtual void execute();
    virtual void finalize();
    virtual void critical_finalize();
    virtual bool check_start_conditions();
    virtual bool check_completion();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
private:
    void execute_vampire_prepare();
    void execute_vampire_continue();
    void execute_vampire_hit();

    void look_head();
    void show_hud();
    void cleanup();
};

#include "bloodsucker_vampire_execute_inline.h"
