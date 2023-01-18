// EngineAPI.h: interface for the CEngineAPI class.
//
//****************************************************************************
// Support for extension DLLs
//****************************************************************************
#pragma once

#include "xrEngine/Engine.h"
#include "xrCore/ModuleLookup.hpp"
#include "xrCore/clsid.h"

#include <memory>

class XR_NOVTABLE IFactoryObject
{
public:
    virtual ~IFactoryObject() = 0;
    virtual CLASS_ID& GetClassId() = 0;
    virtual IFactoryObject* _construct() = 0;
};

inline IFactoryObject::~IFactoryObject() = default;
inline IFactoryObject* IFactoryObject::_construct() { return this; }

class ENGINE_API XR_NOVTABLE FactoryObjectBase : public virtual IFactoryObject
{
public:
    CLASS_ID CLS_ID;

    FactoryObjectBase(void* params) { CLS_ID = 0; }
    FactoryObjectBase() { CLS_ID = 0; }
    virtual CLASS_ID& GetClassId() override { return CLS_ID; }
    virtual IFactoryObject* _construct() override { return IFactoryObject::_construct(); }
};

// Class creation/destroying interface
extern "C" {
using Factory_Create = IFactoryObject* __cdecl(CLASS_ID CLS_ID);
using Factory_Destroy = void __cdecl(IFactoryObject* O);
}

class XR_NOVTABLE RendererModule
{
public:
    virtual ~RendererModule() = default;
    virtual const xr_vector<pcstr>& ObtainSupportedModes() = 0;
    virtual void SetupEnv(pcstr mode) = 0;
};

class ENGINE_API CEngineAPI
{
    using InitializeGameLibraryProc = void(*)();
    using FinalizeGameLibraryProc = void(*)();

    using GetRendererModule = RendererModule*(*)();

    struct RendererDesc
    {
        pcstr libraryName;
        XRay::Module handle;
        RendererModule* module;
    };

    xr_vector<RendererDesc> renderers;
    xr_map<shared_str, RendererModule*> renderModes;

    RendererModule* selectedRenderer{};

    XRay::Module hGame;

    InitializeGameLibraryProc pInitializeGame{};
    FinalizeGameLibraryProc pFinalizeGame{};

public:
    Factory_Create*  pCreate;
    Factory_Destroy* pDestroy;

    void Initialize();

    void InitializeRenderers();
    pcstr SelectRenderer();
    void CloseUnusedLibraries();

    void Destroy();

    void CreateRendererList();
    bool CanSkipGameModuleLoading() const { return !!strstr(Core.Params, "-nogame"); }

    CEngineAPI();
    ~CEngineAPI();
};

ENGINE_API bool is_enough_address_space_available();

#define NEW_INSTANCE(a) Engine.External.pCreate(a)
#define DEL_INSTANCE(a)\
    {\
        Engine.External.pDestroy(a);\
        a = NULL;\
    }
