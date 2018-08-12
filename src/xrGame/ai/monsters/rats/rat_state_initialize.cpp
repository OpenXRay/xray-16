#include "pch_script.h"
#include "ai/monsters/rats/ai_rat.h"
#include "ai/ai_monsters_misc.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "xrAICore/Navigation/game_graph.h"
#include "ai/monsters/rats/ai_rat_space.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "detail_path_manager.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "item_manager.h"
#include "memory_space.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "movement_manager.h"
#include "sound_player.h"
#include "ai/monsters/rats/ai_rat_impl.h"
#include "ai_space.h"

void CAI_Rat::init_state_under_fire()
{
    if (!switch_if_enemy() && get_if_dw_time() && m_tLastSound.dwTime >= m_dwLastUpdateTime)
    {
        Fvector tTemp;
        tTemp.setHP(-movement().m_body.current.yaw, -movement().m_body.current.pitch);
        tTemp.normalize_safe();
        tTemp.mul(m_fUnderFireDistance);
        m_tSpawnPosition.add(Position(), tTemp);
    }
    m_tGoalDir = m_tSpawnPosition;
}

void CAI_Rat::init_free_recoil()
{
    m_dwLostRecoilTime = Device.dwTimeGlobal;
    m_tRecoilPosition = m_tLastSound.tSavedPosition;
    if (!switch_if_enemy() && !switch_if_time())
    {
        Fvector tTemp;
        tTemp.setHP(-movement().m_body.current.yaw, -movement().m_body.current.pitch);
        tTemp.normalize_safe();
        tTemp.mul(m_fUnderFireDistance);
        m_tSpawnPosition.add(Position(), tTemp);
    }
}

void CAI_Rat::init_free_active()
{
    if (bfCheckIfGoalChanged())
    {
        if (Position().distance_to(m_home_position) > m_fMaxHomeRadius)
            m_fSpeed = m_fSafeSpeed = m_fMaxSpeed;
        vfChooseNewSpeed();
    }
}
