#include "stdafx.h"
#pragma hdrstop

#include "LeftBar.h"
#include "BottomBar.h"
#include "UI_ActorMain.h"
#include "main.h"
#include "UI_ActorTools.h"
#include "../xrEProps/FolderLib.h"
#include "../ECore/Editor/EditObject.h"
#include "Motion.h"
#include "../xrEProps/PropertiesList.h"
#include "BonePart.h"
#include "../xrEProps/NumericVector.h"
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
#pragma link "ElBtnCtl"
#pragma link "ElCheckCtl"
#pragma resource "*.dfm"
TfraLeftBar *fraLeftBar;

//---------------------------------------------------------------------------
__fastcall TfraLeftBar::TfraLeftBar(TComponent* Owner)
        : TFrame(Owner)
{
	DEFINE_INI(fsStorage);

    frmMain->paLeftBar->Width = paLeftBar->Width+2;
    frmMain->sbToolsMin->Left = paLeftBar->Width-frmMain->sbToolsMin->Width-3;
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::miRecentFilesClick(TObject *Sender)
{
	TMenuItem* MI 	= dynamic_cast<TMenuItem*>(Sender); R_ASSERT(MI&&(MI->Tag==0x1001));
    xr_string fn 	= AnsiString(MI->Caption).c_str();
    ExecCommand(COMMAND_LOAD,fn);
}
//---------------------------------------------------------------------------


void __fastcall TfraLeftBar::fsStorageSavePlacement(TObject *Sender)
{                                
    ATools->m_Props->SaveParams			(fsStorage);
    ATools->m_PreviewObject.SaveParams	(fsStorage);
    ATools->m_RenderObject.SaveParams	(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::fsStorageRestorePlacement(TObject *Sender)
{
    ATools->m_Props->RestoreParams		(fsStorage);
    ATools->m_PreviewObject.RestoreParams(fsStorage);
    ATools->m_RenderObject.RestoreParams	(fsStorage);         
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

void TfraLeftBar::UpdateBar(){
    int i;
    for (i=0; i<fraLeftBar->ComponentCount; i++){
        TComponent* temp = fraLeftBar->Components[i];
        if (dynamic_cast<TExtBtn *>(temp) != NULL)
            ((TExtBtn*)temp)->UpdateMouseInControl();
    }
    if (ebRenderEngineStyle->Down&&!ATools->IsVisualPresent()) SetRenderStyle(false);
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

void __fastcall TfraLeftBar::ImageEditor1Click(TObject *Sender)
{
	ExecCommand( COMMAND_IMAGE_EDITOR );
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

void __fastcall TfraLeftBar::ebMakePreviewClick(TObject *Sender)
{
	ExecCommand( COMMAND_MAKE_PREVIEW );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSceneFileMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmSceneFile,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSceneCommands1MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmImages,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExtBtn3MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmSounds,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebPreviewObjectClickMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmPreviewObject,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::tvMotionsStartDrag(TObject *Sender,
      TDragObject *&DragObject)
{
	FHelper.StartDrag(Sender,DragObject);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::tvMotionsDragOver(TObject *Sender,
      TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
	FHelper.DragOver(Sender,Source,X,Y,State,Accept);
}
//---------------------------------------------------------------------------
void TfraLeftBar::RenameItem(LPCSTR p0, LPCSTR p1, EItemType tp)
{
    ATools->RenameMotion(p0,p1);
}
//---------------------------------------------------------------------------
void __fastcall TfraLeftBar::tvMotionsDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
	FHelper.DragDrop(Sender,Source,X,Y,TOnItemRename(this,&TfraLeftBar::RenameItem));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Import1Click(TObject *Sender)
{
	ExecCommand( COMMAND_IMPORT );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Load1Click(TObject *Sender)
{
	ExecCommand( COMMAND_LOAD );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Clear1Click(TObject *Sender)
{
	ExecCommand( COMMAND_CLEAR );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Save2Click(TObject *Sender)
{
	ExecCommand( COMMAND_SAVE, xr_string(ATools->m_LastFileName.c_str()), 0 );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSaveAsClick(TObject *Sender)
{
	ExecCommand( COMMAND_SAVE, 0, 1);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebExportBatchClick(TObject *Sender)
{
	ExecCommand( COMMAND_BATCH_CONVERT );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebMakeThumbnailClick(TObject *Sender)
{
	ExecCommand(COMMAND_MAKE_THUMBNAIL);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebBonePartClick(TObject *Sender)
{
	frmBonePart->Run(ATools->CurrentObject());
	ExecCommand(COMMAND_UPDATE_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::miExportOGFClick(TObject *Sender)
{
	ExecCommand( COMMAND_EXPORT_OGF );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::miExportOMFClick(TObject *Sender)
{
	ExecCommand( COMMAND_EXPORT_OMF );
}
//---------------------------------------------------------------------------


void __fastcall TfraLeftBar::ebRenderStyleClick(TObject *Sender)
{
	if (Sender==ebRenderEngineStyle){
		if (!ATools->IsVisualPresent()) ExecCommand( COMMAND_MAKE_PREVIEW );
        if (!ATools->IsVisualPresent()) SetRenderStyle(false);
        else						  SetRenderStyle(true);
    }
    ExecCommand		(COMMAND_UPDATE_PROPERTIES);
    UI->RedrawScene	();
}
//---------------------------------------------------------------------------

void TfraLeftBar::SetRenderStyle(bool bEngineStyle)
{
    if (ATools->IsVisualPresent()&&bEngineStyle) 	ebRenderEngineStyle->Down = true;
    else 										ebRenderEditorStyle->Down = true;
	ATools->PlayMotion();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Custom1Click(TObject *Sender)
{
	ExecCommand( COMMAND_SELECT_PREVIEW_OBJ, false );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::none1Click(TObject *Sender)
{
	ExecCommand( COMMAND_SELECT_PREVIEW_OBJ, true );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Preferences1Click(TObject *Sender)
{
	ExecCommand( COMMAND_PREVIEW_OBJ_PREF );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExportDM1Click(TObject *Sender)
{
	ExecCommand( COMMAND_EXPORT_DM );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExportWaveFrontOBJ1Click(TObject *Sender)
{
	ExecCommand( COMMAND_EXPORT_OBJ );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExportC1Click(TObject *Sender)
{
	ExecCommand( COMMAND_EXPORT_CPP );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExtBtn1Click(TObject *Sender)
{
	ExecCommand( COMMAND_SHOW_CLIPMAKER );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MenuItem2Click(TObject *Sender)
{
	ExecCommand( COMMAND_SOUND_EDITOR );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MenuItem4Click(TObject *Sender)
{
	ExecCommand( COMMAND_SYNC_SOUNDS );
}
//---------------------------------------------------------------------------

void TfraLeftBar::RefreshBar()
{
	miRecentFiles->Clear();
    u32 idx 			= 0;
	for (AStringIt it=EPrefs->scene_recent_list.begin(); it!=EPrefs->scene_recent_list.end(); it++){
        TMenuItem *MI 	= xr_new<TMenuItem>((TComponent*)0);
        MI->Caption 	= *it;
        MI->OnClick 	= miRecentFilesClick;
        MI->Tag			= 0x1001;
        miRecentFiles->Insert(idx++,MI);
    }
    miRecentFiles->Enabled = miRecentFiles->Count;
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebOptimizeMotionsClick(TObject *Sender)
{
	ExecCommand( COMMAND_OPTIMIZE_MOTIONS );
}
//---------------------------------------------------------------------------

void TfraLeftBar::SetReadOnly(BOOL val)
{	
    ebSave->Enabled				= !val;
    ebSaveAs->Enabled			= !val;
    ebMakeThumbnail->Enabled	= !val;
    ebOptimizeMotions->Enabled	= !val;
    ebExportBatch->Enabled		= !val;
	ebExport->Enabled			= !val;
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Quit1Click(TObject *Sender)
{
	ExecCommand(COMMAND_QUIT);	
}
//---------------------------------------------------------------------------








