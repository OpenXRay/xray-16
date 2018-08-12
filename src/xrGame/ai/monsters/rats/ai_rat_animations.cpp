////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_rat_animations.cpp
//	Created 	: 21.06.2002
//  Modified 	: 06.11.2002
//	Author		: Dmitriy Iassenev
//	Description : Animations, Bone transformations and Sounds for monster "Rat"
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ai/monsters/rats/ai_rat.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "ai_debug.h"
#include "movement_manager.h"

#define MIN_TURN_ANGLE PI_DIV_6 * .5f

// animations
void CAI_Rat::load_animations()
{
    IKinematicsAnimated* tpVisualObject = smart_cast<IKinematicsAnimated*>(Visual());

    // loading normal animations
    m_tRatAnimations.tNormal.tGlobal.tpaDeath[0] = tpVisualObject->ID_Cycle("norm_death");
    m_tRatAnimations.tNormal.tGlobal.tpaDeath[1] = tpVisualObject->ID_Cycle("norm_death_2");

    m_tRatAnimations.tNormal.tGlobal.tpaAttack[0] = tpVisualObject->ID_Cycle("attack_1");
    m_tRatAnimations.tNormal.tGlobal.tpaAttack[1] = tpVisualObject->ID_Cycle("attack_2");
    m_tRatAnimations.tNormal.tGlobal.tpaAttack[2] = tpVisualObject->ID_Cycle("attack_3");

    m_tRatAnimations.tNormal.tGlobal.tpaIdle[0] = tpVisualObject->ID_Cycle("norm_idle_1");
    m_tRatAnimations.tNormal.tGlobal.tpaIdle[1] = tpVisualObject->ID_Cycle("norm_idle_2");

    m_tRatAnimations.tNormal.tGlobal.tpTurnLeft = tpVisualObject->ID_Cycle("norm_turn_ls");
    m_tRatAnimations.tNormal.tGlobal.tpTurnRight = tpVisualObject->ID_Cycle("norm_turn_rs");

    m_tRatAnimations.tNormal.tGlobal.tWalk.Create(tpVisualObject, "norm_walk");

    m_tRatAnimations.tNormal.tGlobal.tRun.Create(tpVisualObject, "norm_run");
    m_tRatAnimations.tNormal.tGlobal.tRunAttack = tpVisualObject->ID_Cycle("norm_run_fwd_1");

    tpVisualObject->PlayCycle(m_tRatAnimations.tNormal.tGlobal.tpaIdle[0]);
}

void CAI_Rat::SelectAnimation(const Fvector& /**_view/**/, const Fvector& /**_move/**/, float /**speed/**/)
{
    IKinematicsAnimated* tpVisualObject = smart_cast<IKinematicsAnimated*>(Visual());
    MotionID tpGlobalAnimation;

    if (!g_Alive())
    {
        for (int i = 0; i < 2; ++i)
        {
            if (m_tRatAnimations.tNormal.tGlobal.tpaDeath[i] == m_tpCurrentGlobalAnimation)
            {
                tpGlobalAnimation = m_tpCurrentGlobalAnimation;
                break;
            }
        }
        if (!tpGlobalAnimation)
        {
            if (m_tpCurrentGlobalAnimation == m_tRatAnimations.tNormal.tGlobal.tpaIdle[1])
                tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tpaDeath[0];
            else
                tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tpaDeath[::Random.randI(0, 2)];
        }
    }
    else
    {
        if (m_bFiring)
            tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tpaAttack[2];
        else if (angle_difference(movement().m_body.target.yaw, movement().m_body.current.yaw) <= MIN_TURN_ANGLE)
            if (m_fSpeed < 0.2f)
            {
                if (m_bStanding)
                    tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tpaIdle[1];
                else
                    tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tpaIdle[0];
            }
            else if (_abs(m_fSpeed - m_fAttackSpeed) < EPS_L)
                tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tRunAttack;
            else if (_abs(m_fSpeed - m_fMaxSpeed) < EPS_L)
                tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tRun.fwd;
            else
                tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tWalk.fwd;
        else
        {
            if (left_angle(-movement().m_body.target.yaw, -movement().m_body.current.yaw))
                //					tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tpaIdle[0];
                tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tpTurnLeft;
            else
                //					tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tpaIdle[0];
                tpGlobalAnimation = m_tRatAnimations.tNormal.tGlobal.tpTurnRight;
        }
    }

    if (tpGlobalAnimation != m_tpCurrentGlobalAnimation)
        m_tpCurrentGlobalBlend = tpVisualObject->PlayCycle(m_tpCurrentGlobalAnimation = tpGlobalAnimation);

#ifdef DEBUG
    if (psAI_Flags.is(aiAnimation))
    {
        IKinematicsAnimated* skeleton_animated = smart_cast<IKinematicsAnimated*>(Visual());
        Msg("%6d %s animation : %s (%f,%f)", Device.dwTimeGlobal, "Global",
            skeleton_animated->LL_MotionDefName_dbg(m_tpCurrentGlobalAnimation), movement().m_body.current.yaw,
            movement().m_body.target.yaw);
    }
#endif
}
