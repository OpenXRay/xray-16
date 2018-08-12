#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterFindEnemyAngry : public CState<_Object>
{
    typedef CState<_Object> inherited;

public:
    CStateMonsterFindEnemyAngry(_Object* obj);
    virtual ~CStateMonsterFindEnemyAngry();

    virtual void execute();
    virtual bool check_completion();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_find_enemy_angry_inline.h"
