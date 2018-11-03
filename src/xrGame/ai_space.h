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

#include <mutex>

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
public:
    class CEventCallback
    {
    public:
        using CID = size_t;
        static const CID INVALID_CID = std::numeric_limits<CID>::max();

        virtual void ProcessEvent() = 0;
        virtual ~CEventCallback(){};
    };

    class CEventCallbackStorage
    {
        xr_vector<xr_unique_ptr<CEventCallback>> m_callbacks;
        std::mutex m_lock;

    public:
        CEventCallback::CID RegisterCallback(CEventCallback* cb);
        bool UnregisterCallback(CEventCallback::CID cid);
        void ExecuteCallbacks();
    };

    class CNotifier
    {
    public:
        enum EEventID
        {
            EVENT_SCRIPT_ENGINE_STARTED,
            EVENT_SCRIPT_ENGINE_RESET,
            EVENT_COUNT,
        };

    private:
        xr_array<CEventCallbackStorage, EVENT_COUNT> m_callbacks;

    public:
        CEventCallback::CID RegisterCallback(CEventCallback* cb, EEventID event_id);
        bool UnregisterCallback(CEventCallback::CID cid, EEventID event_id);
        void FireEvent(EEventID event_id);
    };

private:
    friend class CALifeSimulator;
    friend class CALifeGraphRegistry;
    friend class CALifeSpawnRegistry;
    friend class CALifeSpawnRegistry;
    friend class CLevel;

private:
    bool m_inited = false;
    CNotifier m_events_notifier;

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

    CEventCallback::CID Subscribe(CEventCallback* cb, CNotifier::EEventID event_id);
    bool Unsubscribe(CEventCallback::CID cid, CNotifier::EEventID event_id);
    void RestartScriptEngine();

    IC CEF_Storage& ef_storage() const;

    IC const CALifeSimulator& alife() const;
    IC const CALifeSimulator* get_alife() const;
    IC const CCoverManager& cover_manager() const;
    IC moving_objects& get_moving_objects() const;
    IC doors::manager& doors() const;
};

IC CAI_Space& ai();

#include "ai_space_inline.h"
