#pragma once
#include "ai/monsters/monster_state_manager.h"

class CTushkano;

class CStateManagerTushkano : public CMonsterStateManager<CTushkano>
{
    typedef CMonsterStateManager<CTushkano> inherited;

public:
    CStateManagerTushkano(CTushkano* obj);
    virtual ~CStateManagerTushkano();

    virtual void execute();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};
