#include "stdafx.h"
#include "dxStatsRender.h"
#include "../../xrEngine/GameFont.h"
#include "dxRenderDeviceRender.h"

void dxStatsRender::Copy(IStatsRender&_in)
{
	*this = *((dxStatsRender*)&_in);
}

void dxStatsRender::OutData1 (CGameFont &F)
{
	F.OutNext	("VERT:        %d/%d",		RCache.stat.verts,RCache.stat.calls?RCache.stat.verts/RCache.stat.calls:0);
	F.OutNext	("POLY:        %d/%d",		RCache.stat.polys,RCache.stat.calls?RCache.stat.polys/RCache.stat.calls:0);
	F.OutNext	("DIP/DP:      %d",			RCache.stat.calls);
}

void dxStatsRender::OutData2 (CGameFont &F)
{
#ifdef DEBUG
	F.OutNext	("SH/T/M/C:    %d/%d/%d/%d",RCache.stat.states,RCache.stat.textures,RCache.stat.matrices,RCache.stat.constants);
	F.OutNext	("RT/PS/VS:    %d/%d/%d",	RCache.stat.target_rt,RCache.stat.ps,RCache.stat.vs);
	F.OutNext	("DCL/VB/IB:   %d/%d/%d",   RCache.stat.decl,RCache.stat.vb,RCache.stat.ib);
#endif
}

void dxStatsRender::OutData3 (CGameFont &F)
{
	F.OutNext	("xforms:      %d",			RCache.stat.xforms);
}

void dxStatsRender::OutData4 (CGameFont &F)
{
	F.OutNext	("static:        %3.1f/%d",	RCache.stat.r.s_static.verts/1024.f,		RCache.stat.r.s_static.dips );
	F.OutNext	("flora:         %3.1f/%d",	RCache.stat.r.s_flora.verts/1024.f,			RCache.stat.r.s_flora.dips );
	F.OutNext	("  flora_lods:  %3.1f/%d",	RCache.stat.r.s_flora_lods.verts/1024.f,	RCache.stat.r.s_flora_lods.dips );
	F.OutNext	("dynamic:       %3.1f/%d",	RCache.stat.r.s_dynamic.verts/1024.f,		RCache.stat.r.s_dynamic.dips );
	F.OutNext	("  dynamic_sw:  %3.1f/%d",	RCache.stat.r.s_dynamic_sw.verts/1024.f,	RCache.stat.r.s_dynamic_sw.dips );
	F.OutNext	("  dynamic_inst:%3.1f/%d",	RCache.stat.r.s_dynamic_inst.verts/1024.f,	RCache.stat.r.s_dynamic_inst.dips );
	F.OutNext	("  dynamic_1B:  %3.1f/%d",	RCache.stat.r.s_dynamic_1B.verts/1024.f,	RCache.stat.r.s_dynamic_1B.dips );
	F.OutNext	("  dynamic_2B:  %3.1f/%d",	RCache.stat.r.s_dynamic_2B.verts/1024.f,	RCache.stat.r.s_dynamic_2B.dips );
	F.OutNext	("  dynamic_3B:  %3.1f/%d",	RCache.stat.r.s_dynamic_3B.verts/1024.f,	RCache.stat.r.s_dynamic_3B.dips );
	F.OutNext	("  dynamic_4B:  %3.1f/%d",	RCache.stat.r.s_dynamic_4B.verts/1024.f,	RCache.stat.r.s_dynamic_4B.dips );
	F.OutNext	("details:       %3.1f/%d",	RCache.stat.r.s_details.verts/1024.f,		RCache.stat.r.s_details.dips );
}

void dxStatsRender::GuardVerts (CGameFont &F)
{
	if (RCache.stat.verts>500000)	F.OutNext	("Verts     > 500k: %d",	RCache.stat.verts);
}

void dxStatsRender::GuardDrawCalls (CGameFont &F)
{
	if (RCache.stat.calls>1000)		F.OutNext	("DIP/DP    > 1k:   %d",	RCache.stat.calls);
}

void dxStatsRender::SetDrawParams (IRenderDeviceRender *pRender)
{
	dxRenderDeviceRender *pR = (dxRenderDeviceRender*) pRender;

	RCache.set_xform_world  (Fidentity);
	RCache.set_Shader		(pR->m_SelectionShader);
	RCache.set_c			("tfactor",1,1,1,1);
}