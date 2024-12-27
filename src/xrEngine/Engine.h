#pragma once

#ifdef XRAY_STATIC_BUILD
#    define ENGINE_API
#else
#    ifdef ENGINE_BUILD
#        define ENGINE_API XR_EXPORT
#    else
#        define ENGINE_API XR_IMPORT
#    endif
#endif

#include "pure.h"
#include "EngineAPI.h"
#include "EventAPI.h"
#include "xrSheduler.h"
#include "xrSound/Sound.h"

// TODO: this should be in render configuration
#define R__NUM_SUN_CASCADES         (3u) // csm/s.ligts
#define R__NUM_AUX_CONTEXTS         (1u) // rain/s.lights
#define R__NUM_PARALLEL_CONTEXTS    (R__NUM_SUN_CASCADES + R__NUM_AUX_CONTEXTS)
#define R__NUM_CONTEXTS             (R__NUM_PARALLEL_CONTEXTS + 1/* imm */)

class ENGINE_API CEngine final : public pureFrame, public IEventReceiver
{
    EVENT eQuit;

public:
    // DLL api stuff
    CEngineAPI External;
    CEventAPI Event;
    CSheduler Sheduler;
    CSoundManager Sound;

    void Initialize(GameModule* game);
    void Destroy();

    void OnEvent(EVENT E, u64 P1, u64 P2) override;
    void OnFrame() override;

    CEngine();
    ~CEngine();
};

ENGINE_API extern CEngine Engine;
