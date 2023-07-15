////////////////////////////////////////////////////////////////////////////
//	Module 		: script_animation_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script animation action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_abstract_action.h"
#include "ai_monster_space.h"

class CScriptAnimationAction : public CScriptAbstractAction
{
public:
    enum EGoalType : u32
    {
        eGoalTypeAnimation = u32(0),
        eGoalTypeMental,
    };

public:
    shared_str m_caAnimationToPlay;
    MonsterSpace::EMentalState m_tMentalState{ MonsterSpace::eMentalStateDanger };
    EGoalType m_tGoalType{ eGoalTypeMental };
    bool m_use_animation_movement_controller{};
    MonsterSpace::EScriptMonsterAnimAction m_tAnimAction{ MonsterSpace::eAA_NoAction };
    int anim_index{};

public:
    IC CScriptAnimationAction() = default;
    IC CScriptAnimationAction(LPCSTR caAnimationToPlay, bool use_animation_movement_controller = false);
    IC CScriptAnimationAction(MonsterSpace::EMentalState tMentalState);
    // -------------------------------------------------------------------------------------------------
    // Monster
    // -------------------------------------------------------------------------------------------------
    IC CScriptAnimationAction(MonsterSpace::EScriptMonsterAnimAction tAnimAction, int index);
    IC void SetAnimation(LPCSTR caAnimationToPlay);
    IC void SetMentalState(MonsterSpace::EMentalState tMentalState);
    IC void initialize();
};

#include "script_animation_action_inline.h"
