#ifndef	StatsRender_included
#define	StatsRender_included
#pragma once

class CGameFont;
class IRenderDeviceRender;

class IStatsRender
{
public:
	virtual ~IStatsRender() {;}
	virtual void Copy(IStatsRender &_in) = 0;
	virtual void OutData1 (CGameFont &F) = 0;
	virtual void OutData2 (CGameFont &F) = 0;
	virtual void OutData3 (CGameFont &F) = 0;
	virtual void OutData4 (CGameFont &F) = 0;
	virtual void GuardVerts (CGameFont &F) = 0;
	virtual void GuardDrawCalls (CGameFont &F) = 0;
	virtual void SetDrawParams (IRenderDeviceRender *pRender) = 0;
};

#endif	//	StatsRender_included