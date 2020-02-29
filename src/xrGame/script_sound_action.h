////////////////////////////////////////////////////////////////////////////
//	Module 		: script_sound_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script sound action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_abstract_action.h"
#include "ai_sounds.h"
#include "ai_monster_space.h"
#include "script_sound.h"
#include "ai/monsters/monster_sound_defs.h"

class CScriptSoundAction : public CScriptAbstractAction
{
public:
    enum EGoalType
    {
        eGoalTypeSoundAttached = u32(0),
        eGoalTypeSoundPosition,
        eGoalTypeDummy = u32(-1),
    };

public:
    shared_str m_caSoundToPlay;
    shared_str m_caBoneName;
    EGoalType m_tGoalType;
    bool m_bLooped;
    bool m_bStartedToPlay;
    Fvector m_tSoundPosition;
    Fvector m_tSoundAngles;
    ESoundTypes m_sound_type;
    MonsterSound::EType m_monster_sound;
    int m_monster_sound_delay;
    MonsterSpace::EMonsterHeadAnimType m_tHeadAnimType;

public:
    IC CScriptSoundAction();
    IC CScriptSoundAction(const char* caSoundToPlay, const char* caBoneName,
        const Fvector& tPositionOffset = Fvector().set(0, 0, 0), const Fvector& tAngleOffset = Fvector().set(0, 0, 0),
        bool bLooped = false, ESoundTypes sound_type = SOUND_TYPE_NO_SOUND);
    IC CScriptSoundAction(const char* caSoundToPlay, Fvector* tPosition,
        const Fvector& tAngleOffset = Fvector().set(0, 0, 0), bool bLooped = false,
        ESoundTypes sound_type = SOUND_TYPE_NO_SOUND);
    IC CScriptSoundAction(CScriptSound* sound, const char* caBoneName,
        const Fvector& tPositionOffset = Fvector().set(0, 0, 0), const Fvector& tAngleOffset = Fvector().set(0, 0, 0),
        bool bLooped = false, ESoundTypes sound_type = SOUND_TYPE_NO_SOUND);
    IC CScriptSoundAction(CScriptSound* sound, Fvector* tPosition, const Fvector& tAngleOffset = Fvector().set(0, 0, 0),
        bool bLooped = false, ESoundTypes sound_type = SOUND_TYPE_NO_SOUND);
    ////////////////////////////////////////////////////////////////////////////////////
    // Monster Specific
    ///////////////////////////////////////////////////////////////////////////////////
    IC CScriptSoundAction(MonsterSound::EType sound_type);
    IC CScriptSoundAction(MonsterSound::EType sound_type, int delay);
    ////////////////////////////////////////////////////////////////////////////////////
    // Trader Specific
    ///////////////////////////////////////////////////////////////////////////////////
    IC CScriptSoundAction(const char* caSoundToPlay, const char* caBoneName, MonsterSpace::EMonsterHeadAnimType head_anim_type);
    virtual ~CScriptSoundAction();
    void SetSound(const char* caSoundToPlay);
    IC void SetSound(const CScriptSound& sound);
    IC void SetPosition(const Fvector& tPosition);
    IC void SetBone(const char* caBoneName);
    IC void SetAngles(const Fvector& tAngles);
    IC void SetSoundType(const ESoundTypes sound_type);
    IC void initialize();
};

#include "script_sound_action_inline.h"
