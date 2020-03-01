#pragma once
#include "ai/monsters/basemonster/base_monster.h"

class CChimera : public CBaseMonster
{
public:
    CChimera();
    virtual ~CChimera();

    virtual void Load(LPCSTR section);
    virtual void reinit();
    virtual void UpdateCL();

    virtual void CheckSpecParams(u32 spec_params);
    virtual void HitEntityInJump(const CEntity* pEntity);
    virtual void jump(Fvector const& position, float factor);

private:
    pcstr get_monster_class_name() override { return "chimera"; }
    virtual EAction CustomVelocityIndex2Action(u32 velocity_index);

    typedef CBaseMonster inherited;

    SVelocityParam m_velocity_rotate;
    SVelocityParam m_velocity_jump_start;

    struct attack_params
    {
        float attack_radius;
        TTime prepare_jump_timeout;
        TTime attack_jump_timeout;
        TTime stealth_timeout;
        float force_attack_distance;
        u32 num_attack_jumps;
        u32 num_prepare_jumps;
    };

    attack_params m_attack_params;

public:
    attack_params const& get_attack_params() const { return m_attack_params; }
};
