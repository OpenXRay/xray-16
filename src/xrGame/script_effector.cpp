////////////////////////////////////////////////////////////////////////////
//	Module 		: script_effector.cpp
//	Created 	: 06.02.2004
//  Modified 	: 06.02.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script effector class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "script_effector.h"
#include "actor.h"
#include "ActorEffector.h"

CScriptEffector::~CScriptEffector	()
{
	Msg								("CScriptEffector::~CScriptEffector() called");
}

BOOL CScriptEffector::Process		(SPPInfo	&pp)
{
	return							(!!process(&pp));
}

bool CScriptEffector::process		(SPPInfo *pp)
{
	return							(!!inherited::Process(*pp));
}

void CScriptEffector::Add			()
{
	Actor()->Cameras().AddPPEffector		(this);
}

void CScriptEffector::Remove		()
{
	Actor()->Cameras().RemovePPEffector	(m_tEffectorType);
}
