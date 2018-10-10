#pragma once

#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/ai_monster_bones.h"
#include "ai_monster_space.h"

class CController;

class CControllerDirection : public CControlDirectionBase
{
    typedef CControlDirectionBase inherited;

    CController* m_controller;

    bonesManipulation m_bones;
    CBoneInstance* m_bone_spine;
    CBoneInstance* m_bone_head;

    MonsterSpace::SBoneRotation m_head_orient;

    Fvector m_head_look_point;

public:
    virtual void reinit();
    virtual void update_schedule();

    void head_look_point(const Fvector& look_point);
    Fvector& get_head_look_point() { return m_head_look_point; }
    const MonsterSpace::SBoneRotation& get_head_orientation() const { return m_head_orient; }
private:
    static void bone_callback(CBoneInstance* B);

    void assign_bones();
    void update_head_orientation();
};
