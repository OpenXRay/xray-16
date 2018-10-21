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
};

extern XRAPI_API EngineGlobalEnvironment GEnv;
