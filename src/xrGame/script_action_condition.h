////////////////////////////////////////////////////////////////////////////
//	Module 		: script_action_condition.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script action condition class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"

class CScriptActionCondition final
{
public:
    enum EActionFlags
    {
        MOVEMENT_FLAG = u32(1 << 0),
        WATCH_FLAG = u32(1 << 1),
        ANIMATION_FLAG = u32(1 << 2),
        SOUND_FLAG = u32(1 << 3),
        PARTICLE_FLAG = u32(1 << 4),
        OBJECT_FLAG = u32(1 << 5),
        TIME_FLAG = u32(1 << 6),
        ACT_FLAG = u32(1 << 7)
    };

public:
    u32 m_dwFlags{};
    ALife::_TIME_ID m_tLifeTime{ ALife::_TIME_ID(-1) };
    ALife::_TIME_ID m_tStartTime{ ALife::_TIME_ID(-1) };

public:
    IC CScriptActionCondition() = default;
    IC CScriptActionCondition(u32 dwFlags, double dTime = -1);
    IC void initialize();
};

#include "script_action_condition_inline.h"
