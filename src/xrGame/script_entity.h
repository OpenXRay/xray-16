////////////////////////////////////////////////////////////////////////////
//	Module 		: script_entity.h
//	Created 	: 06.10.2003
//  Modified 	: 14.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Script entity class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "script_entity_space.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "xrCommon/xr_deque.h"

class CSE_Abstract;
class CGameObject;
class CScriptEntityAction;
class CEntity;
class CScriptGameObject;
class CCustomMonster;
class ref_sound;

using namespace ScriptEntity;

class CScriptEntity
{
public:
    struct CSavedSound
    {
        u16 m_game_object_id;
        int m_sound_type;
        Fvector m_position;
        float m_sound_power;

        IC CSavedSound(u16 game_object_id, int sound_type, const Fvector& position, float sound_power)
            : m_game_object_id(game_object_id), m_sound_type(sound_type), m_position(position),
              m_sound_power(sound_power)
        {
        }
    };

protected:
    typedef xr_deque<CScriptEntityAction*> ACTIONS;

private:
    CGameObject* m_object;
    CCustomMonster* m_monster;
    bool m_initialized;

    bool m_can_capture;

protected:
    ACTIONS m_tpActionQueue;
    bool m_bScriptControl;
    shared_str m_caScriptName;
    MotionID m_tpNextAnimation;
    bool m_use_animation_movement_controller;
    CScriptEntityAction* m_tpCurrentEntityAction;

public:
    MotionID m_tpScriptAnimation;

protected:
    ref_sound* m_current_sound;
    xr_vector<CSavedSound> m_saved_sounds;

public:
    CScriptEntity();
    virtual ~CScriptEntity();
    void init();
    virtual void reinit();
    virtual bool net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void shedule_Update(u32 DT);
    virtual void UpdateCL();
    virtual CScriptEntity* cast_script_entity() { return this; }
    virtual IFactoryObject* _construct();

public:
    const Fmatrix GetUpdatedMatrix(shared_str caBoneName, const Fvector& tPositionOffset, const Fvector& tAngleOffset);
    void vfUpdateParticles();
    void vfUpdateSounds();
    virtual void vfFinishAction(CScriptEntityAction* tpEntityAction);
    virtual void SetScriptControl(const bool bScriptControl, shared_str caSciptName);
    virtual bool GetScriptControl() const;
    virtual LPCSTR GetScriptControlName() const;
    virtual bool CheckObjectVisibility(const CGameObject* tpObject);
    virtual bool CheckTypeVisibility(const char* section_name);
    virtual bool CheckIfCompleted() const { return false; };
    virtual CScriptEntityAction* GetCurrentAction();
    virtual void AddAction(const CScriptEntityAction* tpEntityAction, bool bHighPriority = false);
    virtual void ProcessScripts();
    virtual void ResetScriptData(void* P = 0);
    virtual void ClearActionQueue();
    virtual bool bfAssignMovement(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignWatch(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignAnimation(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignSound(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignParticles(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignObject(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignMonsterAction(CScriptEntityAction* tpEntityAction);

    virtual void sound_callback(const IGameObject* object, int sound_type, const Fvector& position, float sound_power);

    virtual LPCSTR GetPatrolPathName();
    bool bfScriptAnimation();
    u32 GetActionCount() const;
    const CScriptEntityAction* GetActionByIndex(u32 action_index) const;

    virtual CEntity* GetCurrentEnemy();
    virtual CEntity* GetCurrentCorpse();
    virtual int get_enemy_strength();
    void process_sound_callbacks();

    void set_script_capture(bool val = true) { m_can_capture = val; }
    bool can_script_capture() { return m_can_capture; }
public:
    IC CGameObject& object() const;
};

#include "script_entity_inline.h"
