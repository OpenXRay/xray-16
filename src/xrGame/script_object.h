////////////////////////////////////////////////////////////////////////////
//	Module 		: script_object.h
//	Created 	: 06.10.2003
//  Modified 	: 14.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GameObject.h"
#include "script_entity.h"

class CScriptObject : public CGameObject, public CScriptEntity
{
public:
    CScriptObject();
    virtual ~CScriptObject();
    virtual IFactoryObject* _construct();
    virtual void reinit();
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual BOOL UsedAI_Locations();
    virtual void shedule_Update(u32 DT);
    virtual void UpdateCL();
    virtual CScriptEntity* cast_script_entity() { return this; }
};
