//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "main.h"

TfrmMain *frmMain;
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "RenderWindow"
#pragma resource "*.dfm"

#include "topbar.h"
#include "leftbar.h"
#include "bottombar.h"

//------------------------------------------------------------------------------
#include "../xrEProps/ChoseForm.h"
#include "../../Layers/xrRender/shader.h"
#include "shader_xrlc.h"
#include "../ECore/Engine/texture.h"
#include "../ECore/Editor/Library.h"
#include "../ECore/Editor/EditObject.h"
#include "../ECore/Editor/EThumbnail.h"
#include "../xrEProps/FolderLib.h"
#include "LightAnimLibrary.h"
#include "../ECore/Editor/ImageManager.h"
#include "../ECore/Editor/SoundManager.h"
#include "../ECore/Editor/ui_main.h"
#include "..\..\Layers\xrRender\PSLibrary.h"
#include "GameMtlLib.h"
#include "../../xrSound/soundrender_source.h"
#include "../ECore/Editor/render.h"
#include "ResourceManager.h"
#include "../xrEProps/EditorChooseEvents.h"


__fastcall TfrmMain::TfrmMain(TComponent* Owner)
        : TForm(Owner)
{
// forms
    fraBottomBar	= xr_new<TfraBottomBar>	((TComponent*)0);
    fraTopBar   	= xr_new<TfraTopBar>	((TComponent*)0);
    fraLeftBar  	= xr_new<TfraLeftBar>	((TComponent*)0);
//-

	fraBottomBar->Parent    = paBottomBar;
	fraTopBar->Parent       = paTopBar;
	fraLeftBar->Parent      = paLeftBar;
	if (paLeftBar->Tag > 0) paLeftBar->Parent = paTopBar;
	else paLeftBar->Parent 	= frmMain;

	EDevice.SetHandle		(Handle,D3DWindow->Handle);
    EnableReceiveCommands	();
    if (!ExecCommand(COMMAND_INITIALIZE,(u32)D3DWindow,(u32)paRender)){ 
    	FlushLog			();
    	TerminateProcess(GetCurrentProcess(),-1);
    }
}
//---------------------------------------------------------------------------
void __fastcall TfrmMain::FormShow(TObject *Sender)
{
    tmRefresh->Enabled 		= true; tmRefreshTimer(Sender);
    ExecCommand				(COMMAND_UPDATE_GRID);
    ExecCommand				(COMMAND_RENDER_FOCUS);
    FillChooseEvents		();

    // special case :(
	frmMain->WindowState 	= (TWindowState)fsStorage->ReadInteger("window_state",frmMain->WindowState);
}
//---------------------------------------------------------------------------
void __fastcall TfrmMain::FormClose(TObject *Sender, TCloseAction &Action)
{
    Application->OnIdle     = 0;

    ClearChooseEvents		();

    ExecCommand				(COMMAND_DESTROY);

	fraTopBar->Parent       = 0;
	fraLeftBar->Parent      = 0;
	fraBottomBar->Parent    = 0;

    xr_delete(fraTopBar);
    xr_delete(fraBottomBar);
	xr_delete(fraLeftBar);
}
//---------------------------------------------------------------------------
void __fastcall TfrmMain::FormCloseQuery(TObject *Sender, bool &CanClose)
{
    tmRefresh->Enabled = false;
    CanClose = ExecCommand(COMMAND_EXIT);
    if (!CanClose) tmRefresh->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TfrmMain::FormCreate(TObject *Sender)
{
	DEFINE_INI(fsStorage);
    Application->OnIdle = IdleHandler;
}

//---------------------------------------------------------------------------


#define MIN_PANEL_HEIGHT 17
void __fastcall TfrmMain::sbToolsMinClick(TObject *Sender)
{
    if (paLeftBar->Tag > 0){
        paLeftBar->Parent = frmMain;
        paLeftBar->Tag    = 0;
    }else{
        paLeftBar->Parent = paTopBar;
        paLeftBar->Tag    = 1;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::TopClick(TObject *Sender)
{
    if (paLeftBar->Tag > 0){
        paLeftBar->Align  = alRight;
        paLeftBar->Parent = frmMain;
        paLeftBar->Height = paLeftBar->Tag;
        paLeftBar->Tag    = 0;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::IdleHandler(TObject *Sender, bool &Done)
{
    Done = false;
    UI->Idle();
}
void __fastcall TfrmMain::D3DWindowResize(TObject *Sender)
{
    UI->Resize();
}                     
//---------------------------------------------------------------------------

void __fastcall TfrmMain::D3DWindowKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    ShiftKey = Shift;
    if (!UI->KeyDown(Key, Shift)){UI->ApplyShortCut(Key, Shift);}
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::D3DWindowKeyUp(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (!UI->KeyUp(Key, Shift)){;}
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::D3DWindowKeyPress(TObject *Sender, char &Key)
{
    if (!UI->KeyPress(Key, ShiftKey)){;}
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
    if (!D3DWindow->Focused()) UI->ApplyGlobalShortCut(Key, Shift);
	if (Key==VK_MENU) Key=0;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::UpdateCaption()
{
    AnsiString name;
    name.sprintf("%s - [%s%s]",UI->EditorDesc(),UI->GetCaption(),UI->IsModified()?"*":"");
    Caption = name;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::tmRefreshTimer(TObject *Sender)
{
    int i;
    for (i=0; i<frmMain->ComponentCount; i++){
        TComponent* temp = frmMain->Components[i];
        if (dynamic_cast<TExtBtn *>(temp) != NULL)
            ((TExtBtn*)temp)->UpdateMouseInControl();
    }
	fraLeftBar->OnTimer();
	fraTopBar->OnTimer();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::D3DWindowPaint(TObject *Sender)
{
	if (!UI||!UI->m_bReady) return;
    UI->RedrawScene();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::D3DWindowChangeFocus(TObject *Sender)
{
	if (!UI||!UI->m_bReady) return;
	if (D3DWindow->Focused()){
        UI->IR_Capture();
		UI->OnAppActivate();
    }else{
		UI->OnAppDeactivate();
        UI->IR_Release();
//        paRender->Color=paRender->Color; // чтобы не было  internal code gen error
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::D3DWindowMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    UI->MousePress(Shift,X,Y);
    UI->RedrawScene();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::D3DWindowMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    UI->MouseRelease(Shift,X,Y);
    UI->RedrawScene();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::D3DWindowMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    UI->MouseMove(Shift,X,Y);
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ebAllMinClick(TObject *Sender)
{
	fraLeftBar->MinimizeAllFrames();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ebAllMaxClick(TObject *Sender)
{
	fraLeftBar->MaximizeAllFrames();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::FormResize(TObject *Sender)
{
	if (fraLeftBar) fraLeftBar->UpdateBar();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::paRenderResize(TObject *Sender)
{
	ExecCommand(COMMAND_RENDER_RESIZE);
}
//---------------------------------------------------------------------------
 
void __fastcall TfrmMain::fsStorageSavePlacement(TObject *Sender)
{
    fsStorage->WriteInteger("window_state",frmMain->WindowState);
}
//---------------------------------------------------------------------------

