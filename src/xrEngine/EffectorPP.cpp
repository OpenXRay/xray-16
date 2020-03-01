// EffectorPP.cpp: implementation of the CEffectorFall class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EffectorPP.h"
#include "CameraManager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEffectorPP::CEffectorPP(EEffectorPPType type, f32 lifeTime, bool free_on_remove) : bOverlap(true)
{
    eType = type;
    fLifeTime = lifeTime;
    bFreeOnRemove = free_on_remove;
}

CEffectorPP::~CEffectorPP() {}
BOOL CEffectorPP::Process(SPPInfo& PPInfo)
{
    fLifeTime -= Device.fTimeDelta;
    return TRUE;
}
