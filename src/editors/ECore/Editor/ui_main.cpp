//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "xr_input.h"
#include "UI_ToolsCustom.h"

#include "UI_Main.h"
#include "d3dutils.h"
#include "SoundManager.h"
#include "PSLibrary.h"

#include "ImageEditor.h"
#include "SoundEditor.h"
#include "ChoseForm.h"
#include "ETools.h"

TUI* 	UI			= 0;

TUI::TUI()
{
	UI				= this;
    m_bAppActive 	= false;
	m_bReady 		= false;
    m_D3DWindow 	= 0;
    m_D3DPanel		= 0;
    bNeedAbort   	= false;

	m_CurrentRStart.set(0,0,0);
	m_CurrentRDir.set(0,0,0);

	m_Flags.assign	(flResize);

	m_Pivot.set		( 0, 0, 0 );

	m_MouseCaptured = false;
    m_MouseMultiClickCaptured = false;
 	m_SelectionRect = false;
    bMouseInUse		= false;

    m_bHintShowing	= false;
	m_pHintWindow	= 0;
	m_LastHint		= "";
}
//---------------------------------------------------------------------------
TUI::~TUI()
{
	VERIFY(m_ProgressItems.size()==0);
    VERIFY(m_EditorState.size()==0);
}

void TUI::OnDeviceCreate()
{
	DU_impl.OnDeviceCreate();
}

void TUI::OnDeviceDestroy()
{
	DU_impl.OnDeviceDestroy();
}

bool TUI::IsModified()
{
	return ExecCommand(COMMAND_CHECK_MODIFIED);
}
//---------------------------------------------------------------------------

void TUI::EnableSelectionRect( bool flag ){
	m_SelectionRect = flag;
	m_SelEnd.x = m_SelStart.x = 0;
	m_SelEnd.y = m_SelStart.y = 0;
}

void TUI::UpdateSelectionRect( const Ivector2& from, const Ivector2& to ){
	m_SelStart.set(from);
	m_SelEnd.set(to);
}

bool __fastcall TUI::KeyDown (WORD Key, TShiftState Shift)
{
	if (!m_bReady) return false;
//	m_ShiftState = Shift;
//	Log("Dn  ",Shift.Contains(ssShift)?"1":"0");
	if (EDevice.m_Camera.KeyDown(Key,Shift)) return true;
    return Tools->KeyDown(Key, Shift);
}

bool __fastcall TUI::KeyUp   (WORD Key, TShiftState Shift)
{
	if (!m_bReady) return false;
//	m_ShiftState = Shift;
	if (EDevice.m_Camera.KeyUp(Key,Shift)) return true;
    return Tools->KeyUp(Key, Shift);
}

bool __fastcall TUI::KeyPress(WORD Key, TShiftState Shift)
{
	if (!m_bReady) return false;
    return Tools->KeyPress(Key, Shift);
}
//----------------------------------------------------

void TUI::MousePress(TShiftState Shift, int X, int Y)
{
	if (!m_bReady) return;
    if (m_MouseCaptured) return;

    bMouseInUse = true;

    m_ShiftState = Shift;

    // camera activate
    if(!EDevice.m_Camera.MoveStart(m_ShiftState)){
    	if (Tools->Pick(Shift)) return;
        if( !m_MouseCaptured ){
            if( Tools->HiddenMode() ){
				IR_GetMousePosScreen(m_StartCpH);
                m_DeltaCpH.set(0,0);
            }else{
                IR_GetMousePosReal(EDevice.m_hRenderWnd, m_CurrentCp);
                m_StartCp = m_CurrentCp;
                EDevice.m_Camera.MouseRayFromPoint(m_CurrentRStart, m_CurrentRDir, m_CurrentCp );
            }

            if(Tools->MouseStart(m_ShiftState)){
                if(Tools->HiddenMode()) ShowCursor( FALSE );
                m_MouseCaptured = true;
            }
        }
    }
    RedrawScene();
}

void TUI::MouseRelease(TShiftState Shift, int X, int Y)
{
	if (!m_bReady) return;

    m_ShiftState = Shift;

    if( EDevice.m_Camera.IsMoving() ){
        if (EDevice.m_Camera.MoveEnd(m_ShiftState)) bMouseInUse = false;
    }else{
	    bMouseInUse = false;
        if( m_MouseCaptured ){
            if( !Tools->HiddenMode() ){
                IR_GetMousePosReal(EDevice.m_hRenderWnd, m_CurrentCp);
                EDevice.m_Camera.MouseRayFromPoint(m_CurrentRStart,m_CurrentRDir,m_CurrentCp );
            }
            if( Tools->MouseEnd(m_ShiftState) ){
                if( Tools->HiddenMode() ){
                    SetCursorPos(m_StartCpH.x,m_StartCpH.y);
                    ShowCursor( TRUE );
                }
                m_MouseCaptured = false;
            }
        }
    }
    // update tools (change action)
    Tools->OnFrame	();
    RedrawScene		();
}
//----------------------------------------------------
void TUI::MouseMove(TShiftState Shift, int X, int Y)
{
	if (!m_bReady) return;
    m_ShiftState = Shift;
}
//----------------------------------------------------
void TUI::IR_OnMouseMove(int x, int y){
	if (!m_bReady) return;
    bool bRayUpdated = false;

	if (!EDevice.m_Camera.Process(m_ShiftState,x,y)){
        if( m_MouseCaptured || m_MouseMultiClickCaptured ){
            if( Tools->HiddenMode() ){
				m_DeltaCpH.set(x,y);
                if( m_DeltaCpH.x || m_DeltaCpH.y ){
                	Tools->MouseMove(m_ShiftState);
                }
            }else{
                IR_GetMousePosReal(EDevice.m_hRenderWnd, m_CurrentCp);
                EDevice.m_Camera.MouseRayFromPoint(m_CurrentRStart,m_CurrentRDir,m_CurrentCp);
                Tools->MouseMove(m_ShiftState);
            }
		    RedrawScene();
            bRayUpdated = true;
        }
    }
    if (!bRayUpdated){
		IR_GetMousePosReal(EDevice.m_hRenderWnd, m_CurrentCp);
        EDevice.m_Camera.MouseRayFromPoint(m_CurrentRStart,m_CurrentRDir,m_CurrentCp);
    }
    // Out cursor pos
    OutUICursorPos	();
}
//---------------------------------------------------------------------------

void TUI::OnAppActivate()
{
	VERIFY(m_bReady);
	if (pInput){
        m_ShiftState.Clear();
     	pInput->OnAppActivate();
        EDevice.seqAppActivate.Process	(rp_AppActivate);
    }
    m_bAppActive 	= true;
}
//---------------------------------------------------------------------------

void TUI::OnAppDeactivate()
{
	VERIFY(m_bReady);
	if (pInput){
		pInput->OnAppDeactivate();
        m_ShiftState.Clear();
        EDevice.seqAppDeactivate.Process(rp_AppDeactivate);
    }
    HideHint();
    m_bAppActive 	= false;
}
//---------------------------------------------------------------------------

bool TUI::ShowHint(const AStringVec& SS)
{
	VERIFY(m_bReady);
    if (SS.size()){
        AnsiString S=_ListToSequence2(SS);
        if (m_bHintShowing&&(S==m_LastHint)) return true;
        m_LastHint = S;
        m_bHintShowing = true;
        if (!m_pHintWindow){
            m_pHintWindow = xr_new<THintWindow>((TComponent*)0);
            m_pHintWindow->Brush->Color = (TColor)0x0d9F2FF;
        }
        TRect rect = m_pHintWindow->CalcHintRect(320,S,0);
        rect.Left+=m_HintPoint.x;    rect.Top+=m_HintPoint.y;
        rect.Right+=m_HintPoint.x;   rect.Bottom+=m_HintPoint.y;
        m_pHintWindow->ActivateHint(rect,S);
    }else{
    	m_bHintShowing = false;
        m_LastHint = "";
    }
    return m_bHintShowing;
}
//---------------------------------------------------------------------------

void TUI::HideHint()
{
	VERIFY(m_bReady);
    m_bHintShowing = false;
    xr_delete(m_pHintWindow);
}
//---------------------------------------------------------------------------

void TUI::ShowHint(const AnsiString& s)
{
	VERIFY			(m_bReady);
    GetCursorPos	(&m_HintPoint);
	AStringVec 		SS;
    SS.push_back	(s);
	Tools->OnShowHint(SS);
    if (!ShowHint(SS)&&m_pHintWindow) HideHint();
}
//---------------------------------------------------------------------------

void TUI::ShowObjectHint()
{
	VERIFY(m_bReady);
    if (!EPrefs->object_flags.is(epoShowHint)){
//    	if (m_bHintShowing) HideHint();
    	return;
    }
    if (EDevice.m_Camera.IsMoving()||m_MouseCaptured) return;
    if (!m_bAppActive) return;

    GetCursorPos(&m_HintPoint);
    TWinControl* ctr = FindVCLWindow(m_HintPoint);
    if (ctr!=m_D3DWindow) return;

	AStringVec SS;
	Tools->OnShowHint(SS);
    if (!ShowHint(SS)&&m_pHintWindow) HideHint();
}
//---------------------------------------------------------------------------
void TUI::CheckWindowPos(TForm* form)
{
	if (form->Left+form->Width>Screen->Width) 	form->Left	= Screen->Width-form->Width;
	if (form->Top+form->Height>Screen->Height)	form->Top 	= Screen->Height-form->Height;
	if (form->Left<0) 							form->Left	= 0;
	if (form->Top<0) 							form->Top 	= 0;
}
//---------------------------------------------------------------------------
#include "igame_persistent.h"
#include "environment.h"

void TUI::PrepareRedraw()
{
	VERIFY(m_bReady);
	if (m_Flags.is(flResize)) 			RealResize();
// set render state
    EDevice.SetRS(D3DRS_TEXTUREFACTOR,	0xffffffff);
    // fog
    u32 fog_color;
	float fog_start, fog_end;
    Tools->GetCurrentFog	(fog_color, fog_start, fog_end);
/*
    if (0==g_pGamePersistent->Environment().GetWeather().size())
    {
        g_pGamePersistent->Environment().CurrentEnv->fog_color.set	(color_get_R(fog_color),color_get_G(fog_color),color_get_B(fog_color));
        g_pGamePersistent->Environment().CurrentEnv->fog_far		= fog_end;
        g_pGamePersistent->Environment().CurrentEnv->fog_near		= fog_start;
    }
*/    
	EDevice.SetRS( D3DRS_FOGCOLOR,		fog_color			);
	EDevice.SetRS( D3DRS_RANGEFOGENABLE,	FALSE				);
	if (HW.Caps.bTableFog)	{
		EDevice.SetRS( D3DRS_FOGTABLEMODE,	D3DFOG_LINEAR 	);
		EDevice.SetRS( D3DRS_FOGVERTEXMODE,	D3DFOG_NONE	 	);
	} else {
		EDevice.SetRS( D3DRS_FOGTABLEMODE,	D3DFOG_NONE	 	);
		EDevice.SetRS( D3DRS_FOGVERTEXMODE,	D3DFOG_LINEAR	);
	}
	EDevice.SetRS( D3DRS_FOGSTART,	*(DWORD *)(&fog_start)	);
	EDevice.SetRS( D3DRS_FOGEND,		*(DWORD *)(&fog_end)	);
    // filter
    for (u32 k=0; k<HW.Caps.raster.dwStages; k++){
        if( psDeviceFlags.is(rsFilterLinear)){
            EDevice.SetSS(k,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
            EDevice.SetSS(k,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
            EDevice.SetSS(k,D3DSAMP_MIPFILTER,D3DTEXF_LINEAR);
        } else {
            EDevice.SetSS(k,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
            EDevice.SetSS(k,D3DSAMP_MINFILTER,D3DTEXF_POINT);
            EDevice.SetSS(k,D3DSAMP_MIPFILTER,D3DTEXF_POINT);
        }
    }
	// ligthing
    if (psDeviceFlags.is(rsLighting)) 	EDevice.SetRS(D3DRS_AMBIENT,0x00000000);
    else                				EDevice.SetRS(D3DRS_AMBIENT,0xFFFFFFFF);

    EDevice.SetRS			(D3DRS_FILLMODE, EDevice.dwFillMode);
    EDevice.SetRS			(D3DRS_SHADEMODE,EDevice.dwShadeMode);

    RCache.set_xform_world	(Fidentity);
}
void TUI::Redraw()
{
	PrepareRedraw();
    try{
    	EDevice.Statistic->RenderDUMP_RT.Begin();
        if (EDevice.Begin()){
            EDevice.UpdateView		();
            EDevice.ResetMaterial	();

            Tools->RenderEnvironment	();

            //. temporary reset filter (      )
            for (u32 k=0; k<HW.Caps.raster.dwStages; k++){
                if( psDeviceFlags.is(rsFilterLinear)){
                    EDevice.SetSS(k,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
                    EDevice.SetSS(k,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
                    EDevice.SetSS(k,D3DSAMP_MIPFILTER,D3DTEXF_LINEAR);
                } else {
                    EDevice.SetSS(k,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
                    EDevice.SetSS(k,D3DSAMP_MINFILTER,D3DTEXF_POINT);
                    EDevice.SetSS(k,D3DSAMP_MIPFILTER,D3DTEXF_POINT);
                }
            }

            // draw grid
            if (psDeviceFlags.is(rsDrawGrid)){
                DU_impl.DrawGrid		();
                DU_impl.DrawPivot		(m_Pivot);
            }

            try{
	            Tools->Render			();
		    }catch(...){
		    	ELog.DlgMsg(mtError, "Please notify AlexMX!!! Critical error has occured in render routine!!! [Type B]");
            }

            // draw selection rect
            if(m_SelectionRect) 	DU_impl.DrawSelectionRect(m_SelStart,m_SelEnd);

            // draw axis
            DU_impl.DrawAxis(EDevice.m_Camera.GetTransform());

            try{
	            // end draw
    	        EDevice.End();
		    }catch(...){
		    	ELog.DlgMsg(mtError, "Please notify AlexMX!!! Critical error has occured in render routine!!! [Type C]");
            }
        }
    	EDevice.Statistic->RenderDUMP_RT.End();
    }catch(...){
    	ELog.DlgMsg(mtError, "Please notify AlexMX!!! Critical error has occured in render routine!!! [Type A]");
//		_clear87();
//		FPU::m24r();
//    	ELog.DlgMsg(mtError, "Critical error has occured in render routine.\nEditor may work incorrectly.");
        EDevice.End();
//		EDevice.Resize(m_D3DWindow->Width,m_D3DWindow->Height);
    }

	OutInfo();
}
//---------------------------------------------------------------------------
void TUI::RealResize()
{
    EDevice.Resize		(m_D3DWindow->Width,m_D3DWindow->Height);
    m_Flags.set			(flResize,FALSE);
    ExecCommand			(COMMAND_UPDATE_PROPERTIES);
}
void TUI::RealUpdateScene()
{
    Tools->UpdateProperties	(false);
    m_Flags.set			(flUpdateScene,FALSE);
}
void TUI::RealRedrawScene()
{
    if (!psDeviceFlags.is(rsRenderRealTime)) 
    	m_Flags.set		(flRedraw,FALSE);
    Redraw				();         
}
void TUI::OnFrame()
{
	EDevice.FrameMove	();
    SndLib->OnFrame		();
    // tools on frame
    if (m_Flags.is(flUpdateScene)) RealUpdateScene();
    Tools->OnFrame		();
	// show hint
    ShowObjectHint		();
	ResetBreak			();
	// check mail
    CheckMailslot		();
    // OnFrame
    TfrmImageLib::OnFrame();
    TfrmSoundLib::OnFrame();
    TfrmChoseItem::OnFrame();
    // Progress
    ProgressDraw		();
}
void TUI::Idle()         
{
	VERIFY(m_bReady);
    EDevice.b_is_Active  = Application->Active;
	// input
    pInput->OnFrame();
    Sleep(1);
    if (ELog.in_use) return;

    OnFrame			();
    if (m_Flags.is(flRedraw))	RealRedrawScene();

    // test quit
    if (m_Flags.is(flNeedQuit))	RealQuit();
}
//---------------------------------------------------------------------------
void ResetActionToSelect()
{
    ExecCommand(COMMAND_CHANGE_ACTION, etaSelect);
}
//---------------------------------------------------------------------------

#define MIN_PANEL_HEIGHT 15
void __fastcall PanelMinMax(TPanel *pa)
{
	if (pa&&(pa->Align!=alClient)){
        if (pa->Tag > 0){
            pa->Height = pa->Tag;
            pa->Tag    = 0;
        }else{
            pa->Tag    = pa->Height;
            pa->Height = MIN_PANEL_HEIGHT;
        }
        ExecCommand(COMMAND_UPDATE_TOOLBAR);
    }
}
void __fastcall PanelMinimize(TPanel *pa)
{
	if (pa&&(pa->Align!=alClient)){
        if (pa->Tag <= 0){
            pa->Tag    = pa->Height;
            pa->Height = MIN_PANEL_HEIGHT;
        }
        ExecCommand(COMMAND_UPDATE_TOOLBAR);
    }
}
void __fastcall PanelMaximize(TPanel *pa)
{
	if (pa&&(pa->Align!=alClient)){
        if (pa->Tag > 0){
            pa->Height = pa->Tag;
            pa->Tag    = 0;
        }
        ExecCommand(COMMAND_UPDATE_TOOLBAR);
    }
}
void __fastcall PanelMinMaxClick(TObject* Sender)
{
    PanelMinMax(((TPanel*)((TControl*)Sender)->Parent));
}
void __fastcall PanelMinimizeClick(TObject* Sender)
{
    PanelMinimize(((TPanel*)((TControl*)Sender)->Parent));
}
void __fastcall PanelMaximizeClick(TObject* Sender)
{
    PanelMaximize(((TPanel*)((TControl*)Sender)->Parent));
}
//---------------------------------------------------------------------------

bool TUI::OnCreate(TD3DWindow* w, TPanel* p)
{
// create base class
	EDevice.InitTimer();

    m_D3DWindow 	= w;
    m_D3DPanel		= p;
    VERIFY(m_D3DWindow);
    EDevice.Initialize();
    
	// Creation
	ETOOLS::ray_options	(CDB::OPT_ONLYNEAREST | CDB::OPT_CULL);

    pInput			= xr_new<CInput>(FALSE,mouse_device_key);
    UI->IR_Capture	();

    m_bReady		= true;

    if (!CreateMailslot()){
    	ELog.DlgMsg	(mtError,"Can't create mail slot.\nIt's possible two Editors started.");
        return 		false;
    }

    if (!FS.path_exist(_local_root_)){
    	ELog.DlgMsg	(mtError,"Undefined Editor local directory.");
        return 		false;
    }

	BeginEState		(esEditScene);

    return true;
}

void TUI::OnDestroy()
{
	VERIFY(m_bReady);
	m_bReady		= false;
	UI->IR_Release	();
    xr_delete		(pInput);
    EndEState		();

    EDevice.ShutDown	();
    
}

SPBItem* TUI::ProgressStart		(float max_val, LPCSTR text)
{
	VERIFY(m_bReady);
	SPBItem* item 				= xr_new<SPBItem>(text,"",max_val);
    m_ProgressItems.push_back	(item);
    ELog.Msg					(mtInformation,text);
    ProgressDraw				();
	return item;
}
void TUI::ProgressEnd			(SPBItem*& pbi)
{
	VERIFY(m_bReady);
    if (pbi){
        PBVecIt it=std::find(m_ProgressItems.begin(),m_ProgressItems.end(),pbi); VERIFY(it!=m_ProgressItems.end());
        m_ProgressItems.erase	(it);
        xr_delete				(pbi);
        ProgressDraw			();
    }
}

void SPBItem::GetInfo			(AnsiString& txt, float& p, float& m)
{
    if (info.size())txt.sprintf("%s (%s)",text.c_str(),info.c_str());
    else			txt.sprintf("%s",text.c_str());
    p				= progress;
    m				= max;
}  
void SPBItem::Inc				(LPCSTR info, bool bWarn)
{
    Info						(info,bWarn);
    Update						(progress+1.f);
}
void SPBItem::Update			(float val)
{
    progress					= val;
    UI->ProgressDraw			();
}
void SPBItem::Info				(LPCSTR text, bool bWarn)
{
	if (text&&text[0]){
    	info					= text;
        AnsiString 				txt;
        float 					p,m;
        GetInfo					(txt,p,m);
	    ELog.Msg				(bWarn?mtError:mtInformation,txt.c_str());
	    UI->ProgressDraw		();
    }
}

