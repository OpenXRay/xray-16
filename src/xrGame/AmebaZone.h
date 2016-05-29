#pragma once

#include "CustomZone.h"
#include "../Include/xrRender/KinematicsAnimated.h"
#include "ZoneVisual.h"
#include "../../../xrphysics/PHUpdateObject.h"

class CAmebaZone :
	public CVisualZone,
	public CPHUpdateObject
{
typedef				CVisualZone		inherited;	
float m_fVelocityLimit;
public:
									CAmebaZone			()						;
									~CAmebaZone			()						;
	virtual				void		Affect				(SZoneObjectInfo* O)		;
	
protected:
	virtual				void		PhTune				(float step)			;
	virtual				void		PhDataUpdate		(float step)			{;}
	virtual				bool		BlowoutState		()						;
	virtual				void		SwitchZoneState		(EZoneState new_state)	;
	virtual				void		Load				(LPCSTR section)		;
	virtual				float		distance_to_center	(CObject* O)			;	
};
