////////////////////////////////////////////////////////////////////////////
//	Module 		: script_object.cpp
//	Created 	: 06.10.2003
//  Modified 	: 14.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "script_object.h"

CScriptObject::CScriptObject() {}
CScriptObject::~CScriptObject() {}
IFactoryObject* CScriptObject::_construct()
{
    CGameObject::_construct();
    CScriptEntity::_construct();
    return (this);
}

void CScriptObject::reinit()
{
    CScriptEntity::reinit();
    CGameObject::reinit();
}

BOOL CScriptObject::net_Spawn(CSE_Abstract* DC) { return (CGameObject::net_Spawn(DC) && CScriptEntity::net_Spawn(DC)); }
void CScriptObject::net_Destroy()
{
    CGameObject::net_Destroy();
    CScriptEntity::net_Destroy();
}

BOOL CScriptObject::UsedAI_Locations() { return (FALSE); }
void CScriptObject::shedule_Update(u32 DT)
{
    CGameObject::shedule_Update(DT);
    CScriptEntity::shedule_Update(DT);
}

void CScriptObject::UpdateCL()
{
    CGameObject::UpdateCL();
    CScriptEntity::UpdateCL();
}
