////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_space.h
//	Created 	: 12.11.2003
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : AI space class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/Events/Notifier.h"

class CScriptEngine;

class CAI_Space
{
private:
    bool m_inited = false;

    void init();
    void RegisterScriptClasses();

public:
    CAI_Space() = default;
    CAI_Space(const CAI_Space&) = delete;
    CAI_Space& operator=(const CAI_Space&) = delete;
    virtual ~CAI_Space();
    static CAI_Space& GetInstance();

    IC CScriptEngine& script_engine() const;

    enum EEventID
    {
        EVENT_SCRIPT_ENGINE_STARTED,
        EVENT_SCRIPT_ENGINE_RESET,
        EVENT_COUNT,
    };

    template <class CB, class... Args>
    CEventNotifierCallback::CID Subscribe(EEventID event_id, Args&&... args)
    {
        return 0;
    }

    bool Unsubscribe(CEventNotifierCallback::CID cid, EEventID event_id) { return true; }
};

// Static initializer for every translation unit
// Need to avoid static initilization order problem
static struct SAI_Space_Initializer
{
    SAI_Space_Initializer();
    ~SAI_Space_Initializer();
} s_AISpaceInitializer;

IC CAI_Space& ai();

#include "ai_space_inline.h"
