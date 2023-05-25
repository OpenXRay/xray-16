//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop
#include "gamefont.h"
#include <sal.h>
#include "ui_main.h"
#include "render.h"
#include "GameMtlLib.h"
#include "ResourceManager.h"
#pragma package(smart_init)

CEditorRenderDevice EDevice;

extern int rsDVB_Size;
extern int rsDIB_Size;

ENGINE_API BOOL g_bRendering = FALSE;

void _BCL CEditorRenderDevice::AddSeqFrame(pureFrame *f, bool mt) { seqFrame.Add(f, REG_PRIORITY_LOW); }
void _BCL CEditorRenderDevice::RemoveSeqFrame(pureFrame *f) { seqFrame.Remove(f); }

//---------------------------------------------------------------------------
CEditorRenderDevice::CEditorRenderDevice()
{
	RadiusRender = 400;
	psDeviceFlags.assign(rsStatistic | rsFilterLinear | rsFog | rsDrawGrid);
	// dynamic buffer size
	rsDVB_Size = 2048;
	rsDIB_Size = 2048;
	// default initialization
	m_ScreenQuality = 1.f;
	dwMaximized = 0;
	dwWidth = dwHeight = 256;
	m_RenderWidth = m_RenderHeight = 256;
	mProject.identity();
	mFullTransform.identity();
	mView.identity();
	m_WireShader = 0;
	m_SelectionShader = 0;

	b_is_Ready = FALSE;
	b_is_Active = FALSE;

	// Engine flow-control
	fTimeDelta = 0;
	fTimeGlobal = 0;
	dwTimeDelta = 0;
	dwTimeGlobal = 0;

	dwFillMode = D3DFILL_SOLID;
	dwShadeMode = D3DSHADE_GOURAUD;

	m_CurrentShader = 0;
	pSystemFont = 0;

	fASPECT = 1.f;
	fFOV = 60.f;
	dwPrecacheFrame = 0;
}

CEditorRenderDevice::~CEditorRenderDevice()
{
	VERIFY(!b_is_Ready);
}

// extern void Surface_Init();
#include "../../../xrAPI/xrAPI.h"
#include "../../../xrRender/Private/dxRenderFactory.h"
void CEditorRenderDevice::Initialize()
{
	//	m_Camera.Reset();

	m_DefaultMat.set(1, 1, 1);
	//	Surface_Init();

	// game materials
	GMLib.Load();

	// compiler shader
	string_path fn;
	FS.update_path(fn, _game_data_, "shaders_xrlc.xr");
	if (FS.exist(fn))
	{
		ShaderXRLC.Load(fn);
	}
	else
	{
		ELog.DlgMsg(mtInformation, "Can't find file '%s'", fn);
	}

	CreateWindow();

	RenderFactory = &RenderFactoryImpl;

	// Startup shaders
	Create();

	::Render->Initialize();

	Resize(EPrefs->start_w, EPrefs->start_h, EPrefs->start_maximized);
	HW.updateWindowProps(m_hWnd);

	::ShowWindow(m_hWnd, EPrefs->start_maximized ? SW_SHOWMAXIMIZED : SW_SHOWDEFAULT);
}

void CEditorRenderDevice::ShutDown()
{
	::Render->ShutDown();

	ShaderXRLC.Unload();
	GMLib.Unload();

	// destroy context
	Destroy();
	xr_delete(pSystemFont);

	// destroy shaders
	//	PSLib.xrShutDown	();
}

void CEditorRenderDevice::InitTimer()
{
	Timer_MM_Delta = 0;
	{
		u32 time_mm = clock();
		while (clock() == time_mm)
			; // wait for next tick
		u32 time_system = clock();
		u32 time_local = TimerAsync();
		Timer_MM_Delta = time_system - time_local;
	}
}
//---------------------------------------------------------------------------
void CEditorRenderDevice::RenderNearer(float n)
{
	mProject._43 = m_fNearer - n;
	RCache.set_xform_project(mProject);
}
void CEditorRenderDevice::ResetNearer()
{
	mProject._43 = m_fNearer;
	RCache.set_xform_project(mProject);
}
//---------------------------------------------------------------------------
bool CEditorRenderDevice::Create()
{
	if (b_is_Ready)
		return false;
	Statistic = xr_new<CEStats>();
	ELog.Msg(mtInformation, "Starting RENDER device...");

	HW.CreateDevice(m_hWnd, true);
	if (UI)
	{
		string_path ini_path;
		string_path ini_name;
		xr_strcpy(ini_name, UI->EditorName());
		xr_strcat(ini_name, "_imgui.ini");
		FS.update_path(ini_path, "$local_root$", ini_name);
		if (!FS.exist(ini_path))
			UI->ResetUI();
		UI->Initialize(m_hWnd, HW.pDevice, ini_path);
	}

	// after creation
	dwFrame = 0;

	string_path sh;
	FS.update_path(sh, _game_data_, "shaders.xr");

	IReader *F = 0;
	if (FS.exist(sh))
		F = FS.r_open(0, sh);
	Resources = xr_new<CResourceManager>();

	// if build options - load textures immediately
	if (strstr(Core.Params, "-build") || strstr(Core.Params, "-ebuild"))
		EDevice.Resources->DeferredLoad(FALSE);

	_Create(F);
	FS.r_close(F);

	ELog.Msg(mtInformation, "D3D: initialized");

	return true;
}

//---------------------------------------------------------------------------
void CEditorRenderDevice::Destroy()
{
	if (!b_is_Ready)
		return;

	ELog.Msg(mtInformation, "Destroying Direct3D...");

	HW.Validate();

	// before destroy
	_Destroy(FALSE);
	xr_delete(Resources);

	UI->Destroy();
	// real destroy
	HW.DestroyDevice();

	ELog.Msg(mtInformation, "D3D: device cleared");
	xr_delete(Statistic);
}
//---------------------------------------------------------------------------
void CEditorRenderDevice::_SetupStates()
{
	HW.Caps.Update();
	for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
	{
		float fBias = -1.f;
		CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)(&fBias))));
	}
	EDevice.SetRS(D3DRS_DITHERENABLE, TRUE);
	EDevice.SetRS(D3DRS_COLORVERTEX, TRUE);
	EDevice.SetRS(D3DRS_STENCILENABLE, FALSE);
	EDevice.SetRS(D3DRS_ZENABLE, TRUE);
	EDevice.SetRS(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	EDevice.SetRS(D3DRS_CULLMODE, D3DCULL_CCW);
	EDevice.SetRS(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	EDevice.SetRS(D3DRS_LOCALVIEWER, TRUE);
	EDevice.SetRS(D3DRS_NORMALIZENORMALS, TRUE);

	EDevice.SetRS(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
	EDevice.SetRS(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
	EDevice.SetRS(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	EDevice.SetRS(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);

	ResetMaterial();
}
//---------------------------------------------------------------------------
void CEditorRenderDevice::_Create(IReader *F)
{
	b_is_Ready = TRUE;

	// General Render States
	_SetupStates();

	RCache.OnDeviceCreate();
	Resources->OnDeviceCreate(F);
	::Render->OnDeviceCreate();

	m_WireShader.create("editor\\wire");
	m_SelectionShader.create("editor\\selection");

	// signal another objects
	UI->OnDeviceCreate();
	//.	seqDevCreate.Process		(rp_DeviceCreate);

	pSystemFont = xr_new<CGameFont>("hud_font_small");
	//	pSystemFont					= xr_new<CGameFont>("hud_font_medium");
}

void CEditorRenderDevice::_Destroy(BOOL bKeepTextures)
{
	xr_delete(pSystemFont);

	b_is_Ready = FALSE;
	m_CurrentShader = 0;

	UI->OnDeviceDestroy();

	m_WireShader.destroy();
	m_SelectionShader.destroy();

	//.	seqDevDestroy.Process		(rp_DeviceDestroy);

	::Render->Models->OnDeviceDestroy();

	Resources->OnDeviceDestroy(bKeepTextures);

	RCache.OnDeviceDestroy();
	::Render->OnDeviceDestroy();
}

//---------------------------------------------------------------------------
void CEditorRenderDevice::Resize(int w, int h, bool maximized)
{
	if (dwWidth == w && dwHeight == h && dwMaximized == maximized)
		return;
	m_RenderArea = w * h;

	dwWidth = w;
	dwHeight = h;
	dwMaximized = maximized;

	Reset();
	UI->RedrawScene();
}

void CEditorRenderDevice::Reset()
{
	u32 tm_start = TimerAsync();
	Resources->reset_begin();
	UI->ResetBegin();
	Memory.mem_compact();
	HW.DevPP.BackBufferWidth = dwWidth;
	HW.DevPP.BackBufferHeight = dwHeight;
	HW.Reset(m_hWnd);
	dwWidth = HW.DevPP.BackBufferWidth;
	dwHeight = HW.DevPP.BackBufferHeight;
	//		fWidth_2			= float(dwWidth/2);
	//		fHeight_2			= float(dwHeight/2);
	Resources->reset_end();
	UI->ResetEnd();
	_SetupStates();
	u32 tm_end = TimerAsync();
	Msg("*** RESET [%d ms]", tm_end - tm_start);
}

BOOL CEditorRenderDevice::Begin()
{
	VERIFY(b_is_Ready);
	mFullTransform_saved = mFullTransform;
	mProject_saved = mProject;
	mView = mView_saved;
	vCameraPosition_saved = vCameraPosition;
	HW.Validate();
	HRESULT _hr = HW.pDevice->TestCooperativeLevel();
	if (FAILED(_hr))
	{
		// If the device was lost, do not render until we get it back
		if (D3DERR_DEVICELOST == _hr)
		{
			Sleep(33);
			return FALSE;
		}

		// Check if the device is ready to be reset
		if (D3DERR_DEVICENOTRESET == _hr)
		{
			Reset();
		}
	}

	VERIFY(FALSE == g_bRendering);
	HW.pDevice->BeginScene(); //CHK_DX(HW.pDevice->BeginScene());
	CHK_DX(HW.pDevice->Clear(0, 0,
							 D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET |
								 (HW.Caps.bStencil ? D3DCLEAR_STENCIL : 0),
							 EPrefs ? EPrefs->scene_clear_color : 0x0, 1, 0));
	RCache.OnFrameBegin();
	g_bRendering = TRUE;
	return TRUE;
}

void CEditorRenderDevice::End()
{
	VERIFY(HW.pDevice);
	VERIFY(b_is_Ready);
	g_bRendering = FALSE;
	// end scene
	RCache.OnFrameEnd();

	#pragma TODO("TSMP: restore CHK_DX!")
	// TSMP: when calling MakeScreenshot by user click on button, we are drawing imgui in render window
	// so we call twice BeginScene and EndScene, and they return error. Need to reimplement this and
	// restore CHK_DX for BeginScene and EndScene
	HW.pDevice->EndScene(); //CHK_DX(HW.pDevice->EndScene());

	CHK_DX(HW.pDevice->Present(NULL, NULL, NULL, NULL));
}

void CEditorRenderDevice::UpdateView()
{
	// set camera matrix
	m_Camera.GetView(mView);

	RCache.set_xform_view(mView);
	mFullTransform.mul(mProject, mView);

	// frustum culling sets
	::Render->ViewBase.CreateFromMatrix(mFullTransform, FRUSTUM_P_ALL);
}

void CEditorRenderDevice::FrameMove()
{
	dwFrame++;

	// Timer
	float fPreviousFrameTime = Timer.GetElapsed_sec();
	Timer.Start();												// previous frame
	fTimeDelta = 0.1f * fTimeDelta + 0.9f * fPreviousFrameTime; // smooth random system activity - worst case ~7% error
	if (fTimeDelta > .1f)
		fTimeDelta = .1f; // limit to 15fps minimum

	fTimeGlobal = TimerGlobal.GetElapsed_sec(); // float(qTime)*CPU::cycles2seconds;
	dwTimeGlobal = TimerGlobal.GetElapsed_ms(); // u32((qTime*u64(1000))/CPU::cycles_per_second);
	dwTimeDelta = iFloor(fTimeDelta * 1000.f + 0.5f);
	dwTimeContinual = dwTimeGlobal;

	m_Camera.Update(fTimeDelta);

	// process objects
	seqFrame.Process(rp_Frame);
}

void CEditorRenderDevice::DP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 vBase, u32 pc)
{
	ref_shader S = m_CurrentShader ? m_CurrentShader : m_WireShader;
	u32 dwRequired = S->E[0]->passes.size();
	RCache.set_Geometry(geom);
	for (u32 dwPass = 0; dwPass < dwRequired; dwPass++)
	{
		RCache.set_Shader(S, dwPass);
		RCache.Render(pt, vBase, pc);
	}
}

void CEditorRenderDevice::DIP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
	ref_shader S = m_CurrentShader ? m_CurrentShader : m_WireShader;
	u32 dwRequired = S->E[0]->passes.size();
	RCache.set_Geometry(geom);
	for (u32 dwPass = 0; dwPass < dwRequired; dwPass++)
	{
		RCache.set_Shader(S, dwPass);
		RCache.Render(pt, baseV, startV, countV, startI, PC);
	}
}

void CEditorRenderDevice::ReloadTextures()
{
	UI->SetStatus("Reload textures...");
	Resources->ED_UpdateTextures(0);
	UI->SetStatus("");
}

void CEditorRenderDevice::UnloadTextures()
{
#ifndef _EDITOR
	Resources->DeferredUnload();
#endif
}

void CEditorRenderDevice::Reset(IReader *F, BOOL bKeepTextures)
{
	CTimer tm;
	tm.Start();
	_Destroy(bKeepTextures);
	_Create(F);
	Msg("*** RESET [%d ms]", tm.GetElapsed_ms());
}

void CEditorRenderDevice::time_factor(float v)
{
	Timer.time_factor(v);
	TimerGlobal.time_factor(v);
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void CEditorRenderDevice::CreateWindow()
{
	m_WC = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), UI->EditorIcon() , NULL, NULL, NULL, TEXT("X-Ray editor") , NULL};
	::RegisterClassEx(&m_WC);
	m_hWnd = ::CreateWindowA(m_WC.lpszClassName, UI->EditorDesc(), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, m_WC.hInstance, NULL);

	::UpdateWindow(m_hWnd);
}
void CEditorRenderDevice::DestryWindow()
{
	::DestroyWindow(m_hWnd);
	::UnregisterClass(m_WC.lpszClassName, m_WC.hInstance);
}
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
	{
		u16 fActive = LOWORD(wParam);
		BOOL fMinimized = (BOOL)HIWORD(wParam);
		BOOL bActive = ((fActive != WA_INACTIVE) && (!fMinimized)) ? TRUE : FALSE;
		if (bActive != EDevice.b_is_Active)
		{
			EDevice.b_is_Active = bActive;

			if (EDevice.b_is_Active)
			{
				if (UI)
					UI->OnAppActivate();
			}
			else
			{

				if (UI)
					UI->OnAppDeactivate();
			}
		}
	}
	break;
	}
	if (UI && UI->WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (UI)
			UI->KeyDown(wParam, UI->GetShiftState());
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (UI)
			UI->KeyUp(wParam, UI->GetShiftState());
		break;

	case WM_SIZE:

		if (UI && HW.pDevice)
		{
			UI->Resize(LOWORD(lParam), HIWORD(lParam), wParam == SIZE_MAXIMIZED);
		}
		/*if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = ;
			g_d3dpp.BackBufferHeight = ;
			ResetDevice();
		}*/
		return 0;

	case WM_SYSCOMMAND:

		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
		{
			return 0;
		}
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
