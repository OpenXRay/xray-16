#pragma once
#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterLookActor : public CState<_Object>
{
    typedef CState<_Object> inherited;

public:
    CStateMonsterLookActor(_Object* obj) : inherited(obj) {}
    virtual void execute();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

template <typename _Object>
class CStateMonsterTurnAwayFromActor : public CState<_Object>
{
    typedef CState<_Object> inherited;

public:
    CStateMonsterTurnAwayFromActor(_Object* obj) : inherited(obj) {}
    virtual void execute();
};

template <typename _Object>
class CStateMonstertTestIdle : public CState<_Object>
{
    typedef CState<_Object> inherited;

public:
    CStateMonstertTestIdle(_Object* obj) : inherited(obj) {}
    virtual void execute();
};

#include "state_test_look_actor_inline.h"
