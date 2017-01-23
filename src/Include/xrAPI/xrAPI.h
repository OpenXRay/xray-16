#pragma once

#include "Common/Platform.hpp"

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

class XRAPI_API EngineGlobalEnvironment
{
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
    AISpaceBase *AISpace;
};

extern XRAPI_API EngineGlobalEnvironment GlobalEnv; // XXX: rename to GEnv
extern XRAPI_API bool g_dedicated_server; // XXX: move to EngineGlobalEnvironment
