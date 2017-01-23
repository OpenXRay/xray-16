#pragma once
#include "CustomZone.h"

class CNoGravityZone :
	public CCustomZone
{
typedef CCustomZone inherited;
public:
protected:
	virtual		void	enter_Zone						(SZoneObjectInfo& io)				;
	virtual		void	exit_Zone						(SZoneObjectInfo& io)				;
private:
				void	switchGravity					(SZoneObjectInfo& io,bool val)		;
	virtual		void	UpdateWorkload					(u32	dt	)						;
};