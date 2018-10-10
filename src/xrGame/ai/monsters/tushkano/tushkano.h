#pragma once
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/monsters/controlled_entity.h"

class CTushkano : public CBaseMonster, public CControlledEntity<CTushkano>
{
    typedef CBaseMonster inherited;
    typedef CControlledEntity<CTushkano> CControlled;

public:
    CTushkano();
    virtual ~CTushkano();

    virtual void Load(LPCSTR section);
    virtual void CheckSpecParams(u32 spec_params);
    pcstr get_monster_class_name() override { return "tushkano"; }
};
