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
class UICore;

class XRAPI_API EngineGlobalEnvironment
{
    using SupportCheck = bool(*)();
    using SetupEnv = void(*)();

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

    int CurrentRenderer;
    SupportCheck CheckR2;
    SupportCheck CheckR3;
    SupportCheck CheckR4;
    SupportCheck CheckRGL;
    SetupEnv SetupR1;
    SetupEnv SetupR2;
    SetupEnv SetupR3;
    SetupEnv SetupR4;
    SetupEnv SetupRGL;
    SetupEnv SetupCurrentRenderer;
};

extern XRAPI_API EngineGlobalEnvironment GEnv;
