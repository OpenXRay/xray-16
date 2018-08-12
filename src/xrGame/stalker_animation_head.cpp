////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_head.cpp
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation manager : head animations
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_animation_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_space.h"
#include "sound_player.h"
#include "stalker_animation_data.h"
#include "UIGameSP.h"
#include "ui/UITalkWnd.h"

void CStalkerAnimationManager::head_play_callback(CBlend* blend)
{
    CAI_Stalker* object = (CAI_Stalker*)blend->CallbackParam;
    VERIFY(object);

    CStalkerAnimationPair& pair = object->animation().head();
    pair.on_animation_end();
}

MotionID CStalkerAnimationManager::assign_head_animation()
{
    const ANIM_VECTOR& animations = m_data_storage->m_head_animations.A;

    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (pGameSP && pGameSP->TalkMenu->IsShown())
    {
        if (pGameSP->TalkMenu->OthersInvOwner() == m_object)
        {
            if (pGameSP->TalkMenu->playing_sound())
                return (animations[1]);
        }
    }

    CSoundPlayer& sound = object().sound();
    if (!sound.active_sound_count(true))
        return (animations[0]);

    if (!sound.active_sound_type((u32)StalkerSpace::eStalkerSoundMaskMovingInDanger))
        return (animations[1]);

    return (animations[0]);
}
