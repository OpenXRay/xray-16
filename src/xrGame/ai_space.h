////////////////////////////////////////////////////////////////////////////
//  Module      : ai_space.h
//  Created     : 12.11.2003
//  Modified    : 12.11.2003
//  Author      : Dmitriy Iassenev
//  Description : AI space class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/AISpaceBase.hpp"

class CGameGraph;
class CGameLevelCrossTable;
class CLevelGraph;
class CGraphEngine;
class CEF_Storage;
class CALifeSimulator;
class CCoverManager;
class CScriptEngine;
class CPatrolPathStorage;
class moving_objects;

namespace doors
{
class manager;
} // namespace doors

class CAI_Space : public AISpaceBase
{
private:
    friend class CALifeSimulator;
    friend class CALifeGraphRegistry;
    friend class CALifeSpawnRegistry;
    friend class CALifeSpawnRegistry;
    friend class CLevel;

private:
    CEF_Storage* m_ef_storage;
    CALifeSimulator* m_alife_simulator;
    CCoverManager* m_cover_manager;
    moving_objects* m_moving_objects;
    doors::manager* m_doors_manager;

private:
    void load(LPCSTR level_name);
    void unload(bool reload = false);
    void set_alife(CALifeSimulator* alife_simulator);
    void LoadCommonScripts();
    void RegisterScriptClasses();

public:
    CAI_Space();
    virtual ~CAI_Space();
    void init();
    void SetupScriptEngine();
    IC CEF_Storage& ef_storage() const;

    IC const CALifeSimulator& alife() const;
    IC const CALifeSimulator* get_alife() const;
    IC const CCoverManager& cover_manager() const;
    IC moving_objects& get_moving_objects() const;
    IC doors::manager& doors() const;
};

IC CAI_Space& ai();

extern CAI_Space* g_ai_space;

#include "ai_space_inline.h"
