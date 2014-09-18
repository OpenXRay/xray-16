#ifndef	dxStatsRender_included
#define	dxStatsRender_included
#pragma once

#include "..\..\Include\xrRender\StatsRender.h"

class dxStatsRender : public IStatsRender
{
public:
	virtual void Copy(IStatsRender &_in);

	virtual void OutData1 (CGameFont &F);
	virtual void OutData2 (CGameFont &F);
	virtual void OutData3 (CGameFont &F);
	virtual void OutData4 (CGameFont &F);
	virtual void GuardVerts (CGameFont &F);
	virtual void GuardDrawCalls (CGameFont &F);
	virtual void SetDrawParams (IRenderDeviceRender *pRender);
private:
};

#endif	//	dxStatsRender_included