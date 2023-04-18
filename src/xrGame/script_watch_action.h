////////////////////////////////////////////////////////////////////////////
//	Module 		: script_watch_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script watch action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_abstract_action.h"
#include "sight_manager_space.h"
/*
namespace SightManager
{
enum ESightType : u32;
};*/

class CScriptGameObject;

class CScriptWatchAction : public CScriptAbstractAction
{
public:
    enum EGoalType : u32
    {
        eGoalTypeObject = u32(0),
        eGoalTypeWatchType,
        eGoalTypeDirection,
        eGoalTypeCurrent,
    };

public:
    IGameObject* m_tpObjectToWatch{};
    SightManager::ESightType m_tWatchType{ SightManager::eSightTypeCurrentDirection };
    EGoalType m_tGoalType{ eGoalTypeCurrent };
    Fvector m_tWatchVector{};
    shared_str m_bone_to_watch;

    // Searchlight
    Fvector m_tTargetPoint;
    float vel_bone_x;
    float vel_bone_y;

public:
    CScriptWatchAction() = default;
    IC CScriptWatchAction(SightManager::ESightType tWatchType);
    IC CScriptWatchAction(SightManager::ESightType tWatchType, const Fvector& tDirection);
    IC CScriptWatchAction(
        SightManager::ESightType tWatchType, CScriptGameObject* tpObjectToWatch, LPCSTR bone_to_watch = "");
    // Searchlight look ///////////////////////////////////////////////
    CScriptWatchAction(const Fvector& tTarget, float vel1, float vel2);
    IC CScriptWatchAction(CScriptGameObject* tpObjectToWatch, float vel1, float vel2);
    ///////////////////////////////////////////////////////////////////
    void SetWatchObject(CScriptGameObject* tpObjectToWatch);
    IC void SetWatchType(SightManager::ESightType tWatchType);
    IC void SetWatchDirection(const Fvector& tDirection);
    IC void SetWatchBone(LPCSTR bone_to_watch);
    IC void initialize();
};

#include "script_watch_action_inline.h"
