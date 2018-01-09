#pragma once

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

class XRAPI_API EngineGlobalEnvironment
{
    using SupportCheck = bool(*)();
    using SetupEnv = void(*)();
public:
#ifdef _EDITOR
    CRender* Render;
#else
    IRender* Render;
    IDebugRender* DRender;
    CDUInterface* DU;
    xr_token* vid_mode_token;
    IUIRender* UIRender;
    CGameMtlLibrary* PGMLib;
#endif
    IRenderFactory* RenderFactory;
    CScriptEngine* ScriptEngine;
    AISpaceBase* AISpace;
    ISoundManager* Sound;

    bool isDedicatedServer;

    SupportCheck CheckR2;
    SupportCheck CheckR3;
    SupportCheck CheckR4;
    SetupEnv SetupR1;
    SetupEnv SetupR2;
    SetupEnv SetupR3;
    SetupEnv SetupR4;
    SetupEnv SetupCurrentRenderer;
};

extern XRAPI_API EngineGlobalEnvironment GEnv;
