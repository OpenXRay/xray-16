#include "stdafx.h"
#pragma hdrstop

#include "EStats.h"
#include "hw.h"
#include "gamefont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEStats::CEStats()
{
	fFPS		= 30.f;
	fRFPS		= 30.f;
	fTPS		= 0;
	dwLevelSelFaceCount	= 0;
	dwLevelSelVertexCount=0;
}

CEStats::~CEStats()
{

}

#include "igame_persistent.h"
void CEStats::Show(CGameFont* font)
{
	// Stop timers
	{
		RenderTOTAL.FrameEnd		();
		RenderCALC.FrameEnd			();

        RenderDUMP_SKIN.FrameEnd	();

		Animation.FrameEnd			();
		Input.FrameEnd				();
		clRAY.FrameEnd				();
		clBOX.FrameEnd				();
        clFRUSTUM.FrameEnd			();

        RenderDUMP_RT.FrameEnd		();

		RenderDUMP_DT_VIS.FrameEnd	();
		RenderDUMP_DT_Render.FrameEnd();
		RenderDUMP_DT_Cache.FrameEnd();
        TEST0.FrameEnd				();
        TEST1.FrameEnd				();
        TEST2.FrameEnd				();
        TEST3.FrameEnd				();
	}

	// calc FPS & TPS
    CBackend::_stats& DPS = RCache.stat;
	if (EDevice.fTimeDelta>EPS_S) {
		float fps  = 1.f/EDevice.fTimeDelta;
		float fOne = 0.3f;
		float fInv = 1.f-fOne;
		fFPS = fInv*fFPS + fOne*fps;

		if (RenderTOTAL.result>EPS_S) {
			fTPS = fInv*fTPS + fOne*float(DPS.polys)/(RenderTOTAL.result*1000.f);
			fRFPS= fInv*fRFPS+ fOne*1000.f/RenderTOTAL.result;
		}
	}

	// Show them
	if (psDeviceFlags.is(rsStatistic))
	{
	    CGameFont& 	F = *font;
		F.SetColor	(0xFFFFFFFF	);
		F.OutSet	(5,5);
		F.OutNext	("FPS/RFPS:     %3.1f/%3.1f",	fFPS,fRFPS);
		F.OutNext	("TPS:          %2.2f M",		fTPS);
		F.OutNext	("VERT:         %d",			DPS.verts);
		F.OutNext	("POLY:         %d",			DPS.polys);
		F.OutNext	("DIP/DP:       %d",			DPS.calls);
		F.OutNext	("SH/T/M/C:     %d/%d/%d/%d",	dwShader_Codes,dwShader_Textures,dwShader_Matrices,dwShader_Constants);
		F.OutNext	("LIGHT S/T:    %d/%d",			dwLightInScene,dwTotalLight);
		F.OutNext	("Skeletons:    %2.2fms, %d",	Animation.result,Animation.count);
		F.OutNext	("Skinning:     %2.2fms",		RenderDUMP_SKIN.result);
		F.OutSkip	();
		F.OutNext	("Input:        %2.2fms",		Input.result);
		F.OutNext	("clRAY:        %2.2fms, %d",	clRAY.result,clRAY.count);
		F.OutNext	("clBOX:        %2.2fms, %d",	clBOX.result,clBOX.count);
        F.OutNext	("clFRUSTUM:    %2.2fms, %d",	clFRUSTUM.result,clFRUSTUM.count);
		F.OutSkip	();
//        F.OutNext	("Render:       %2.2fms",		RenderDUMP.result);
		F.OutNext	(" RT:          %2.2fms, %d",	RenderDUMP_RT.result,RenderDUMP_RT.count);
		F.OutNext	(" DT_Vis:      %2.2fms",		RenderDUMP_DT_VIS.result);
		F.OutNext	(" DT_Render:   %2.2fms",		RenderDUMP_DT_Render.result);
		F.OutNext	(" DT_Cache:    %2.2fms",		RenderDUMP_DT_Cache.result);
		F.OutSkip	();
		F.OutNext	("TEST 0:       %2.2fms, %d",	TEST0.result,TEST0.count);
		F.OutNext	("TEST 1:       %2.2fms, %d",	TEST1.result,TEST1.count);
		F.OutNext	("TEST 2:       %2.2fms, %d",	TEST2.result,TEST2.count);
		F.OutNext	("TEST 3:       %2.2fms, %d",	TEST3.result,TEST3.count);
		F.OutSkip	();
//		F.OutNext	("GAME TIME:    %s",			FloatTimeToStrTime(g_pGamePersistent->Environment().GetGameTime()).c_str());
//		F.OutSkip	(2.f);
//        F.OutNext	("Level summary:");
//        F.OutNext	(" Sel Faces:   %d",			dwLevelSelFaceCount);
//        F.OutNext	(" Sel Verts:   %d",			dwLevelSelVertexCount);
	}

	{
		Animation.FrameStart		();
		RenderTOTAL.FrameStart		();

		Input.FrameStart			();
		clRAY.FrameStart			();
		clBOX.FrameStart			();
		clFRUSTUM.FrameStart		();

		RenderDUMP_SKIN.FrameStart	();
		RenderDUMP_RT.FrameStart	();

		RenderDUMP_DT_VIS.FrameStart();
		RenderDUMP_DT_Render.FrameStart();
		RenderDUMP_DT_Cache.FrameStart();

		TEST0.FrameStart			();
		TEST1.FrameStart			();
		TEST2.FrameStart			();
		TEST3.FrameStart			();
	}
	dwShader_Codes = dwShader_Textures = dwShader_Matrices = dwShader_Constants = 0;
	dwSND_Played = dwSND_Allocated = 0;
    dwTotalLight = dwLightInScene = 0;

	DPS.polys	= 0;
	DPS.verts	= 0;
	DPS.calls	= 0;
	DPS.vs		= 0;
//	DPS.vb		= 0;
//	DPS.ib		= 0;

	dwLevelSelFaceCount		= 0;
	dwLevelSelVertexCount	= 0;
}
