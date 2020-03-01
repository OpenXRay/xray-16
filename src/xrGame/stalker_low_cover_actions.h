////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_low_cover_actions.h
//	Created 	: 05.09.2007
//  Modified 	: 05.09.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker low cover actions
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_LOW_COVER_ACTIONS_H_INCLUDED
#define STALKER_LOW_COVER_ACTIONS_H_INCLUDED

#include "stalker_combat_action_base.h"

class CCoverPoint;

namespace MonsterSpace
{
enum EBodyState : u32;
enum EMovementType : u32;
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionGetItemToKillLowCover
//////////////////////////////////////////////////////////////////////////

class CStalkerActionGetReadyToKillLowCover : public CStalkerActionCombatBase
{
protected:
    typedef CStalkerActionCombatBase inherited;

public:
    CStalkerActionGetReadyToKillLowCover(CAI_Stalker* object, LPCSTR action_name = "");
    virtual void initialize();
    virtual void execute();
    virtual void finalize();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionKillEnemyLowCover
//////////////////////////////////////////////////////////////////////////

class CStalkerActionKillEnemyLowCover : public CStalkerActionCombatBase
{
protected:
    typedef CStalkerActionCombatBase inherited;

private:
    u32 m_last_change_time;
    CRandom32 m_crouch_look_out_random;

public:
    CStalkerActionKillEnemyLowCover(CAI_Stalker* object, LPCSTR action_name = "");
    virtual void initialize();
    virtual void execute();
    virtual void finalize();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerActionHoldPositionLowCover
//////////////////////////////////////////////////////////////////////////

class CStalkerActionHoldPositionLowCover : public CStalkerActionCombatBase
{
protected:
    typedef CStalkerActionCombatBase inherited;

private:
    u32 m_last_change_time;
    CRandom32 m_crouch_look_out_random;

public:
    CStalkerActionHoldPositionLowCover(CAI_Stalker* object, LPCSTR action_name = "");
    virtual void initialize();
    virtual void execute();
    virtual void finalize();
};

#endif // STALKER_LOW_COVER_ACTIONS_H_INCLUDED
