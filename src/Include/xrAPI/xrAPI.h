#pragma once

#ifdef XRAY_STATIC_BUILD
#   define XRAPI_API
#else
#   ifdef XRAPI_EXPORTS
#      define XRAPI_API XR_EXPORT
#   else
#      define XRAPI_API XR_IMPORT
#   endif
#endif

class IRender;
class IRenderFactory;
class IDebugRender;
class IRenderBackend;
class CDUInterface;
struct xr_token;
class IUIRender;
class CGameMtlLibrary;
class CScriptEngine;
class AISpaceBase;
class ISoundManager;
class UICore;

class XRAPI_API EngineGlobalEnvironment
{
public:
    IRender* Render;
    IRenderBackend* Backend;  // Modern backend (D3D12/Vulkan) - replaces HW
    IDebugRender* DRender;
    CDUInterface* DU;
    IUIRender* UIRender;
    IRenderFactory* RenderFactory;
    CScriptEngine* ScriptEngine;
    AISpaceBase* AISpace;
    ISoundManager* Sound;
    UICore* UI;

    bool isDedicatedServer;
};

extern XRAPI_API EngineGlobalEnvironment GEnv;
