// Engine.cpp: implementation of the CEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Engine.h"

CEngine Engine;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEngine::CEngine() {}
CEngine::~CEngine() {}
extern void msCreate(pcstr name);

void CEngine::Initialize(void)
{
    Engine.Sheduler.Initialize();
    Engine.Scheduler.Initialize();
#ifdef DEBUG
    msCreate("game");
#endif
}

void CEngine::Destroy()
{
    Engine.Sheduler.Destroy();
    Engine.Scheduler.Destroy();
    Engine.External.Destroy();
}
