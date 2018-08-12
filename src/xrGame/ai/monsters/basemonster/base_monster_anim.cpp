////////////////////////////////////////////////////////////////////////////
//	Module 		: base_monster_anim.cpp
//	Created 	: 22.05.2003
//  Modified 	: 23.09.2003
//	Author		: Serge Zhem
//	Description : Animations for monsters of biting class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "base_monster.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "sound_player.h"
#include "ai_monster_space.h"
#include "ai/monsters/control_animation_base.h"

// Установка анимации
void CBaseMonster::SelectAnimation(const Fvector& /**_view/**/, const Fvector& /**_move/**/, float /**speed/**/)
{
    control().animation().update_frame();
}
