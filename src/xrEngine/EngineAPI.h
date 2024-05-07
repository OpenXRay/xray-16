// EngineAPI.h: interface for the CEngineAPI class.
//
//****************************************************************************
// Support for extension DLLs
//****************************************************************************
#pragma once

#include "xrEngine/Engine.h"
#include "xrCore/clsid.h"

class IGame_Persistent;

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

class XR_NOVTABLE GameModule
{
public:
    virtual ~GameModule() = default;
    virtual void initialize(Factory_Create*& pCreate, Factory_Destroy*& pDestroy) = 0;
    virtual void finalize() = 0;
    virtual IGame_Persistent* create_persistent() = 0;
    virtual void destroy_persistent(IGame_Persistent*& persistent) = 0;
};

class XR_NOVTABLE RendererModule
{
public:
    virtual ~RendererModule() = default;
    virtual const xr_vector<std::pair<pcstr, int>>& ObtainSupportedModes() = 0;
    virtual bool CheckGameRequirements() = 0;
    virtual void SetupEnv(pcstr mode) = 0;
    virtual void ClearEnv() = 0;
};

class ENGINE_API CEngineAPI
{
    xr_map<shared_str, RendererModule*> renderModes;

    GameModule* gameModule{};
    RendererModule* selectedRenderer{};

    void SelectRenderer();
    void CloseUnusedLibraries() const;

public:
    Factory_Create*  pCreate;
    Factory_Destroy* pDestroy;

public:
    CEngineAPI();
    ~CEngineAPI();

    void CreateRendererList();
    void Initialize(GameModule* game);
    void Destroy();
};

ENGINE_API bool is_enough_address_space_available();

#define NEW_INSTANCE(a) Engine.External.pCreate(a)
#define DEL_INSTANCE(a)\
    {\
        Engine.External.pDestroy(a);\
        a = NULL;\
    }
