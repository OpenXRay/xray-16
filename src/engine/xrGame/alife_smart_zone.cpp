////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_smart_zone.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife smart zone class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife.h"

CSE_ALifeItemWeapon	*CSE_ALifeSmartZone::tpfGetBestWeapon	(ALife::EHitType		&tHitType,			float		&fHitPower)
{
	m_tpCurrentBestWeapon		= 0;
	return						(m_tpCurrentBestWeapon);
}

ALife::EMeetActionType CSE_ALifeSmartZone::tfGetActionType	(CSE_ALifeSchedulable	*tpALifeSchedulable,int			iGroupIndex, bool bMutualDetection)
{
	CSE_ALifeObject				*object = smart_cast<CSE_ALifeObject*>(tpALifeSchedulable->base());
	VERIFY						(object);
	return						((object->m_tGraphID == m_tGraphID) ? ALife::eMeetActionSmartTerrain : ALife::eMeetActionTypeIgnore);
}

bool CSE_ALifeSmartZone::bfActive							()
{
	return						(true);
}

CSE_ALifeDynamicObject *CSE_ALifeSmartZone::tpfGetBestDetector	()
{
	VERIFY2						(false,"This function shouldn't be called");
	return						(0);
}
