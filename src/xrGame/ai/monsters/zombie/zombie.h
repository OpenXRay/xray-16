#pragma once
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/monsters/controlled_entity.h"
#include "ai/monsters/ai_monster_bones.h"
#include "ai/monsters/anim_triple.h"

#define FAKE_DEATH_TYPES_COUNT 4

class CZombie : public CBaseMonster, public CControlledEntity<CZombie>
{
    typedef CBaseMonster inherited;
    typedef CControlledEntity<CZombie> CControlled;

    bonesManipulation Bones;

public:
    CZombie();
    virtual ~CZombie();

    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void reinit();
    virtual void reload(LPCSTR section);

    virtual void Hit(SHit* pHDS);

    virtual bool ability_pitch_correction() { return false; }
    virtual void shedule_Update(u32 dt);

    static void BoneCallback(CBoneInstance* B);
    void vfAssignBones();

    virtual bool use_center_to_aim() const { return true; }
    pcstr get_monster_class_name() override { return "zombie"; }
    CBoneInstance* bone_spine;
    CBoneInstance* bone_head;

    SAnimationTripleData anim_triple_death[FAKE_DEATH_TYPES_COUNT];
    u8 active_triple_idx;

    u32 time_dead_start;
    u32 last_hit_frame;
    u32 time_resurrect;

    u8 fake_death_count;
    float health_death_threshold;
    u8 fake_death_left;

    bool fake_death_fall_down(); // return true if everything is ok
    void fake_death_stand_up();

#ifdef _DEBUG
    virtual void debug_on_key(int key);
#endif
};
