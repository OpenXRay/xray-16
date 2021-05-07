#pragma once

#ifdef XRAPI_EXPORTS
#define XRAPI_API XR_EXPORT
#else
#define XRAPI_API XR_IMPORT
#endif

class IRender;
class IRenderFactory;
class IDebugRender;
class CDUInterface;
struct xr_token;
class IUIRender;
class CGameMtlLibrary;
class CRender;
class CScriptEngine;
class AISpaceBase;
class ISoundManager;
class UICore;

class XRAPI_API EngineGlobalEnvironment
{
public:
    IRender* Render;
    IDebugRender* DRender;
    CDUInterface* DU;
    IUIRender* UIRender;
    CGameMtlLibrary* PGMLib;
    IRenderFactory* RenderFactory;
    CScriptEngine* ScriptEngine;
    AISpaceBase* AISpace;
    ISoundManager* Sound;
    UICore* UI;

    bool isEditor;
    bool isDedicatedServer;
};

extern XRAPI_API EngineGlobalEnvironment GEnv;
