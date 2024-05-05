#pragma once

#include "xrCore/clsid.h"
#include "xrEngine/EngineAPI.h"

#ifdef XRAY_STATIC_BUILD
#    define XRGAME_API
#else
#    ifdef XRGAME_EXPORTS
#        define XRGAME_API XR_EXPORT
#    else
#        define XRGAME_API XR_IMPORT
#    endif
#endif

extern "C"
{
XRGAME_API IFactoryObject* __cdecl xrFactory_Create(CLASS_ID clsid);
XRGAME_API void __cdecl xrFactory_Destroy(IFactoryObject* O);
}

class xrGameModule final : public GameModule
{
public:
    void initialize(Factory_Create*& pCreate, Factory_Destroy*& pDestroy) override;
    void finalize() override;
    IGame_Persistent* create_persistent() override;
    void destroy_persistent(IGame_Persistent*& persistent) override;
};

extern XRGAME_API xrGameModule xrGame;
