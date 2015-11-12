// EngineAPI.h: interface for the CEngineAPI class.
//
//****************************************************************************
// Support for extension DLLs
//****************************************************************************

#if !defined(AFX_ENGINEAPI_H__CF21372B_C8B8_4891_82FC_D872C84E1DD4__INCLUDED_)
#define AFX_ENGINEAPI_H__CF21372B_C8B8_4891_82FC_D872C84E1DD4__INCLUDED_
#pragma once
#include "xrEngine/Engine.h"

class IFactoryObject
{
public:
    virtual ~IFactoryObject() = 0;
    virtual CLASS_ID &GetClassId() = 0;
    virtual IFactoryObject *_construct() = 0;
};

inline IFactoryObject::~IFactoryObject() {}
inline IFactoryObject *IFactoryObject::_construct() { return this; }

class ENGINE_API FactoryObjectBase : public IFactoryObject
{
public:
    CLASS_ID CLS_ID;

    FactoryObjectBase(void* params) { CLS_ID = 0; };
    FactoryObjectBase() { CLS_ID = 0; };
    virtual CLASS_ID &GetClassId() override { return CLS_ID; }
    virtual IFactoryObject *_construct() override { return IFactoryObject::_construct(); }
    virtual ~FactoryObjectBase() {};
};

// Class creation/destroying interface
extern "C" {
    typedef DLL_API IFactoryObject* __cdecl Factory_Create(CLASS_ID CLS_ID);
    typedef DLL_API void __cdecl Factory_Destroy(IFactoryObject* O);
};

// Tuning interface
extern "C" {
    typedef void __cdecl VTPause(void);
    typedef void __cdecl VTResume(void);
};

class ENGINE_API CEngineAPI
{
private:
    HMODULE hGame;
    HMODULE hRender;
    HMODULE hTuner;
public:
    BENCH_SEC_SCRAMBLEMEMBER1
    Factory_Create* pCreate;
    Factory_Destroy* pDestroy;
    BOOL tune_enabled;
    VTPause* tune_pause;
    VTResume* tune_resume;
    void Initialize();

#ifndef DEDICATED_SERVER
    void InitializeNotDedicated();
#endif // DEDICATED_SERVER

    void Destroy();

    void CreateRendererList();

    CEngineAPI();
    ~CEngineAPI();
};

ENGINE_API bool is_enough_address_space_available();

#define NEW_INSTANCE(a) Engine.External.pCreate(a)
#define DEL_INSTANCE(a) { Engine.External.pDestroy(a); a=NULL; }

#endif // !defined(AFX_ENGINEAPI_H__CF21372B_C8B8_4891_82FC_D872C84E1DD4__INCLUDED_)
