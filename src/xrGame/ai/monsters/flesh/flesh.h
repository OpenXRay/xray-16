#pragma once
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/monsters/controlled_entity.h"

class CAI_Flesh : public CBaseMonster, public CControlledEntity<CAI_Flesh>
{
    typedef CBaseMonster inherited;
    typedef CControlledEntity<CAI_Flesh> CControlled;

public:
    CAI_Flesh();
    virtual ~CAI_Flesh();

    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);

    virtual void CheckSpecParams(u32 spec_params);

    virtual bool ability_can_drag() { return true; }
    pcstr get_monster_class_name() override { return "flesh"; }

private:
    bool ConeSphereIntersection(
        Fvector ConeVertex, float ConeAngle, Fvector ConeDir, Fvector SphereCenter, float SphereRadius);
};
