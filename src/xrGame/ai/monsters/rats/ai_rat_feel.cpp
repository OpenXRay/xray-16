////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_rat_feel.cpp
//	Created 	: 23.07.2002
//  Modified 	: 07.11.2002
//	Author		: Dmitriy Iassenev
//	Description : Visibility and look for monster "Rat"
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai/monsters/rats/ai_rat.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "xrServerEntities/ai_sounds.h"

bool CAI_Rat::feel_vision_isRelevant(IGameObject* O)
{
    CEntityAlive* E = smart_cast<CEntityAlive*>(O);
    if (!E)
        return false;
    if ((E->g_Team() == g_Team()) && (E->g_Alive()))
        return false;
    return true;
}

void CAI_Rat::feel_sound_new(
    IGameObject* who, int eType, CSound_UserDataPtr user_data, const Fvector& Position, float power)
{
    if (!g_Alive())
        return;

    if ((eType & SOUND_TYPE_WEAPON_SHOOTING) == SOUND_TYPE_WEAPON_SHOOTING)
        power = 1.f;

    if (power >= m_fSoundThreshold)
    {
        if ((this != who) && ((m_tLastSound.dwTime <= m_dwLastUpdateTime) || (m_tLastSound.fPower <= power)))
        {
            m_tLastSound.eSoundType = ESoundTypes(eType);
            m_tLastSound.dwTime = Device.dwTimeGlobal;
            m_tLastSound.fPower = power;
            m_tLastSound.tSavedPosition = Position;
            m_tLastSound.tpEntity = smart_cast<CEntityAlive*>(who);
            if ((eType & SOUND_TYPE_MONSTER_DYING) == SOUND_TYPE_MONSTER_DYING)
                m_fMorale += m_fMoraleDeathQuant;
            else if (((eType & SOUND_TYPE_WEAPON_SHOOTING) == SOUND_TYPE_WEAPON_SHOOTING) &&
                !memory().enemy().selected())
                m_fMorale += m_fMoraleFearQuant; /// fDistance;
            else if ((eType & SOUND_TYPE_MONSTER_ATTACKING) == SOUND_TYPE_MONSTER_ATTACKING)
                m_fMorale += m_fMoraleSuccessAttackQuant; /// fDistance;
        }
    }

    inherited::feel_sound_new(who, eType, user_data, Position, power);
}

bool CAI_Rat::feel_touch_on_contact(IGameObject* O) { return (inherited::feel_touch_on_contact(O)); }
