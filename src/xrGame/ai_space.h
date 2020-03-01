////////////////////////////////////////////////////////////////////////////
//  Module      : ai_space.h
//  Created     : 12.11.2003
//  Modified    : 12.11.2003
//  Author      : Dmitriy Iassenev
//  Description : AI space class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/AISpaceBase.hpp"
#include "xrCommon/xr_array.h"
#include "xrCommon/xr_smart_pointers.h"
#include "xrCore/Events/Notifier.h"

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

public:
    enum EEventID
    {
        EVENT_SCRIPT_ENGINE_STARTED,
        EVENT_SCRIPT_ENGINE_RESET,
        EVENT_COUNT,
    };

private:
    bool m_inited = false;
    CEventNotifier<EVENT_COUNT> m_events_notifier;

    xr_unique_ptr<CEF_Storage> m_ef_storage;
    xr_unique_ptr<CCoverManager> m_cover_manager;
    xr_unique_ptr<moving_objects> m_moving_objects;
    xr_unique_ptr<doors::manager> m_doors_manager;

    CALifeSimulator* m_alife_simulator = nullptr;

private:
    void init();
    void load(LPCSTR level_name);
    void unload(bool reload = false);
    void set_alife(CALifeSimulator* alife_simulator);
    void LoadCommonScripts();
    void RegisterScriptClasses();
    void SetupScriptEngine();

public:
    CAI_Space() = default;
    CAI_Space(const CAI_Space&) = delete;
    CAI_Space& operator=(const CAI_Space&) = delete;
    virtual ~CAI_Space();
    static CAI_Space& GetInstance();

    template <class CB, class... Args>
    CEventNotifierCallback::CID Subscribe(EEventID event_id, Args&&... args)
    {
        return m_events_notifier.template CreateRegisteredCallback<CB>(event_id, args...);
    }

    bool Unsubscribe(CEventNotifierCallback::CID cid, EEventID event_id);
    void RestartScriptEngine();

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
