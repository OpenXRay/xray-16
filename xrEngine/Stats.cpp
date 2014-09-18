#include "stdafx.h"
#include "GameFont.h"
#pragma hdrstop

#include "../xrcdb/ISpatial.h"
#include "IGame_Persistent.h"
#include "render.h"
#include "xr_object.h"

#include "../Include/xrRender/DrawUtils.h"

int		g_ErrorLineCount	= 15;
Flags32 g_stats_flags		= {0};

// stats
DECLARE_RP(Stats);

class	optimizer	{
	float	average_	;
	BOOL	enabled_	;
public:
	optimizer	()		{
		average_	= 30.f;
//		enabled_	= TRUE;
//		disable		();
		// because Engine is not exist
		enabled_	= FALSE;
	}

	BOOL	enabled	()	{ return enabled_;	}
	void	enable	()	{ if (!enabled_)	{ Engine.External.tune_resume	();	enabled_=TRUE;	}}
	void	disable	()	{ if (enabled_)		{ Engine.External.tune_pause	();	enabled_=FALSE; }}
	void	update	(float value)	{
		if (value < average_*0.7f)	{
			// 25% deviation
			enable	();
		} else {
			disable	();
		};
		average_	= 0.99f*average_ + 0.01f*value;
	};
};
static	optimizer	vtune;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
BOOL			g_bDisableRedText	= FALSE;
CStats::CStats	()
{
	fFPS				= 30.f;
	fRFPS				= 30.f;
	fTPS				= 0;
	pFont				= 0;
	fMem_calls			= 0;
	RenderDUMP_DT_Count = 0;
	Device.seqRender.Add		(this,REG_PRIORITY_LOW-1000);
}

CStats::~CStats()
{
	Device.seqRender.Remove		(this);
	xr_delete		(pFont);
}

void _draw_cam_pos(CGameFont* pFont)
{
	float sz		= pFont->GetHeight();
	pFont->SetHeightI(0.02f);
	pFont->SetColor	(0xffffffff);
	pFont->Out		(10, 600, "CAMERA POSITION:  [%3.2f,%3.2f,%3.2f]",VPUSH(Device.vCameraPosition));
	pFont->SetHeight(sz);
	pFont->OnRender	();
}

void CStats::Show() 
{
	// Stop timers
	{
		EngineTOTAL.FrameEnd		();	
		Sheduler.FrameEnd			();	
		UpdateClient.FrameEnd		();	
		Physics.FrameEnd			();	
		ph_collision.FrameEnd		();
		ph_core.FrameEnd			();
		Animation.FrameEnd			();	
		AI_Think.FrameEnd			();
		AI_Range.FrameEnd			();
		AI_Path.FrameEnd			();
		AI_Node.FrameEnd			();
		AI_Vis.FrameEnd				();
		AI_Vis_Query.FrameEnd		();
		AI_Vis_RayTests.FrameEnd	();
		
		RenderTOTAL.FrameEnd		();
		RenderCALC.FrameEnd			();
		RenderCALC_HOM.FrameEnd		();
		RenderDUMP.FrameEnd			();	
		RenderDUMP_RT.FrameEnd		();
		RenderDUMP_SKIN.FrameEnd	();	
		RenderDUMP_Wait.FrameEnd	();	
		RenderDUMP_Wait_S.FrameEnd	();	
		RenderDUMP_HUD.FrameEnd		();	
		RenderDUMP_Glows.FrameEnd	();	
		RenderDUMP_Lights.FrameEnd	();	
		RenderDUMP_WM.FrameEnd		();	
		RenderDUMP_DT_VIS.FrameEnd	();	
		RenderDUMP_DT_Render.FrameEnd();	
		RenderDUMP_DT_Cache.FrameEnd();
		RenderDUMP_Pcalc.FrameEnd	();	
		RenderDUMP_Scalc.FrameEnd	();	
		RenderDUMP_Srender.FrameEnd	();	
		
		Sound.FrameEnd				();
		Input.FrameEnd				();
		clRAY.FrameEnd				();	
		clBOX.FrameEnd				();
		clFRUSTUM.FrameEnd			();
		
		netClient1.FrameEnd			();
		netClient2.FrameEnd			();
		netServer.FrameEnd			();

		netClientCompressor.FrameEnd();
		netServerCompressor.FrameEnd();
		
		TEST0.FrameEnd				();
		TEST1.FrameEnd				();
		TEST2.FrameEnd				();
		TEST3.FrameEnd				();

		g_SpatialSpace->stat_insert.FrameEnd		();
		g_SpatialSpace->stat_remove.FrameEnd		();
		g_SpatialSpacePhysic->stat_insert.FrameEnd	();
		g_SpatialSpacePhysic->stat_remove.FrameEnd	();
	}

	// calc FPS & TPS
	if (Device.fTimeDelta>EPS_S) {
		float fps  = 1.f/Device.fTimeDelta;
		//if (Engine.External.tune_enabled)	vtune.update	(fps);
		float fOne = 0.3f;
		float fInv = 1.f-fOne;
		fFPS = fInv*fFPS + fOne*fps;

		if (RenderTOTAL.result>EPS_S) {
			u32	rendered_polies = Device.m_pRender->GetCacheStatPolys();
			fTPS = fInv*fTPS + fOne*float(rendered_polies)/(RenderTOTAL.result*1000.f);
			//fTPS = fInv*fTPS + fOne*float(RCache.stat.polys)/(RenderTOTAL.result*1000.f);
			fRFPS= fInv*fRFPS+ fOne*1000.f/RenderTOTAL.result;
		}
	}
	{
		float mem_count		= float	(Memory.stat_calls);
		if (mem_count>fMem_calls)	fMem_calls	=	mem_count;
		else						fMem_calls	=	.9f*fMem_calls + .1f*mem_count;
		Memory.stat_calls	= 0		;
	}

	////////////////////////////////////////////////
	if (g_dedicated_server) return;
	////////////////////////////////////////////////
	int frm = 2000;
	div_t ddd = div(Device.dwFrame,frm);
	if( ddd.rem < frm/2.0f ){
		pFont->SetColor	(0xFFFFFFFF	);
		pFont->OutSet	(0,0);
		pFont->OutNext	(*eval_line_1);
		pFont->OutNext	(*eval_line_2);
		pFont->OutNext	(*eval_line_3);
		pFont->OnRender	();
	}

	CGameFont& F = *pFont;
	float		f_base_size	= 0.01f;
				F.SetHeightI	(f_base_size);

	if (vtune.enabled())	{
		float sz		= pFont->GetHeight();
		pFont->SetHeightI(0.02f);
		pFont->SetColor	(0xFFFF0000	);
		pFont->OutSet	(Device.dwWidth/2.0f+(pFont->SizeOf_("--= tune =--")/2.0f),Device.dwHeight/2.0f);
		pFont->OutNext	("--= tune =--");
		pFont->OnRender	();
		pFont->SetHeight(sz);
	};

	// Show them
	if (psDeviceFlags.test(rsStatistic))
	{
		static float	r_ps		= 0;
		static float	b_ps		= 0;
		r_ps						= .99f*r_ps + .01f*(clRAY.count/clRAY.result);
		b_ps						= .99f*b_ps + .01f*(clBOX.count/clBOX.result);

		CSound_stats				snd_stat;
		::Sound->statistic			(&snd_stat,0);
		F.SetColor	(0xFFFFFFFF	);

		F.OutSet	(0,0);
		F.OutNext	("FPS/RFPS:    %3.1f/%3.1f",fFPS,fRFPS);
		F.OutNext	("TPS:         %2.2f M",	fTPS);
		m_pRender->OutData1(F);
		//F.OutNext	("VERT:        %d/%d",		RCache.stat.verts,RCache.stat.calls?RCache.stat.verts/RCache.stat.calls:0);
		//F.OutNext	("POLY:        %d/%d",		RCache.stat.polys,RCache.stat.calls?RCache.stat.polys/RCache.stat.calls:0);
		//F.OutNext	("DIP/DP:      %d",			RCache.stat.calls);
#ifdef DEBUG
		F.OutSkip	();
		F.OutNext	("mapped:      %d",			g_file_mapped_memory);
		F.OutSkip	();
		m_pRender->OutData2(F);
		//F.OutNext	("SH/T/M/C:    %d/%d/%d/%d",RCache.stat.states,RCache.stat.textures,RCache.stat.matrices,RCache.stat.constants);
		//F.OutNext	("RT/PS/VS:    %d/%d/%d",	RCache.stat.target_rt,RCache.stat.ps,RCache.stat.vs);
		//F.OutNext	("DCL/VB/IB:   %d/%d/%d",   RCache.stat.decl,RCache.stat.vb,RCache.stat.ib);
#endif
		m_pRender->OutData3(F);
		//F.OutNext	("xforms:      %d",			RCache.stat.xforms);
		F.OutSkip	();

#define PPP(a) (100.f*float(a)/float(EngineTOTAL.result))
		F.OutNext	("*** ENGINE:  %2.2fms",EngineTOTAL.result);	
		F.OutNext	("Memory:      %2.2fa",fMem_calls);
		F.OutNext	("uClients:    %2.2fms, %2.1f%%, crow(%d)/active(%d)/total(%d)",UpdateClient.result,PPP(UpdateClient.result),UpdateClient_crows,UpdateClient_active,UpdateClient_total);
		F.OutNext	("uSheduler:   %2.2fms, %2.1f%%",Sheduler.result,		PPP(Sheduler.result));
		F.OutNext	("uSheduler_L: %2.2fms",fShedulerLoad);
		F.OutNext	("uParticles:  Qstart[%d] Qactive[%d] Qdestroy[%d]",	Particles_starting,Particles_active,Particles_destroy);
		F.OutNext	("spInsert:    o[%.2fms, %2.1f%%], p[%.2fms, %2.1f%%]",	g_SpatialSpace->stat_insert.result, PPP(g_SpatialSpace->stat_insert.result),	g_SpatialSpacePhysic->stat_insert.result, PPP(g_SpatialSpacePhysic->stat_insert.result));
		F.OutNext	("spRemove:    o[%.2fms, %2.1f%%], p[%.2fms, %2.1f%%]",	g_SpatialSpace->stat_remove.result, PPP(g_SpatialSpace->stat_remove.result),	g_SpatialSpacePhysic->stat_remove.result, PPP(g_SpatialSpacePhysic->stat_remove.result));
		F.OutNext	("Physics:     %2.2fms, %2.1f%%",Physics.result,		PPP(Physics.result));	
		F.OutNext	("  collider:  %2.2fms", ph_collision.result);	
		F.OutNext	("  solver:    %2.2fms, %d",ph_core.result,ph_core.count);	
		F.OutNext	("aiThink:     %2.2fms, %d",AI_Think.result,AI_Think.count);	
		F.OutNext	("  aiRange:   %2.2fms, %d",AI_Range.result,AI_Range.count);
		F.OutNext	("  aiPath:    %2.2fms, %d",AI_Path.result,AI_Path.count);
		F.OutNext	("  aiNode:    %2.2fms, %d",AI_Node.result,AI_Node.count);
		F.OutNext	("aiVision:    %2.2fms, %d",AI_Vis.result,AI_Vis.count);
		F.OutNext	("  Query:     %2.2fms",	AI_Vis_Query.result);
		F.OutNext	("  RayCast:   %2.2fms",	AI_Vis_RayTests.result);
		F.OutSkip	();
								   
#undef  PPP
#define PPP(a) (100.f*float(a)/float(RenderTOTAL.result))
		F.OutNext	("*** RENDER:  %2.2fms",RenderTOTAL.result);
		F.OutNext	("R_CALC:      %2.2fms, %2.1f%%",RenderCALC.result,	PPP(RenderCALC.result));	
		F.OutNext	("  HOM:       %2.2fms, %d",RenderCALC_HOM.result,	RenderCALC_HOM.count);
		F.OutNext	("  Skeletons: %2.2fms, %d",Animation.result,		Animation.count);
		F.OutNext	("R_DUMP:      %2.2fms, %2.1f%%",RenderDUMP.result,	PPP(RenderDUMP.result));	
		F.OutNext	("  Wait-L:    %2.2fms",RenderDUMP_Wait.result);	
		F.OutNext	("  Wait-S:    %2.2fms",RenderDUMP_Wait_S.result);	
		F.OutNext	("  Skinning:  %2.2fms",RenderDUMP_SKIN.result);	
		F.OutNext	("  DT_Vis/Cnt:%2.2fms/%d",RenderDUMP_DT_VIS.result,RenderDUMP_DT_Count);	
		F.OutNext	("  DT_Render: %2.2fms",RenderDUMP_DT_Render.result);	
		F.OutNext	("  DT_Cache:  %2.2fms",RenderDUMP_DT_Cache.result);	
		F.OutNext	("  Wallmarks: %2.2fms, %d/%d - %d",RenderDUMP_WM.result,RenderDUMP_WMS_Count,RenderDUMP_WMD_Count,RenderDUMP_WMT_Count);
		F.OutNext	("  Glows:     %2.2fms",RenderDUMP_Glows.result);	
		F.OutNext	("  Lights:    %2.2fms, %d",RenderDUMP_Lights.result,RenderDUMP_Lights.count);
		F.OutNext	("  RT:        %2.2fms, %d",RenderDUMP_RT.result,RenderDUMP_RT.count);
		F.OutNext	("  HUD:       %2.2fms",RenderDUMP_HUD.result);	
		F.OutNext	("  P_calc:    %2.2fms",RenderDUMP_Pcalc.result);
		F.OutNext	("  S_calc:    %2.2fms",RenderDUMP_Scalc.result);
		F.OutNext	("  S_render:  %2.2fms, %d",RenderDUMP_Srender.result,RenderDUMP_Srender.count);
		F.OutSkip	();
		F.OutNext	("*** SOUND:   %2.2fms",Sound.result);
		F.OutNext	("  TGT/SIM/E: %d/%d/%d",  snd_stat._rendered, snd_stat._simulated, snd_stat._events);
		F.OutNext	("  HIT/MISS:  %d/%d",  snd_stat._cache_hits, snd_stat._cache_misses);
		F.OutSkip	();
		F.OutNext	("Input:       %2.2fms",Input.result);
		F.OutNext	("clRAY:       %2.2fms, %d, %2.0fK",clRAY.result,		clRAY.count,r_ps);
		F.OutNext	("clBOX:       %2.2fms, %d, %2.0fK",clBOX.result,		clBOX.count,b_ps);
		F.OutNext	("clFRUSTUM:   %2.2fms, %d",		clFRUSTUM.result,	clFRUSTUM.count	);
		F.OutSkip	();
		F.OutNext	("netClientRecv:   %2.2fms, %d",	netClient1.result,netClient1.count);
		F.OutNext	("netClientSend:   %2.2fms, %d",	netClient2.result,netClient2.count);
		F.OutNext	("netServer:   %2.2fms, %d",		netServer.result,netServer.count);
		F.OutNext	("netClientCompressor:   %2.2fms",	netClientCompressor.result);
		F.OutNext	("netServerCompressor:   %2.2fms",	netServerCompressor.result);
		
		F.OutSkip	();

		F.OutSkip	();
		F.OutNext	("TEST 0:      %2.2fms, %d",TEST0.result,TEST0.count);
		F.OutNext	("TEST 1:      %2.2fms, %d",TEST1.result,TEST1.count);
		F.OutNext	("TEST 2:      %2.2fms, %d",TEST2.result,TEST2.count);
		F.OutNext	("TEST 3:      %2.2fms, %d",TEST3.result,TEST3.count);
#ifdef DEBUG_MEMORY_MANAGER
		F.OutSkip	();
		F.OutNext	("str: cmp[%3d], dock[%3d], qpc[%3d]",Memory.stat_strcmp,Memory.stat_strdock,CPU::qpc_counter);
		Memory.stat_strcmp	=	0		;
		Memory.stat_strdock	=	0		;
		CPU::qpc_counter	=	0		;
#else // DEBUG_MEMORY_MANAGER
		F.OutSkip	();
		F.OutNext	("qpc[%3d]",CPU::qpc_counter);
		CPU::qpc_counter	=	0		;
#endif // DEBUG_MEMORY_MANAGER
//		F.OutSet	(640,0);
		F.OutSkip	();
		m_pRender->OutData4(F);
		/*
		F.OutNext	("static:        %3.1f/%d",	RCache.stat.r.s_static.verts/1024.f,		RCache.stat.r.s_static.dips );
		F.OutNext	("flora:         %3.1f/%d",	RCache.stat.r.s_flora.verts/1024.f,			RCache.stat.r.s_flora.dips );
		F.OutNext	("  flora_lods:  %3.1f/%d",	RCache.stat.r.s_flora_lods.verts/1024.f,	RCache.stat.r.s_flora_lods.dips );
		F.OutNext	("dynamic:       %3.1f/%d",	RCache.stat.r.s_dynamic.verts/1024.f,		RCache.stat.r.s_dynamic.dips );
		F.OutNext	("  dynamic_sw:  %3.1f/%d",	RCache.stat.r.s_dynamic_sw.verts/1024.f,	RCache.stat.r.s_dynamic_sw.dips );
		F.OutNext	("  dynamic_inst:%3.1f/%d",	RCache.stat.r.s_dynamic_inst.verts/1024.f,	RCache.stat.r.s_dynamic_inst.dips );
		F.OutNext	("  dynamic_1B:  %3.1f/%d",	RCache.stat.r.s_dynamic_1B.verts/1024.f,	RCache.stat.r.s_dynamic_1B.dips );
		F.OutNext	("  dynamic_2B:  %3.1f/%d",	RCache.stat.r.s_dynamic_2B.verts/1024.f,	RCache.stat.r.s_dynamic_2B.dips );
		F.OutNext	("details:       %3.1f/%d",	RCache.stat.r.s_details.verts/1024.f,		RCache.stat.r.s_details.dips );
*/
		//////////////////////////////////////////////////////////////////////////
		// Renderer specific
		F.SetHeightI						(f_base_size);
		F.OutSet						(200,0);
		Render->Statistics				(&F);

		//////////////////////////////////////////////////////////////////////////
		// Game specific
		F.SetHeightI						(f_base_size);
		F.OutSet						(400,0);
		g_pGamePersistent->Statistics	(&F);

		//////////////////////////////////////////////////////////////////////////
		// process PURE STATS
		F.SetHeightI						(f_base_size);
		seqStats.Process				(rp_Stats);
		pFont->OnRender					();
	};

	if( /*psDeviceFlags.test(rsStatistic) ||*/ psDeviceFlags.test(rsCameraPos) ){
		_draw_cam_pos					(pFont);
		pFont->OnRender					();
	};
#ifdef DEBUG
	//////////////////////////////////////////////////////////////////////////
	// PERF ALERT
	if (!g_bDisableRedText)
	{
		CGameFont&	F = *((CGameFont*)pFont);
		F.SetColor						(color_rgba(255,16,16,255));
		F.OutSet						(300,300);
		F.SetHeightI						(f_base_size*2);
		if (fFPS<30)					F.OutNext	("FPS       < 30:   %3.1f",	fFPS);
		//if (RCache.stat.verts>500000)	F.OutNext	("Verts     > 500k: %d",	RCache.stat.verts);
		m_pRender->GuardVerts(F);
		////if (RCache.stat.polys>500000)	F.OutNext	("Polys     > 500k: %d",	RCache.stat.polys);
		if (psDeviceFlags.test(rsStatistic))
		{
			m_pRender->GuardDrawCalls(F);
			//if (RCache.stat.calls>1000)		F.OutNext	("DIP/DP    > 1k:   %d",	RCache.stat.calls);
			////if (RCache.stat.textures>1000)F.OutNext	("T_change  > 500:  %d",	RCache.stat.textures);
			if (RenderDUMP_DT_Count>1000)	F.OutNext	("DT_count  > 1000: %u",	RenderDUMP_DT_Count);
			F.OutSkip						();
			//if (fMem_calls>1500)			F.OutNext	("MMGR calls > 1500:%3.1f",	fMem_calls);
			if (Sheduler.result>3.f)		F.OutNext	("Update     > 3ms:	%3.1f",	Sheduler.result);
			if (UpdateClient.result>3.f)	F.OutNext	("UpdateCL   > 3ms: %3.1f",	UpdateClient.result);
			if (Physics.result>5.f)			F.OutNext	("Physics    > 5ms: %3.1f",	Physics.result);	
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Show errors
	if (!g_bDisableRedText && errors.size())
	{
		CGameFont&	F = *((CGameFont*)pFont);
		F.SetColor	(color_rgba(255,16,16,191));
		F.OutSet	(200,0);
		F.SetHeightI	(f_base_size);
#if 0
		for (u32 it=0; it<errors.size(); it++)
			F.OutNext("%s",errors[it].c_str());
#else
		for (u32 it=(u32)_max(int(0),(int)errors.size() - g_ErrorLineCount); it<errors.size(); it++)
			F.OutNext("%s",errors[it].c_str());
#endif
		F.OnRender	();
	}
#endif

	{
		EngineTOTAL.FrameStart		();	
		Sheduler.FrameStart			();	
		UpdateClient.FrameStart		();	
		Physics.FrameStart			();	
		ph_collision.FrameStart		();
		ph_core.FrameStart			();
		Animation.FrameStart		();	
		AI_Think.FrameStart			();
		AI_Range.FrameStart			();
		AI_Path.FrameStart			();
		AI_Node.FrameStart			();
		AI_Vis.FrameStart			();
		AI_Vis_Query.FrameStart		();
		AI_Vis_RayTests.FrameStart	();
		
		RenderTOTAL.FrameStart		();
		RenderCALC.FrameStart		();
		RenderCALC_HOM.FrameStart	();
		RenderDUMP.FrameStart		();	
		RenderDUMP_RT.FrameStart	();
		RenderDUMP_SKIN.FrameStart	();	
		RenderDUMP_Wait.FrameStart	();	
		RenderDUMP_Wait_S.FrameStart();	
		RenderDUMP_HUD.FrameStart	();	
		RenderDUMP_Glows.FrameStart	();	
		RenderDUMP_Lights.FrameStart();	
		RenderDUMP_WM.FrameStart	();	
		RenderDUMP_DT_VIS.FrameStart();	
		RenderDUMP_DT_Render.FrameStart();	
		RenderDUMP_DT_Cache.FrameStart();	
		RenderDUMP_Pcalc.FrameStart	();	
		RenderDUMP_Scalc.FrameStart	();	
		RenderDUMP_Srender.FrameStart();	
		
		Sound.FrameStart			();
		Input.FrameStart			();
		clRAY.FrameStart			();	
		clBOX.FrameStart			();
		clFRUSTUM.FrameStart		();
		
		netClient1.FrameStart		();
		netClient2.FrameStart		();
		netServer.FrameStart		();
		netClientCompressor.FrameStart();
		netServerCompressor.FrameStart();

		TEST0.FrameStart			();
		TEST1.FrameStart			();
		TEST2.FrameStart			();
		TEST3.FrameStart			();

		g_SpatialSpace->stat_insert.FrameStart		();
		g_SpatialSpace->stat_remove.FrameStart		();

		g_SpatialSpacePhysic->stat_insert.FrameStart();
		g_SpatialSpacePhysic->stat_remove.FrameStart();
	}
	dwSND_Played = dwSND_Allocated = 0;
	Particles_starting = Particles_active = Particles_destroy = 0;
}

void	_LogCallback				(LPCSTR string)
{
	if (string && '!'==string[0] && ' '==string[1])
		Device.Statistic->errors.push_back	(shared_str(string));
}

void CStats::OnDeviceCreate			()
{
	g_bDisableRedText				= strstr(Core.Params,"-xclsx")?TRUE:FALSE;

//	if (!strstr(Core.Params, "-dedicated"))
#ifndef DEDICATED_SERVER
	pFont	= xr_new<CGameFont>		("stat_font", CGameFont::fsDeviceIndependent);
#endif
	
	if(!pSettings->section_exist("evaluation")
		||!pSettings->line_exist("evaluation","line1")
		||!pSettings->line_exist("evaluation","line2")
		||!pSettings->line_exist("evaluation","line3") )
		FATAL	("");

	eval_line_1 = pSettings->r_string_wb("evaluation","line1");
	eval_line_2 = pSettings->r_string_wb("evaluation","line2");
	eval_line_3 = pSettings->r_string_wb("evaluation","line3");

	// 
#ifdef DEBUG
	if (!g_bDisableRedText)			SetLogCB	(_LogCallback);
#endif
}

void CStats::OnDeviceDestroy		()
{
	SetLogCB(0);
	xr_delete	(pFont);
}

void CStats::OnRender				()
{
#ifdef DEBUG
	if (g_stats_flags.is(st_sound)){
		CSound_stats_ext				snd_stat_ext;
		::Sound->statistic				(0,&snd_stat_ext);
		CSound_stats_ext::item_vec_it	_I = snd_stat_ext.items.begin();
		CSound_stats_ext::item_vec_it	_E = snd_stat_ext.items.end();
		for (;_I!=_E;_I++){
			const CSound_stats_ext::SItem& item = *_I;
			if (item._3D)
			{
				m_pRender->SetDrawParams(&*Device.m_pRender);
				//RCache.set_xform_world(Fidentity);
				//RCache.set_Shader		(Device.m_SelectionShader);
				//RCache.set_c			("tfactor",1,1,1,1);
				DU->DrawCross			(item.params.position, 0.5f, 0xFF0000FF, true );
				if (g_stats_flags.is(st_sound_min_dist))
					DU->DrawSphere		(Fidentity, item.params.position, item.params.min_distance, 0x400000FF,	0xFF0000FF, true, true);
				if (g_stats_flags.is(st_sound_max_dist))
					DU->DrawSphere		(Fidentity, item.params.position, item.params.max_distance, 0x4000FF00,	0xFF008000, true, true);
				
				xr_string out_txt		= (out_txt.size() && g_stats_flags.is(st_sound_info_name)) ? item.name.c_str():"";

				if (item.game_object)
				{
					if (g_stats_flags.is(st_sound_ai_dist))
						DU->DrawSphere	(Fidentity, item.params.position, item.params.max_ai_distance, 0x80FF0000,0xFF800000,true,true);
					if (g_stats_flags.is(st_sound_info_object)){
						out_txt			+= "  (";
						out_txt			+= item.game_object->cNameSect().c_str();
						out_txt			+= ")";
					}
				}
				if (g_stats_flags.is_any(st_sound_info_name|st_sound_info_object) && item.name.size())
					DU->OutText			(item.params.position, out_txt.c_str(),0xFFFFFFFF,0xFF000000);
			}
		}
	}
#endif
}
