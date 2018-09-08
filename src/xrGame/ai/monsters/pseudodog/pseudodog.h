#pragma once

#include "ai/monsters/basemonster/base_monster.h"

class CAI_PseudoDog : public CBaseMonster
{
    typedef CBaseMonster inherited;

public:
    float m_anger_hunger_threshold;
    float m_anger_loud_threshold;

    TTime m_time_became_angry;

    TTime time_growling; // время нахождения в состоянии пугания

    enum
    {
        eAdditionalSounds = MonsterSound::eMonsterSoundCustom,
        ePsyAttack = eAdditionalSounds | 0,
    };

public:
    CAI_PseudoDog();
    virtual ~CAI_PseudoDog();

    virtual IFactoryObject* _construct();

    virtual void Load(LPCSTR section);

    virtual void reinit();
    virtual void reload(LPCSTR section);

    virtual bool ability_can_drag() { return true; }
    virtual bool ability_psi_attack() { return true; }
    virtual void CheckSpecParams(u32 spec_params);
    // virtual void	play_effect_sound	();

    virtual void HitEntityInJump(const CEntity* pEntity);

    virtual IStateManagerBase* create_state_manager();
    pcstr get_monster_class_name() override { return "pseudodog"; }

private:
#ifdef _DEBUG
    virtual void debug_on_key(int key);
#endif
};
