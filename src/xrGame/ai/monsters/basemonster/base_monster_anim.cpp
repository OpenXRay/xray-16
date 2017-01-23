////////////////////////////////////////////////////////////////////////////
//	Module 		: base_monster_anim.cpp
//	Created 	: 22.05.2003
//  Modified 	: 23.09.2003
//	Author		: Serge Zhem
//	Description : Animations for monsters of biting class
////////////////////////////////////////////////////////////////////////////

#include "Include/xrRender/KinematicsAnimated.h"
#include "ai/Monsters/control_animation_base.h"
#include "ai_monster_space.h"
#include "base_monster.h"
#include "sound_player.h"
#include "stdafx.h"

// Установка анимации
void CBaseMonster::SelectAnimation(const Fvector& /**_view/**/, const Fvector& /**_move/**/, float /**speed/**/)
{
    control().animation().update_frame();
}
