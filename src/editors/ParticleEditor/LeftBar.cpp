#include "stdafx.h"
#pragma hdrstop

#include "LeftBar.h"
#include "BottomBar.h"
#include "UI_ParticleMain.h"
#include "main.h"
#include "xr_trims.h"
#include "../xrEProps/FolderLib.h"
#include "UI_ParticleTools.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ExtBtn"
#pragma link "MxMenus"
#pragma link "mxPlacemnt"
#pragma link "ElTree"
#pragma link "ElXPThemedControl"
#pragma link "ElTreeAdvEdit"
#pragma link "ElPgCtl"                	
#pragma link "MXCtrls"
#pragma link "ElTree"
#pragma resource "*.dfm"
TfraLeftBar *fraLeftBar;

#define MIN_PANEL_HEIGHT 15
//---------------------------------------------------------------------------
__fastcall TfraLeftBar::TfraLeftBar(TComponent* Owner)
        : TFrame(Owner)
{
	DEFINE_INI(fsStorage);

    frmMain->paLeftBar->Width = paLeftBar->Width+2;
    frmMain->sbToolsMin->Left = paLeftBar->Width-frmMain->sbToolsMin->Width-3;
}
//---------------------------------------------------------------------------

void UpdatePanel(TPanel* p){
    if (p){
        for (int j=0; j<p->ControlCount; j++){
            TExtBtn* btn = dynamic_cast<TExtBtn *>(p->Controls[j]);
            if (btn) btn->UpdateMouseInControl();
        }
    }
}
//---------------------------------------------------------------------------

void TfraLeftBar::OnTimer()
{
	UpdateBar();
}
//---------------------------------------------------------------------------

void TfraLeftBar::UpdateBar()
{
    int i;
    for (i=0; i<fraLeftBar->ComponentCount; i++){
        TComponent* temp = fraLeftBar->Components[i];
        if (dynamic_cast<TExtBtn *>(temp) != NULL)
            ((TExtBtn*)temp)->UpdateMouseInControl();
    }
}
//---------------------------------------------------------------------------

void TfraLeftBar::MinimizeAllFrames()
{
    for (int j=0; j<paLeftBar->ControlCount; j++){
        TPanel* pa = dynamic_cast<TPanel*>(paLeftBar->Controls[j]);
	    if (pa) PanelMinimize(pa);
    }
	UpdateBar();
}
//---------------------------------------------------------------------------

void TfraLeftBar::MaximizeAllFrames()
{
    for (int j=0; j<paLeftBar->ControlCount; j++){
        TPanel* pa = dynamic_cast<TPanel*>(paLeftBar->Controls[j]);
	    if (pa)	PanelMaximize(pa);
    }
	UpdateBar();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSaveClick(TObject *Sender)
{
	ExecCommand( COMMAND_SAVE );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebReloadClick(TObject *Sender)
{
//.	
	ExecCommand( COMMAND_LOAD );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Validate1Click(TObject *Sender)
{
	ExecCommand( COMMAND_VALIDATE );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebRefreshTexturesClick(TObject *Sender)
{
	ExecCommand( COMMAND_REFRESH_TEXTURES );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::PanelMimimizeClick(TObject *Sender)
{
    ::PanelMinMaxClick(Sender);
    UpdateBar();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::PanelMaximizeClick(TObject *Sender)
{
    ::PanelMaximizeClick(Sender);
    UpdateBar();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebEditorPreferencesClick(TObject *Sender)
{
	ExecCommand(COMMAND_EDITOR_PREF);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSceneCommandsMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmPreviewObject,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebImageCommandsMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmImages,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSoundCommandsMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmSounds,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::PreviewClick(TObject *Sender)
{
	ExecCommand( COMMAND_SELECT_PREVIEW_OBJ, dynamic_cast<TMenuItem*>(Sender)->Tag );
}
//---------------------------------------------------------------------------


void __fastcall TfraLeftBar::ebEngineRemoveClick(TObject *Sender)
{
	PTools->RemoveCurrent();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebParticleCloneClick(TObject *Sender)
{
	PTools->CloneCurrent();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebPECreateClick(TObject *Sender)
{
    PTools->AppendPE(0);
	PTools->Modified();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebPGCreateClick(TObject *Sender)
{
    PTools->AppendPG(0);
	PTools->Modified();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ImageEditor1Click(TObject *Sender)
{
	ExecCommand( COMMAND_IMAGE_EDITOR );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Refresh1Click(TObject *Sender)
{
	ExecCommand( COMMAND_REFRESH_TEXTURES );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Checknewtextures1Click(TObject *Sender)
{
	ExecCommand( COMMAND_CHECK_TEXTURES );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebFileMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmEngineShadersFile,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebCreateMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmCreateMenu,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::fsStorageRestorePlacement(TObject *Sender)
{
    PTools->m_ItemProps->RestoreParams(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::fsStorageSavePlacement(TObject *Sender)
{
    PTools->m_ItemProps->SaveParams(fsStorage);
}
//---------------------------------------------------------------------------
                                                  
void __fastcall TfraLeftBar::MenuItem3Click(TObject *Sender)
{
	ExecCommand( COMMAND_SOUND_EDITOR );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MenuItem5Click(TObject *Sender)
{
	ExecCommand( COMMAND_SYNC_SOUNDS );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Preferneces1Click(TObject *Sender)
{
	ExecCommand( COMMAND_EDIT_PREVIEW_PROPS );
}
//---------------------------------------------------------------------------



void __fastcall TfraLeftBar::refLBClick(TObject *Sender)
{
	ExecCommand( COMMAND_JUMP_TO_ITEM );
}
//---------------------------------------------------------------------------



void __fastcall TfraLeftBar::Savexr1Click(TObject *Sender)
{
	ExecCommand( COMMAND_SAVE_XR );
	
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Loadxr1Click(TObject *Sender)
{
	ExecCommand( COMMAND_LOAD_XR );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Compact1Click(TObject *Sender)
{
	ExecCommand( COMMAND_COMPACT_PARTICLES );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Groupfromcurrenteffect1Click(TObject *Sender)
{
	ExecCommand( COMMAND_CREATE_GROUP_FROM_SELECTED );
}
//---------------------------------------------------------------------------


