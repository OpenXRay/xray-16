#include "stdafx.h"
#pragma hdrstop

#include "LeftBar.h"
#include "BottomBar.h"
#include "UI_LevelTools.h"
#include "main.h"
#include "ObjectList.h"
#include "scene.h"
#include "EditLibrary.h"
#include "UI_LevelTools.h"
#include "ui_levelmain.h"
#include "CustomObject.h"
#include "ESceneCustomMTools.h"
#include "FrmDBXpacker.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ExtBtn"
#pragma link "MxMenus"
#pragma link "mxPlacemnt"
#pragma resource "*.dfm"
TfraLeftBar *fraLeftBar;

//---------------------------------------------------------------------------
__fastcall TfraLeftBar::TfraLeftBar(TComponent* Owner)
        : TFrame(Owner)
{
	m_TargetButtons.resize	(OBJCLASS_COUNT);
    m_TargetButtons[OBJCLASS_GROUP     	]		= std::make_pair(ebTargetGroup,btEnableGroup);
    m_TargetButtons[OBJCLASS_SCENEOBJECT]		= std::make_pair(ebTargetObject,btEnableObject);
    m_TargetButtons[OBJCLASS_LIGHT      ]		= std::make_pair(ebTargetLight,btEnableLight);
    m_TargetButtons[OBJCLASS_SHAPE      ]		= std::make_pair(ebTargetShape,btEnableShape);
    m_TargetButtons[OBJCLASS_SOUND_SRC  ]		= std::make_pair(ebTargetSoundSrc,btEnableSoundSrc);
    m_TargetButtons[OBJCLASS_SOUND_ENV  ]		= std::make_pair(ebTargetSoundEnv,btEnableSoundEnv);
    m_TargetButtons[OBJCLASS_GLOW       ]		= std::make_pair(ebTargetGlow,btEnableGlow);
    m_TargetButtons[OBJCLASS_SPAWNPOINT ]		= std::make_pair(ebTargetSpawnPoint,btEnableSpawnPoint);
    m_TargetButtons[OBJCLASS_WAY        ]		= std::make_pair(ebTargetWay,btEnableWay);
    m_TargetButtons[OBJCLASS_SECTOR     ]		= std::make_pair(ebTargetSector,btEnableSector);
    m_TargetButtons[OBJCLASS_PORTAL     ]		= std::make_pair(ebTargetPortal,btEnablePortal);
    m_TargetButtons[OBJCLASS_PS         ]		= std::make_pair(ebTargetPS,btEnablePS);
    m_TargetButtons[OBJCLASS_DO         ]		= std::make_pair(ebTargetDO,btEnableDO);
    m_TargetButtons[OBJCLASS_AIMAP      ]		= std::make_pair(ebTargetAIMap,btEnableAIMap);
    m_TargetButtons[OBJCLASS_WM         ]		= std::make_pair(ebTargetWallmarks,btEnableWallmarks);
    m_TargetButtons[OBJCLASS_FOG_VOL    ]		= std::make_pair(ebTargetFogVolumes,btEnableFogVolumes);

    int startGroupIndex = 999;
    for (ObjClassID k=OBJCLASS_FIRST_CLASS; k<OBJCLASS_COUNT; k++)
    {
    	m_TargetButtons[k].first->Tag					= k;
    	m_TargetButtons[k].second->Tag					= k;
        m_TargetButtons[k].second->GroupIndex			= ++startGroupIndex;
    }

    DEFINE_INI				(fsStorage);
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

void GetHeight(int& h, TForm* f)
{
    if (f){
        TPanel* pa;
        for (int j=0; j<f->ControlCount; j++){
            TComponent* temp = f->Controls[j];
            GetHeight(h,dynamic_cast<TForm*>(temp));
            pa = dynamic_cast<TPanel*>(temp);
            if (pa&&pa->Visible){ h+=pa->Height; UpdatePanel(pa);}
        }
    }
}
//---------------------------------------------------------------------------

void TfraLeftBar::OnTimer()
{
	RedrawBar();
}

void TfraLeftBar::RedrawBar()
{
    int i, j, h=0;
    for (i=0; i<fraLeftBar->ComponentCount; i++){
        TComponent* temp = fraLeftBar->Components[i];
        if (dynamic_cast<TExtBtn *>(temp) != NULL)
            ((TExtBtn*)temp)->UpdateMouseInControl();
    }
    for (i=0; i<paFrames->ControlCount; i++)
        GetHeight(h,dynamic_cast<TForm*>(paFrames->Controls[i]));
    
    int hh = fraLeftBar->Height-(paLeftBar->Height+h);
    if (hh<=0) hh = 0; 
    paFrames->Height = h+hh;

    h=0;
    for (j=0; j<paLeftBar->ControlCount; j++){
        TPanel* pa = dynamic_cast<TPanel*>(paLeftBar->Controls[j]);
        if (pa&&pa->Visible) h+=pa->Height;
    }
    paLeftBar->Height = h+2;
    paFrames->Top = paLeftBar->Top+paLeftBar->Height;
}
//---------------------------------------------------------------------------

void TfraLeftBar::UpdateBar()
{
    for (int i=0; i<paFrames->ControlCount; i++){
        TForm* f = dynamic_cast<TForm*>(paFrames->Controls[i]);
        if (f){
            for (int j=0; j<f->ControlCount; j++){
                TPanel* pa = dynamic_cast<TPanel*>(f->Controls[j]);
                if (pa){
                    if (pa->Align==alClient){
                        paFrames->Height-=(pa->Height-pa->Constraints->MinHeight);
                    }
                }
            }
        }
    }
    RedrawBar();
}

void TfraLeftBar::MinimizeAllFrames()
{
    for (int i=0; i<paFrames->ControlCount; i++){
		TForm* f = dynamic_cast<TForm*>(paFrames->Controls[i]);
        if (f){
	        for (int j=0; j<f->ControlCount; j++){
                TPanel* pa = dynamic_cast<TPanel*>(f->Controls[j]);
                if (pa) PanelMinimize(pa);
            }
        }
    }
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
    for (int i=0; i<paFrames->ControlCount; i++){
		TForm* f = dynamic_cast<TForm*>(paFrames->Controls[i]);
        if (f){
	        for (int j=0; j<f->ControlCount; j++){
                TPanel* pa = dynamic_cast<TPanel*>(f->Controls[j]);
                if (pa){
                	if (pa->Align==alClient){
                        paFrames->Height-=(pa->Height-pa->Constraints->MinHeight);
                    }else
	                 	PanelMaximize(pa);
                }
            }
        }
    }
	UpdateBar();
}
//---------------------------------------------------------------------------

void TfraLeftBar::ChangeTarget(ObjClassID tgt)
{
    for (int i=0; i<paTarget->ControlCount; i++){
    	TExtBtn* B = dynamic_cast<TExtBtn *>(paTarget->Controls[i]);
        if (B&&ObjClassID(B->Tag)==tgt)	B->Down = true;
    }
    UI->RedrawScene	();
    UpdateBar		();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebClearClick(TObject *Sender)
{
	ExecCommand( COMMAND_CLEAR );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebLoadClick(TObject *Sender)
{
	ExecCommand( COMMAND_LOAD );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSaveClick(TObject *Sender)
{
	ExecCommand( COMMAND_SAVE, xr_string(LTools->m_LastFileName.c_str()) );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSaveAsClick(TObject *Sender)
{
	ExecCommand( COMMAND_SAVE, 0, 1 );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebBuildClick(TObject *Sender)
{
	ExecCommand( COMMAND_BUILD );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::miSceneSummaryClick(TObject *Sender)
{
	ExecCommand( COMMAND_CLEAR_SCENE_SUMMARY );
	ExecCommand( COMMAND_COLLECT_SCENE_SUMMARY );
	ExecCommand( COMMAND_SHOW_SCENE_SUMMARY );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::miHightlightTextureClick(TObject *Sender)
{
	ExecCommand( COMMAND_SCENE_HIGHLIGHT_TEXTURE );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebOptionsClick(TObject *Sender)
{
	ExecCommand( COMMAND_OPTIONS, CCommandVar("Scene") );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebCutClick(TObject *Sender)
{
	ExecCommand( COMMAND_CUT );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebCopyClick(TObject *Sender)
{
	ExecCommand( COMMAND_COPY );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebPasteClick(TObject *Sender)
{
 	ExecCommand( COMMAND_PASTE );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebUndoClick(TObject *Sender)
{
 	ExecCommand( COMMAND_UNDO );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebRedoClick(TObject *Sender)
{
 	ExecCommand( COMMAND_REDO );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebOpenSelClick(TObject *Sender)
{
 	ExecCommand( COMMAND_LOAD_SELECTION );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSaveSelClick(TObject *Sender)
{
 	ExecCommand( COMMAND_SAVE_SELECTION );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebValidateSceneClick(TObject *Sender)
{
	ExecCommand( COMMAND_VALIDATE_SCENE );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebRefreshEditorClick(TObject *Sender)
{
	ExecCommand( COMMAND_REFRESH_TEXTURES );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::RefreshObjects1Click(TObject *Sender)
{
	ExecCommand( COMMAND_RELOAD_OBJECTS );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::CleanLibraryClick(TObject *Sender)
{
	ExecCommand( COMMAND_CLEAN_LIBRARY );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::CheckNewTexturesClick(TObject *Sender)
{
	ExecCommand( COMMAND_CHECK_TEXTURES );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ImageEditor1Click(TObject *Sender)
{
	ExecCommand( COMMAND_IMAGE_EDITOR );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::UpdateSceneTextures1Click(TObject *Sender)
{
	ExecCommand( COMMAND_RELOAD_TEXTURES );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MenuItem9Click(TObject *Sender)
{
	ExecCommand( COMMAND_REFRESH_TEXTURES );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MenuItem14Click(TObject *Sender)
{
	ExecCommand( COMMAND_REFRESH_SOUND_ENVS );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::UpdateEnvironmentGeometry1Click(
      TObject *Sender)
{
	ExecCommand( COMMAND_REFRESH_SOUND_ENV_GEOMETRY );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebObjectListClick(TObject *Sender)
{
	ExecCommand( COMMAND_SHOW_OBJECTLIST );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebEditLibClick(TObject *Sender)
{
	ExecCommand( COMMAND_LIBRARY_EDITOR );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::TargetClick(TObject *Sender)
{
    TExtBtn* btn=dynamic_cast<TExtBtn*>(Sender);    VERIFY(btn);
    ExecCommand(COMMAND_CHANGE_TARGET, btn->Down?btn->Tag:OBJCLASS_DUMMY);
    // turn off snap mode
    ebSnapListMode->Down 	= false;
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::PanelMimimizeClickClick(TObject *Sender)
{
    PanelMinMaxClick(Sender);
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

void __fastcall TfraLeftBar::ebInvertClick(TObject *Sender)
{
	ExecCommand(COMMAND_INVERT_SELECTION_ALL);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSelectAllClick(TObject *Sender)
{
	ExecCommand(COMMAND_SELECT_ALL);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebUnselectAllClick(TObject *Sender)
{
	ExecCommand(COMMAND_DESELECT_ALL);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebDeleteClick(TObject *Sender)
{
	ExecCommand(COMMAND_DELETE_SELECTION);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebHideAllClick(TObject *Sender)
{
	ExecCommand(COMMAND_HIDE_ALL,FALSE);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebUnhideAllClick(TObject *Sender)
{
	ExecCommand(COMMAND_HIDE_ALL,TRUE);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebHideSelectedClick(TObject *Sender)
{
	ExecCommand(COMMAND_HIDE_SEL,FALSE);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebHideUnselectedClick(TObject *Sender)
{
	ExecCommand(COMMAND_HIDE_UNSEL);
}
//---------------------------------------------------------------------------


void __fastcall TfraLeftBar::ebMakeGameClick(TObject *Sender)
{
	ExecCommand( COMMAND_MAKE_GAME );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MakeDetailsClick(TObject *Sender)
{
	ExecCommand( COMMAND_MAKE_DETAILS );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MakeHOM1Click(TObject *Sender)
{
	ExecCommand( COMMAND_MAKE_HOM );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MakeSoundOccluder1Click(TObject *Sender)
{
	ExecCommand( COMMAND_MAKE_SOM );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MakeAIMap1Click(TObject *Sender)
{
	ExecCommand( COMMAND_MAKE_AIMAP );
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSceneFileMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmSceneFile,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSceneMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmScene,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebCompileMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmCompile,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExtBtn2MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmObjects,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebImagesMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmImages,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSoundsMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmSounds,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExtBtn7MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmToolsEdit,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExtBtn8MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmToolsSelection,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExtBtn10MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmToolsVisibility,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------


void __fastcall TfraLeftBar::ebLightAnimationEditorClick(TObject *Sender)
{
	ExecCommand(COMMAND_LANIM_EDITOR);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebClearSnapClick(TObject *Sender)
{
	if (ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo,"Are you sure to clear snap objects?")==mrYes)
		ExecCommand(COMMAND_CLEAR_SNAP_OBJECTS);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebSetSnapClick(TObject *Sender)
{
	ExecCommand(COMMAND_SET_SNAP_OBJECTS);
}
//---------------------------------------------------------------------------
void __fastcall TfraLeftBar::miAddSelectedToListClick(TObject *Sender)
{
	ExecCommand(COMMAND_ADD_SEL_SNAP_OBJECTS);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::SelectObjectFromList1Click(TObject *Sender)
{
	ExecCommand(COMMAND_SELECT_SNAP_OBJECTS);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::RemoveSelectedFromList1Click(TObject *Sender)
{
	ExecCommand(COMMAND_DEL_SEL_SNAP_OBJECTS);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::UpdateSnapList()
{
	lbSnapList->Items->Clear();
    ObjectList* lst = Scene->GetSnapList(true);
    if (lst&&!lst->empty()){
    	int idx=0;
        ObjectIt _F=lst->begin();
    	for (;_F!=lst->end(); _F++,idx++){
        	AnsiString s; s.sprintf("%d: %s",idx,(*_F)->Name);
        	lbSnapList->Items->Add(s);
        }
    }
    Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExtBtn1MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	FHelper.ShowPPMenu(pmSnapListCommand,dynamic_cast<TExtBtn*>(Sender));
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebUseSnapListClick(TObject *Sender)
{
	UI->RedrawScene();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::miPropertiesClick(TObject *Sender)
{
    ExecCommand(COMMAND_SHOW_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Quit1Click(TObject *Sender)
{
	ExecCommand(COMMAND_QUIT);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebPropertiesClick(TObject *Sender)
{
	ExecCommand(COMMAND_SHOW_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebMultiRenameClick(TObject *Sender)
{
	ExecCommand(COMMAND_MULTI_RENAME_OBJECTS);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ImportCompilerErrorsClick(TObject *Sender)
{
	ExecCommand(COMMAND_IMPORT_COMPILER_ERROR);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExportErrorList1Click(TObject *Sender)
{
	ExecCommand(COMMAND_EXPORT_COMPILER_ERROR);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ebIgnoreModeClick(TObject *Sender)
{
	ExecCommand(COMMAND_UPDATE_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::miClearErrorListClick(TObject *Sender)
{
	ExecCommand(COMMAND_CLEAR_DEBUG_DRAW);
}
//---------------------------------------------------------------------------


void __fastcall TfraLeftBar::ebSnapListModeClick(TObject *Sender)
{
    TExtBtn* E = dynamic_cast<TExtBtn*>(Sender); VERIFY(E);
	if (E->Down&&(etaSelect!=Tools->GetAction())){
    	ELog.Msg(mtError,"Before modify snap list activate select mode!");
        E->Down = false;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::MenuItem12Click(TObject *Sender)
{
	ExecCommand(COMMAND_SOUND_EDITOR);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::SynchronizeSounds1Click(TObject *Sender)
{
	ExecCommand(COMMAND_SYNC_SOUNDS);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::miRecentFilesClick(TObject *Sender)
{
	TMenuItem* MI 	= dynamic_cast<TMenuItem*>(Sender); R_ASSERT(MI&&(MI->Tag==0x1001));
    xr_string fn 	= AnsiString(MI->Caption).c_str();
    ExecCommand		(COMMAND_LOAD,fn);
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
    // refresh target
    for (ObjClassID k=OBJCLASS_FIRST_CLASS; k<OBJCLASS_COUNT; ++k)
    {
    	ESceneToolBase* t 			= Scene->GetTool(k);
        if(!t)
        {
        	Msg("unknown tool");
        }else
        {
            m_TargetButtons[k].first->Enabled		= t->IsEnabled();
            m_TargetButtons[k].first->NormalColor	= t->IsEditable()?clBlack:clGray;
            m_TargetButtons[k].first->Font->Style	= t->IsForceReadonly()?TFontStyles()<<fsStrikeOut:TFontStyles();

            m_TargetButtons[k].second->Down			= t->IsVisible();
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ClearDebugDraw1Click(TObject *Sender)
{
	ExecCommand(COMMAND_CLEAR_DEBUG_DRAW);
}
//---------------------------------------------------------------------------


void __fastcall TfraLeftBar::Editminimap1Click(TObject *Sender)
{
	ExecCommand( COMMAND_MINIMAP_EDITOR );
}
//---------------------------------------------------------------------------
//. #include "..\ECore\editor\EThumbnailTexture.h"
#include "..\ECore\editor\EThumbnail.h"
void __fastcall TfraLeftBar::SyncTHM1Click(TObject *Sender)
{
        FS_FileSet      files;
        FS.file_list    (files,_textures_,FS_ListFiles,"*.thm");
        FS_FileSet::iterator I = files.begin();
        FS_FileSet::iterator E = files.end();

        for(; I!=E; ++I)
        {
                ETextureThumbnail* TH = xr_new<ETextureThumbnail>( (*I).name.c_str(), false);
                TH->Load((*I).name.c_str(),_textures_);
                TH->Save();
                xr_delete(TH);
        }
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::Makepack1Click(TObject *Sender)
{
        TDB_packer::ActivatePacker();
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ExportObjClick(TObject *Sender)
{
        Scene->ExportObj(false);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::e1Click(TObject *Sender)
{
        Scene->ExportObj(true);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::btEnableObjectClick(TObject *Sender)
{
    TExtBtn* btn=dynamic_cast<TExtBtn*>(Sender);    VERIFY(btn);

	ESceneToolBase* M		= Scene->GetTool(btn->Tag); 
    VERIFY					(M);
    int whatToDo			= M->IsVisible()? 0 : 1;
	ExecCommand				(COMMAND_SHOW_TARGET, btn->Tag, whatToDo);
}
//---------------------------------------------------------------------------

void __fastcall TfraLeftBar::ClipEditor1Click(TObject *Sender)
{
	ExecCommand				(COMMAND_SHOW_CLIP_EDITOR);
}
//---------------------------------------------------------------------------

